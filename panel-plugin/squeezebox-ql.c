/***************************************************************************
 *            squeezebox-ql.c
 *
 *  Fri Aug 25 17:08:15 2007
 *  Copyright  2007  Hakan Erduman
 *  Email Hakan.Erduman@web.de
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

#include <thunar-vfs/thunar-vfs.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

// pixmap
#include "squeezebox-ql.png.h"

DEFINE_BACKEND(QL, _("QuodLibet"))
/* --- */
#define QL_MAP(a) parent->a = ql##a;
#define MKTHIS qlData *this = (qlData *)thsPtr;
#define QL_FIFO_PATH "/.quodlibet/control"
#define QL_STAT_PATH "/.quodlibet/current"
#define QL_ALBUM_ART_PATH "/.quodlibet/current.cover"
#define QL_FREE(t)     if(this->t) { \
        g_free(this->t); \
        this->t = NULL; \
    } \

typedef struct qlData {
	SPlayer *parent;
	gpointer *player;
	FILE *fp;
	gchar *fifo;
	gchar *stat;
	gchar *cover;
    gchar *bin;
	ThunarVfsPath *statPath;
	ThunarVfsMonitorHandle *statMon;
	GHashTable *current;
	gboolean isPlaying;
	gboolean isVisible;
	gboolean isShuffle;
	gboolean isRepeat;
	WnckScreen *wnckScreen;
	gint connections[2];
} qlData;

// MFCish property map -- currently none
BEGIN_PROP_MAP(QL)
    END_PROP_MAP()

void *QL_attach(SPlayer * parent);
void qlStatus(gpointer thsPtr, const gchar * args);

void qlCurrentChanged(ThunarVfsMonitor * monitor,
		      ThunarVfsMonitorHandle * handle,
		      ThunarVfsMonitorEvent event,
		      ThunarVfsPath * handle_path,
		      ThunarVfsPath * event_path, gpointer thsPtr) {

	char *fc = NULL;
	const gchar *unknown = _("[Unknown]");
	MKTHIS;
	switch (event) {
	    case THUNAR_VFS_MONITOR_EVENT_CREATED:
	    case THUNAR_VFS_MONITOR_EVENT_CHANGED:
		    LOG("CHANGEDETECT...%s", this->stat);
		    if (g_file_get_contents(this->stat, &fc, NULL, NULL)) {
			    if (!this->current)
				    this->current =
					g_hash_table_new(g_str_hash,
							 g_str_equal);
			    gchar **set = g_strsplit_set(fc, "\n", -1);
			    g_free(fc);

			    LOG("...OK");

			    gchar **ptr = set;
			    do {
				    gchar **line = g_strsplit_set(*ptr, "=", 2);
				    gchar *key = g_strdup(line[0]);
				    gchar *value = g_strdup(line[1]);
				    //LOG(key); LOG("="); LOG(value); LOG("");

				    g_hash_table_replace(this->current, key,
							 value);
				    ptr++;
				    g_strfreev(line);
			    }
			    while (**ptr);
			    g_strfreev(set);

			    gchar *tmpArtist =
				    g_hash_table_lookup(this->current, "artist");
			    gchar *tmpAlbum =
				    g_hash_table_lookup(this->current, "album");
			    gchar *tmpTitle =
				    g_hash_table_lookup(this->current, "title");

			    if (g_file_test(this->cover, G_FILE_TEST_EXISTS))
				    g_string_assign(this->parent->albumArt,
						    this->cover);
			    else
				    g_string_assign(this->parent->albumArt, "");

			    g_string_assign(this->parent->artist,
					    (tmpArtist) ? tmpArtist : unknown);
			    g_string_assign(this->parent->album,
					    (tmpAlbum) ? tmpAlbum : unknown);
			    g_string_assign(this->parent->title,
					    (tmpTitle) ? tmpTitle : unknown);

			    this->parent->Update(this->parent->sd, TRUE,
						 (this->isPlaying) ? estPlay :
						 estPause, NULL);
		    } else {
			    LOG("...KO");
			    g_string_assign(this->parent->artist, "");
			    g_string_assign(this->parent->album, "");
			    g_string_assign(this->parent->title, "");
			    g_string_assign(this->parent->albumArt, "");
			    this->parent->Update(this->parent->sd, FALSE,
						 estStop, NULL);
		    }
		    break;
	    case THUNAR_VFS_MONITOR_EVENT_DELETED:
		    LOG("...fifo has died");
		    g_string_assign(this->parent->artist, "");
		    g_string_assign(this->parent->album, "");
		    g_string_assign(this->parent->title, "");
		    g_string_assign(this->parent->albumArt, "");
		    this->parent->Update(this->parent->sd, FALSE, estStop,
					 NULL);
		    break;
	}
}

gboolean qlAssure(gpointer thsPtr, gboolean noCreate) {
	MKTHIS;
	LOG("Enter qlAssure");
	if (g_file_test(this->fifo, G_FILE_TEST_EXISTS)) {
		if (!this->fp) {
			LOG("Opening %s", this->fifo);
			this->fp = g_fopen(this->fifo, "w");
			LOG("%s", (this->fp) ? " OK" : " KO");

			if (this->fp) {
				GError *err = NULL;
				LOG("enter VFS...");
				this->statPath =
				    thunar_vfs_path_new(this->stat, &err);
				if (!this->statPath) {
					LOG("...VFS KO '%s'", err->message);
				} else {
					LOG("...VFS OK");
					ThunarVfsMonitor *monitor =
					    thunar_vfs_monitor_get_default();

					this->statMon =
					    thunar_vfs_monitor_add_file(monitor,
									this->statPath,
									qlCurrentChanged,
									thsPtr);
					if (!this->statMon) {
						LOG("VFS KO2");
					} else {
						LOG("VFS OK2");
						/* --- this is too late for this->noUpdate
						   thunar_vfs_monitor_feed(
						   monitor,
						   THUNAR_VFS_MONITOR_EVENT_CHANGED,
						   this->statPath);
						   --- we fake it instead.
						 */
						qlCurrentChanged(monitor,
								 NULL,
								 THUNAR_VFS_MONITOR_EVENT_CHANGED,
								 this->statPath,
								 this->statPath,
								 thsPtr);
					}
				}
			}
		}
		LOG("Leave qlAssure OK");
		return TRUE;
	} else if (this->fp) {
		LOG("Anomaly in qlAssure: FIFO disappeared!");
		fclose(this->fp);
		this->fp = NULL;
		g_string_truncate(this->parent->artist, 0);
		g_string_truncate(this->parent->album, 0);
		g_string_truncate(this->parent->title, 0);
		g_string_truncate(this->parent->albumArt, 0);
		this->parent->Update(this->parent->sd, TRUE, estStop, NULL);
	}
	LOG("Leave qlAssure KO");
	return FALSE;
}

gboolean qlPrintFlush(FILE * fp, const char *str) {
	int iLen = strlen(str);
	int iRet = fputs(str, fp);
	fflush(fp);
	return (iLen == iRet);
}

gboolean qlNext(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlNext");
	if (qlAssure(this, FALSE))
		bRet = qlPrintFlush(this->fp, "next");
	else
		bRet = FALSE;
	LOG("Leave qlNext");
	return bRet;
}

gboolean qlPrevious(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlPrevious");
	if (qlAssure(this, FALSE))
		bRet = qlPrintFlush(this->fp, "previous");
	else
		bRet = FALSE;
	LOG("Leave qlPrevious");
	return bRet;
}

gboolean qlPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter qlPlayPause");
	gboolean bRet = FALSE;
	if (qlAssure(this, FALSE))
		bRet = qlPrintFlush(this->fp, "play-pause");
	else {
		LOG("Running...");
		xfce_exec("quodlibet", FALSE, TRUE, NULL);
		bRet = FALSE;
	}
	LOG("LEAVE qlPlayPause");
	return bRet;
}

gboolean qlIsPlaying(gpointer thsPtr) {
	MKTHIS;
	qlStatus(thsPtr, "--status");
	return this->isPlaying;
}

void qlWindowOpened(WnckScreen * screen, WnckWindow * window, gpointer thsPtr) {
	MKTHIS;
	const gchar *windowName = wnck_window_get_name(window);
	if (windowName && windowName[0] == '[')	// minimized but visible
		windowName++;
	if (g_str_has_prefix(windowName, "Quod Libet")) {
		this->isVisible = TRUE;
		LOG("QL:Appeared");
		this->parent->UpdateVisibility(this->parent->sd,
					       this->isVisible);
	}
}

void qlWindowClosed(WnckScreen * screen, WnckWindow * window, gpointer thsPtr) {
	MKTHIS;
	const gchar *windowName = wnck_window_get_name(window);
	if (windowName && windowName[0] == '[')	// minimized but visible
		windowName++;
	if (g_str_has_prefix(windowName, "Quod Libet")) {
		this->isVisible = FALSE;
		LOG("QL:Disappeared");
		this->parent->UpdateVisibility(this->parent->sd,
					       this->isVisible);
	}
}

void qlStatus(gpointer thsPtr, const gchar * args) {
	MKTHIS;
	gchar *outText = NULL;
	const gchar *argv[] = {
		this->bin,
		args,
		NULL
	};
	gint exit_status = 0;
	this->isPlaying = TRUE;
	if (g_spawn_sync(NULL, (gchar **) argv, NULL,
			 G_SPAWN_STDERR_TO_DEV_NULL,
			 NULL, NULL, &outText, NULL, &exit_status, NULL)) {

		LOG("QL says: '%s'", outText);

		// QL generally says things like "paused AlbumList 1.000 inorder off"

		if (strlen(outText)) {
			gchar *ptr = strtok(outText, " ");
			this->isPlaying =
			    !g_ascii_strncasecmp(ptr, "playing", 7);
			this->parent->Update(this->parent->sd, FALSE,
					     (this->isPlaying) ? estPlay :
					     estPause, NULL);
			if ((ptr = strtok(NULL, " "))) {
				//current view
				if ((ptr = strtok(NULL, " "))) {
					//1.000 whatever
					if ((ptr = strtok(NULL, " "))) {
						//shuffle
						this->isShuffle =
						    g_ascii_strncasecmp(ptr,
									"inorder",
									7);
						this->
						    parent->UpdateShuffle(this->
									  parent->sd,
									  this->isShuffle);
						if ((ptr = strtok(NULL, " "))) {
							//repeat
							this->isRepeat =
							    g_ascii_strncasecmp
							    (ptr, "off", 2);
							this->
							    parent->UpdateRepeat
							    (this->parent->sd,
							     this->isRepeat);
						}
					}
				}
			}
		}
		g_free(outText);
	}
}

gboolean qlToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlToggle");
	if (qlAssure(this, FALSE)) {
		bRet = qlPrintFlush(this->fp, "play-pause");
		qlStatus(thsPtr, "--status");
	} else {
		LOG("Running...");
		xfce_exec("quodlibet", FALSE, TRUE, NULL);
		bRet = FALSE;
	}
	LOG("Leave qlToggle");
	return bRet;
}

gboolean qlDetach(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlDetach");
	if (this->fp) {
		fclose(this->fp);
		this->fp = NULL;
	}
	if (this->statPath) {
		thunar_vfs_path_unref(this->statPath);
		this->statPath = NULL;
	}
	if (this->statMon) {
		thunar_vfs_monitor_remove(thunar_vfs_monitor_get_default(),
					  this->statMon);
		this->statMon = NULL;
	}
	if (this->current) {
		g_hash_table_destroy(this->current);
		this->current = NULL;
	}
    
    QL_FREE(fifo);
    QL_FREE(stat);
    QL_FREE(cover);
    QL_FREE(bin);

    thunar_vfs_shutdown();

	if (this->connections[0])
		g_signal_handler_disconnect(this->wnckScreen,
					    this->connections[0]);

	if (this->connections[1])
		g_signal_handler_disconnect(this->wnckScreen,
					    this->connections[1]);

	LOG("Leave qlDetach");
	return bRet;
}

gboolean qlGetRepeat(gpointer thsPtr) {
	MKTHIS;
	qlStatus(thsPtr, "--status");
	return this->isRepeat;
}

gboolean qlSetRepeat(gpointer thsPtr, gboolean newRepeat) {
	MKTHIS;
	LOG("Enter qlSetRepeat");
	gboolean bRet = FALSE;
	if (qlAssure(this, FALSE)) {
		qlStatus(thsPtr, (newRepeat) ? "--repeat=1" : "--repeat=0");
		bRet = TRUE;
	} else {
		bRet = FALSE;
	}
	LOG("LEAVE qlSetRepeat");
	return bRet;
}

gboolean qlGetShuffle(gpointer thsPtr) {
	MKTHIS;
	qlStatus(thsPtr, "--status");
	return this->isShuffle;
}

gboolean qlSetShuffle(gpointer thsPtr, gboolean newShuffle) {
	MKTHIS;
	LOG("Enter qlSetShuffle");
	gboolean bRet = FALSE;
	if (qlAssure(this, FALSE)) {
		qlStatus(thsPtr, (newShuffle) ?
			 "--order=shuffle" : "--order=inorder");
		bRet = TRUE;
	} else {
		bRet = FALSE;
	}
	LOG("LEAVE qlSetShuffle");
	return bRet;
}

gboolean qlIsVisible(gpointer thsPtr) {
	MKTHIS;
	qlStatus(thsPtr, "--status");
	return this->isVisible;
}

gboolean qlShow(gpointer thsPtr, gboolean bShow) {
	MKTHIS;
	LOG("Enter qlShow");
	gboolean bRet = FALSE;
	if (qlAssure(this, FALSE))
		bRet = qlPrintFlush(this->fp, "toggle-window\n");
	else
		bRet = FALSE;
	LOG("Leave qlShow");
	return bRet;
}

void qlPersist(gpointer thsPtr, gboolean bIsStoring) {
	LOG("Enter mpdPersist");
	MKTHIS;
	if(bIsStoring){
		// nothing to do
	} else {
		if (qlAssure(this, FALSE)) {
			qlStatus(thsPtr, "--status");
		}
	}	
	LOG("Leave mpdPersist");
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
	QL_MAP(Persist);
	NOMAP(Configure);
	QL_MAP(IsVisible);
	QL_MAP(Show);
	NOMAP(UpdateDBUS);
	QL_MAP(GetRepeat);
	QL_MAP(SetRepeat);
	QL_MAP(GetShuffle);
	QL_MAP(SetShuffle);

	// we init default values 
	this->parent = parent;
	this->fp = NULL;
	this->fifo = g_strdup_printf("%s%s", homePath, QL_FIFO_PATH);
	this->stat = g_strdup_printf("%s%s", homePath, QL_STAT_PATH);
	this->cover = g_strdup_printf("%s%s", homePath, QL_ALBUM_ART_PATH);
    this->bin = g_find_program_in_path("quodlibet");
    
	thunar_vfs_init();

	// connect to notification events
	int i = 0;
	GList *windows, *l;

	this->wnckScreen =
	    wnck_screen_get(gdk_screen_get_number(gdk_screen_get_default()));

	this->connections[i++] =
	    g_signal_connect(this->wnckScreen, "window_opened",
			     G_CALLBACK(qlWindowOpened), this);

	this->connections[i++] =
	    g_signal_connect(this->wnckScreen, "window_closed",
			     G_CALLBACK(qlWindowClosed), this);

	// update data
	if (qlAssure(this, FALSE)) {
		qlStatus(this, "--status");
	} else {
		this->parent->Update(this->parent->sd, FALSE, estStop, NULL);
	}

	windows = wnck_screen_get_windows(this->wnckScreen);

	for (l = windows; l != NULL; l = l->next) {
		WnckWindow *w = l->data;
		qlWindowOpened(this->wnckScreen, w, this);
	}

	LOG("Leave QL_attach");
	return this;
}
#endif
