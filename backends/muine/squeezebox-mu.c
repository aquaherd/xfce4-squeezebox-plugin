/***************************************************************************
 *            squeezebox-mu.c
 *
 *  Fri Jan 02 23:00:09 2009
 *  Copyright  2009 by Hakan Erduman
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
#ifdef HAVE_BACKEND_MUINE

// default
#include "squeezebox.h"

// libdbus-glib for muine remote
#include "muine-player-binding.h"
#include <glib/gstdio.h>

#define MU_MAP(a) parent->a = mu##a;

typedef struct muData {
	SPlayer *parent;
	DBusGProxy *muPlayer;
	gboolean Visibility;
	eSynoptics eStat;
	gchar* artLocation;
} muData;

#define MKTHIS muData *db = (muData *)thsPtr
static gboolean muAssure(gpointer thsPtr, gboolean noCreate);
gpointer MU_attach(SPlayer * parent);
#define BASENAME "muine"
DEFINE_DBUS_BACKEND(MU, _("Muine"), "org.gnome.Muine", "muine")

// implementation


static void muCallbackStateChanged(DBusGProxy * proxy, gboolean newState, gpointer thsPtr) {
	MKTHIS;
	gboolean actPlaying = FALSE;
	LOG("Enter muCallback: StateChanged");
	if(org_gnome_Muine_Player_get_playing(db->muPlayer, &actPlaying, NULL))
		db->eStat = (actPlaying)?estPlay:estPause;
	else
		db->eStat = estErr;
	db->parent->Update(db->parent->sd, FALSE, db->eStat, NULL);
	LOG("Leave muCallback: StateChanged");
}

static const gchar* muFindTag(const gchar* tagName, const gchar* track) {
	gchar *ptr = g_strstr_len(track, -1, tagName);
	gchar *ptrStop;
	if(ptr) {
		ptr += strlen(tagName);
		ptrStop = g_strstr_len(ptr, -1, "\n");
		ptr = g_strndup(ptr, ptrStop-ptr);
	}		
	return (const gchar*)ptr;
}

static void muCallbackSongChanged(DBusGProxy * proxy, gchar* track, gpointer thsPtr) {
	MKTHIS;
	gboolean success = FALSE;
	LOG("Enter muCallback: SongChanged");
	/* What we get gere with Muine 0.8.9:
	uri: /home/herd/Music/Nine Inch Nails/Halo 6 - Fixed/04 - Throw This Away.ogg
	title: Throw This Away
	artist: Nine Inch Nails
	album: Fixed
	year: 1992
	track_number: 4
	duration: 253
	 */
	g_string_assign(db->parent->artist, muFindTag("\nartist: ", track));
	g_string_assign(db->parent->album, muFindTag("\nalbum: ", track));
	g_string_assign(db->parent->title, muFindTag("\ntitle: ", track));
	
	if(g_file_test(db->artLocation, G_FILE_TEST_EXISTS))
		g_unlink(db->artLocation);
	
	if(org_gnome_Muine_Player_write_album_cover_to_file(
		db->muPlayer, db->artLocation, &success, NULL) && success) {
		g_string_assign(db->parent->albumArt, db->artLocation);
	} else {
		const gchar *uri = muFindTag("uri: ", track);
		if(uri && uri[0])
			db->parent->FindAlbumArtByFilePath(db->parent->sd, uri);
	}
	
	db->parent->Update(db->parent->sd, TRUE, db->eStat, NULL);
	LOG("Leave muCallback: SongChanged");
}

static void muCallbackFake(gpointer thsPtr) {
	MKTHIS;
	LOG("Leave muCallback: Fake");
	if(db->muPlayer) {
		gboolean isPlaying = FALSE;
		gchar *currentTrack = NULL;
		if(org_gnome_Muine_Player_get_playing(db->muPlayer, &isPlaying, NULL))
			db->eStat = (isPlaying)?estPlay:estPause;
		else
			db->eStat = estErr;
		if(org_gnome_Muine_Player_get_current_song(db->muPlayer, &currentTrack, NULL)) {
			muCallbackSongChanged(db->muPlayer, currentTrack, db);
		}
	}
	LOG("Leave muCallback: Fake");
}

static gboolean muUpdateDBUS(gpointer thsPtr, const gchar *name, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("Muine has started");
		if (!db->muPlayer && muAssure(thsPtr, FALSE))
			muCallbackFake(thsPtr);
	} else {
		LOG("Muine has died");
		if (db->muPlayer) {
			g_object_unref(G_OBJECT(db->muPlayer));
			db->muPlayer = NULL;
		}
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		db->eStat = estStop;
		db->parent->Update(db->parent->sd, TRUE, db->eStat, NULL);
	}
	return TRUE;
}

static gboolean muAssure(gpointer thsPtr, gboolean noCreate) {
	MKTHIS;
	gboolean bRet = TRUE;
	LOG("Enter muAssure");

	if (db->parent->bus && !db->muPlayer) {
		GError *error = NULL;
		db->muPlayer = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							       "org.gnome.Muine",
							       "/org/gnome/Muine/Player",
							       "org.gnome.Muine.Player",
							       &error);
		if (error) {
			LOGWARN("\tCouldn't connect to shell proxy '%s' ",
				error->message);
			g_error_free(error);
			error = NULL;
			if (noCreate)
				bRet = FALSE;
			else {
				bRet = db->parent->StartService(db->parent->sd, "org.gnome.Muine");
			}
		}
		if (db->muPlayer) {
			// state changes
			//  playing change
			dbus_g_proxy_add_signal(db->muPlayer, "StateChanged",
						G_TYPE_BOOLEAN,
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->muPlayer,
						    "StateChanged",
						    G_CALLBACK
						    (muCallbackStateChanged),
						    db, NULL);

			//  song change
			dbus_g_proxy_add_signal(db->muPlayer, "SongChanged",
						G_TYPE_STRING,
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->muPlayer, "SongChanged",
						    G_CALLBACK
						    (muCallbackSongChanged), db,
						    NULL);

		}

	}
	// reflect UI
	if (bRet == FALSE) {
		db->eStat = (noCreate)?estStop:estErr; 
		db->parent->Update(db->parent->sd, FALSE, db->eStat, NULL);
	}
	LOG("Leave muAssure");
	return bRet;
}

static gboolean muNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter muNext");
	if (!muAssure(db, FALSE))
		return FALSE;
	if (!org_gnome_Muine_Player_next(db->muPlayer, NULL)) {
		LOGERR("Failed to complete Next");
	}
	LOG("Leave muNext");
	return TRUE;
}

static gboolean muPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter muPrevious");
	if (!muAssure(db, FALSE))
		return FALSE;
	if (!org_gnome_Muine_Player_previous(db->muPlayer, NULL)) {
		LOGERR("Failed to complete Previous");
	}
	LOG("Leave muPrevious");
	return TRUE;
}

static gboolean muPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (!muAssure(db, FALSE))
		return FALSE;
	return org_gnome_Muine_Player_set_playing(db->muPlayer, newState, NULL);
}

static gboolean muIsPlaying(gpointer thsPtr) {
	MKTHIS;
	gboolean isPlaying = FALSE;
	if (!muAssure(db, FALSE))
		return FALSE;
	if (!org_gnome_Muine_Player_get_playing
	    (db->muPlayer, &isPlaying, NULL)) {
		LOGERR("Failed to complete get_playing");
		return FALSE;
	}
	return isPlaying;
}

static gboolean muToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean oldState = FALSE;
	if (!muAssure(db, FALSE))
		return FALSE;
	oldState = muIsPlaying(db);
	muPlayPause(db, !oldState);
	if (newState)
		*newState = muIsPlaying(db);
	return TRUE;
}

static gboolean muDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter muDetach");
	if (db->muPlayer) {
		g_object_unref(G_OBJECT(db->muPlayer));
		db->muPlayer = NULL;
	}
	if(db->artLocation)
		g_free(db->artLocation);
	//g_free(db);
	LOG("Leave muDetach");

	return TRUE;
}

static gboolean muIsVisible(gpointer thsPtr) {
	MKTHIS;
	if (muAssure(thsPtr, TRUE)) {
		org_gnome_Muine_Player_get_window_visible(db->muPlayer, &db->Visibility, NULL);
	}
	return db->Visibility;
}

static gboolean muShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (muAssure(thsPtr, FALSE)) {
		return org_gnome_Muine_Player_set_window_visible(db->muPlayer, newState, 0, NULL);
	}
	return FALSE;
}

gpointer MU_attach(SPlayer * parent) {
	muData *db = NULL;

	LOG("Enter MU_attach");
	MU_MAP(Assure);
	MU_MAP(Next);
	MU_MAP(Previous);
	MU_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	MU_MAP(IsPlaying);
	MU_MAP(Toggle);
	MU_MAP(Detach);
	NOMAP(Configure);	// no settings
	NOMAP(Persist);	// no settings
	MU_MAP(IsVisible);
	MU_MAP(Show);
	MU_MAP(UpdateDBUS);
	NOMAP(GetRepeat);
	NOMAP(SetRepeat);
	NOMAP(GetShuffle);
	NOMAP(SetShuffle);
	NOMAP(UpdateWindow);

	db = g_new0(muData, 1);
	db->parent = parent;
	db->artLocation = g_strdup_printf("%s/muine.pic", g_get_tmp_dir());
	LOG("Leave MU_attach");
	return db;
}

#endif
