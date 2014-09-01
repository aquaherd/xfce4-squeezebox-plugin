/***************************************************************************
 *            squeezebox-ql.c
 *
 *  Fri Aug 25 17:08:15 2007
 *  Copyright  2007  Hakan Erduman
 *  Email hakan@erduman.de
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_BACKEND_QUODLIBET

// default
#include "squeezebox.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <glib/gstdio.h>

#ifndef DBUS_TYPE_G_STRING_STRING_HASHTABLE
#define DBUS_TYPE_G_STRING_STRING_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_STRING))
#endif


/* --- */
#define QL_MAP(a) parent->a = ql##a;
#define MKTHIS qlData *this = (qlData *)thsPtr
#define QL_FIFO_PATH "/.quodlibet/control"
//#define QL_STAT_PATH "/.quodlibet/current"
#define QL_ALBUM_ART_PATH "/.quodlibet/current.cover"
#define QL_FREE(t) if(this->t) { g_free(this->t); this->t = NULL; }

typedef struct qlData {
	SPlayer *parent;
	gpointer *player;
	FILE *fp;
	gchar *fifo;
	gchar *cover;
   gchar *bin;
	gboolean isPlaying;
	gboolean isVisible;
	gboolean isShuffle;
	gboolean isRepeat;
	DBusGProxy *qlPlayer;
} qlData;

void qlStatus(gpointer thsPtr, const gchar * args);
void *QL_attach(SPlayer * parent);
#define BASENAME "quodlibet"
DEFINE_DBUS_BACKEND(QL, _("QuodLibet"), "net.sacredchao.QuodLibet", "quodlibet")

// implementation

static gboolean ql_CurrentSong (DBusGProxy *proxy, GHashTable** OUT_songProps) {
	return dbus_g_proxy_call(proxy, "CurrentSong", NULL, G_TYPE_INVALID, 
		DBUS_TYPE_G_STRING_STRING_HASHTABLE, OUT_songProps, G_TYPE_INVALID);
}

static gboolean ql_Next(DBusGProxy *proxy) {
	return dbus_g_proxy_call(proxy, "Next", NULL, G_TYPE_INVALID, G_TYPE_INVALID);
}

static gboolean ql_Previous(DBusGProxy *proxy) {
	return dbus_g_proxy_call(proxy, "Previous", NULL, G_TYPE_INVALID, G_TYPE_INVALID);
}

static gboolean ql_PlayPause(DBusGProxy *proxy) {
	return dbus_g_proxy_call(proxy, "PlayPause", NULL, G_TYPE_INVALID, G_TYPE_INVALID);
}

static void qlCallbackPaused(DBusGProxy * proxy, gpointer thsPtr) {
	MKTHIS;
	LOG("Enter qlCallback: Paused");
	this->isPlaying = FALSE;
	this->parent->Update(this->parent->sd, FALSE, estPause, NULL);	
	LOG("Leave qlCallback: Paused");
}

static void qlCallbackUnpaused(DBusGProxy * proxy, gpointer thsPtr) {
	MKTHIS;
	LOG("Enter qlCallback: Unpaused");
	this->isPlaying = TRUE;
	this->parent->Update(this->parent->sd, FALSE, estPlay, NULL);	
	LOG("Leave qlCallback: Unpaused");
}

static void qlCallbackSongStarted(DBusGProxy * proxy, GHashTable *table, gpointer thsPtr) {
	MKTHIS;
	const gchar *unknown = _("[Unknown]");
	gchar *tmpArtist = g_hash_table_lookup(table, "artist");
	gchar *tmpAlbum = g_hash_table_lookup(table, "album");
	gchar *tmpTitle = g_hash_table_lookup(table, "title");
	LOG("Enter qlCallback: SongStarted");

	if (g_file_test(this->cover, G_FILE_TEST_EXISTS))
		g_string_assign(this->parent->albumArt, this->cover);
	else
		g_string_assign(this->parent->albumArt, "");

	g_string_assign(this->parent->artist, (tmpArtist) ? tmpArtist : unknown);
	g_string_assign(this->parent->album, (tmpAlbum) ? tmpAlbum : unknown);
	g_string_assign(this->parent->title, (tmpTitle) ? tmpTitle : unknown);

	this->parent->Update(this->parent->sd, TRUE,
			 (this->isPlaying) ? estPlay : estPause, NULL);
	LOG("Leave qlCallback: SongStarted");
}

static void qlCallbackFake(gpointer thsPtr) {
	MKTHIS;
	GHashTable *table = NULL;
	LOG("Enter qlCallback: Fake");
	if(ql_CurrentSong(this->qlPlayer, &table) && table){
		qlCallbackSongStarted(this->qlPlayer, table, thsPtr);
		g_hash_table_destroy(table);
	}
	// update status
	qlStatus(this, "--status");
	LOG("Enter qlCallback: Fake");
}

static gboolean qlAssure(gpointer thsPtr, gboolean noCreate) {
	MKTHIS;
	LOG("Enter qlAssure");
	if (this->parent->bus && !this->qlPlayer) {
		GError *error = NULL;
		this->qlPlayer = dbus_g_proxy_new_for_name_owner(this->parent->bus,
							       "net.sacredchao.QuodLibet",
							       "/net/sacredchao/QuodLibet",
							       "net.sacredchao.QuodLibet",
							       &error);
		if (error) {
			LOGWARN("\tCouldn't connect to shell proxy '%s' ",
				error->message);
			g_error_free(error);
			error = NULL;
			if (!noCreate)
				this->parent->StartService(this->parent->sd, "net.sacredchao.QuodLibet");
		}
		if (this->qlPlayer) {
			// state changes
			dbus_g_proxy_add_signal(this->qlPlayer, "Paused",
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(this->qlPlayer, "Paused", 
						G_CALLBACK (qlCallbackPaused), this, NULL);

			dbus_g_proxy_add_signal(this->qlPlayer, "Unpaused",
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(this->qlPlayer, "Unpaused", 
						G_CALLBACK (qlCallbackUnpaused), this, NULL);

			//  song change
			dbus_g_proxy_add_signal(this->qlPlayer, "SongStarted",
						DBUS_TYPE_G_STRING_STRING_HASHTABLE, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(this->qlPlayer, "SongStarted",
						    G_CALLBACK (qlCallbackSongStarted), this, NULL);
			
		}
	}
	// command queue
	if (!this->fp) {
		if (g_file_test(this->fifo, G_FILE_TEST_EXISTS)) {
			LOG("Opening %s", this->fifo);
			this->fp = g_fopen(this->fifo, "w");
			LOG("%s", (this->fp) ? " OK" : " KO");
		} 
	}
	LOG("Leave qlAssure");
	return (NULL != this->qlPlayer && 0 != this->fp);
}

static gboolean qlPrintFlush(FILE * fp, const char *str) {
	int iLen = strlen(str);
	int iRet = fputs(str, fp);
	fflush(fp);
	return (iLen == iRet);
}

static gboolean qlNext(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlNext");
	if (qlAssure(this, FALSE))
		bRet = ql_Next(this->qlPlayer);
	LOG("Leave qlNext");
	return bRet;
}

static gboolean qlPrevious(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlPrevious");
	if (qlAssure(this, FALSE))
		bRet = ql_Previous(this->qlPlayer);
	LOG("Leave qlPrevious");
	return bRet;
}

static gboolean qlPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlPlayPause");
	if (qlAssure(this, FALSE))
		bRet = ql_PlayPause(this->qlPlayer);
	LOG("LEAVE qlPlayPause");
	return bRet;
}

static gboolean qlIsPlaying(gpointer thsPtr) {
	MKTHIS;
	return this->isPlaying;
}

void qlStatus(gpointer thsPtr, const gchar * args) {
	MKTHIS;
	gchar *outText = NULL;
	const gchar *argv[] = { this->bin, args, NULL };
	gint exit_status = 0;
	this->isPlaying = TRUE;
	if (g_spawn_sync(NULL, (gchar **) argv, NULL, G_SPAWN_STDERR_TO_DEV_NULL,
			 NULL, NULL, &outText, NULL, &exit_status, NULL)) {

		LOG("QL says: '%s'", outText);
		// QL generally says things like "paused AlbumList 1.000 inorder off"
		if (strlen(outText)) {
			gchar *ptr = strtok(outText, " ");
			this->isPlaying = !g_ascii_strncasecmp(ptr, "playing", 7);
			this->parent->Update(this->parent->sd, FALSE, (this->isPlaying) ? 
				estPlay : estPause, NULL);
			if ((ptr = strtok(NULL, " "))) {
				//current view
				if ((ptr = strtok(NULL, " "))) {
					//1.000 whatever
					if ((ptr = strtok(NULL, " "))) {
						//shuffle
						this->isShuffle = g_ascii_strncasecmp(
												ptr, "inorder",7);
						this->parent->UpdateShuffle(
							this->parent->sd, this->isShuffle);
						if ((ptr = strtok(NULL, " "))) {
							//repeat
							this->isRepeat = g_ascii_strncasecmp(
													ptr, "off", 2);
							this->parent->UpdateRepeat(
								this->parent->sd, this->isRepeat);
						}
					}
				}
			}
		}
		g_free(outText);
	} else {
		LOG("Could not spawn.");
	}
}

static gboolean qlToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlToggle");
	if (qlAssure(this, FALSE)) {
		bRet = qlPrintFlush(this->fp, "play-pause");
	}
	LOG("Leave qlToggle");
	return bRet;
}

static gboolean qlDetach(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlDetach");
	if (this->fp) {
		fclose(this->fp);
		this->fp = NULL;
	}
    QL_FREE(cover);
    QL_FREE(bin);
	if (this->qlPlayer) {
		g_object_unref(G_OBJECT(this->qlPlayer));
		this->qlPlayer = NULL;
	}

	LOG("Leave qlDetach");
	return bRet;
}

static gboolean qlGetRepeat(gpointer thsPtr) {
	MKTHIS;
	qlStatus(thsPtr, "--status");
	return this->isRepeat;
}

static gboolean qlSetRepeat(gpointer thsPtr, gboolean newRepeat) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlSetRepeat");
	if (qlAssure(this, FALSE)) {
		qlStatus(thsPtr, (newRepeat) ? "--repeat=1" : "--repeat=0");
		bRet = TRUE;
	} 
	LOG("LEAVE qlSetRepeat");
	return bRet;
}

static gboolean qlGetShuffle(gpointer thsPtr) {
	MKTHIS;
	qlStatus(thsPtr, "--status");
	return this->isShuffle;
}

static gboolean qlSetShuffle(gpointer thsPtr, gboolean newShuffle) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlSetShuffle");
	if (qlAssure(this, FALSE)) {
		qlStatus(thsPtr, (newShuffle) ?
			 "--order=shuffle" : "--order=inorder");
		bRet = TRUE;
	}
	LOG("LEAVE qlSetShuffle");
	return bRet;
}

static gboolean qlIsVisible(gpointer thsPtr) {
	MKTHIS;
	return this->isVisible;
}

static gboolean qlShow(gpointer thsPtr, gboolean bShow) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlShow");
	if (qlAssure(this, FALSE))
		bRet = qlPrintFlush(this->fp, "toggle-window");
	LOG("Leave qlShow");
	return bRet;
}

static gboolean qlUpdateDBUS(gpointer thsPtr, const gchar *name, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("QuodLibet has started");
		qlAssure(thsPtr, FALSE);
		qlCallbackFake(thsPtr);
	} else {
		LOG("QuodLibet has died");
		if (this->fp) {
			fclose(this->fp);
			this->fp = NULL;
		}
		if (this->qlPlayer) {
			g_object_unref(G_OBJECT(this->qlPlayer));
			this->qlPlayer = NULL;
		}
		g_string_truncate(this->parent->artist, 0);
		g_string_truncate(this->parent->album, 0);
		g_string_truncate(this->parent->title, 0);
		g_string_truncate(this->parent->albumArt, 0);
		this->parent->Update(this->parent->sd, TRUE, estStop, NULL);
	}
	return TRUE;
}

static void qlUpdateWindow(gpointer thsPtr, WnckWindow *window, gboolean appeared) {
	MKTHIS;
	LOG("QuodLibet is %s", (appeared)?"visible":"invisible");
	this->isVisible = appeared;
}

void *QL_attach(SPlayer * parent) {
	qlData *this = g_new0(qlData, 1);
	const gchar *homePath = g_get_home_dir();
	LOG("Enter QL_attach");

	QL_MAP(Assure);
	QL_MAP(Next);
	QL_MAP(Previous);
	QL_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	QL_MAP(IsPlaying);
	QL_MAP(Toggle);
	QL_MAP(Detach);
	NOMAP(Persist);
	NOMAP(Configure);
	QL_MAP(IsVisible);
	QL_MAP(Show);
	QL_MAP(UpdateDBUS);
	QL_MAP(GetRepeat);
	QL_MAP(SetRepeat);
	QL_MAP(GetShuffle);
	QL_MAP(SetShuffle);
	QL_MAP(UpdateWindow);

	// we init default values 
	this->parent = parent;
	this->fp = NULL;
	this->fifo = g_strdup_printf("%s%s", homePath, QL_FIFO_PATH);
	this->cover = g_strdup_printf("%s%s", homePath, QL_ALBUM_ART_PATH);
    this->bin = g_find_program_in_path("quodlibet");
    
	LOG("Leave QL_attach");
	return this;
}
#endif
