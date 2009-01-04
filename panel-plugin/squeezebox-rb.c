/***************************************************************************
 *            rythmbox-rb.c
 *
 *  Fri Aug 25 17:20:09 2006
 *  Copyright  2006  Hakan Erduman
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
#ifdef HAVE_BACKEND_RHYTHMBOX

// default
#include "squeezebox.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
//#include <id3tag.h>

#include "rb-shell-binding.h"
#include "rb-shell-player-binding.h"

/*
#ifndef DBUS_TYPE_G_STRING_VALUE_HASHTABLE
#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif
*/
#define RB_MAP(a) parent->a = rb##a;

// pixmap
#include "squeezebox-rb.png.h"

DEFINE_DBUS_BACKEND(RB, _("Rhythmbox"), "org.gnome.Rhythmbox", "ryhthmbox")

typedef struct rbData{
	SPlayer *parent;
	DBusGProxy *rbPlayer;
	DBusGProxy *rbShell;
	gboolean Visibility;
} rbData;

// MFCish property map -- currently none
BEGIN_PROP_MAP(RB)
    END_PROP_MAP()
#define MKTHIS rbData *db = (rbData *)thsPtr;
gboolean rbAssure(gpointer thsPtr, gboolean noCreate);

static void rbCallbackPlayPause(DBusGProxy * proxy, const gboolean playing,
				gpointer thsPtr) {
	MKTHIS;
	LOG("Enter rbCallback: StateChanged %d", playing);
	eSynoptics eStat;
	if (playing)
		eStat = estPlay;
	else
		eStat = estPause;
	db->parent->Update(db->parent->sd, FALSE, eStat, NULL);
	LOG("Leave rbCallback: StateChanged");
}

static void rbCallbackVisibility(DBusGProxy * proxy, const gboolean visible,
				 gpointer thsPtr) {
	MKTHIS;
	LOG("rbCallback: Visibility %d", visible);
	db->Visibility = visible;
	db->parent->UpdateVisibility(db->parent->sd, visible);
}

static void 
rbCallback(DBusGProxy * proxy, const gchar * uri, gpointer thsPtr) {
	MKTHIS;
	LOG("rbCallback: SongChanged '%s'", uri);

	if (db->rbShell) {
		GHashTable *table = NULL;
		GError *err = NULL;
		if (org_gnome_Rhythmbox_Shell_get_song_properties
		    (db->rbShell, uri, &table, &err)) {
			GValue *tmpArtist = g_hash_table_lookup(table, "artist");
			GValue *tmpAlbum = g_hash_table_lookup(table, "album");
			GValue *tmpTitle = g_hash_table_lookup(table, "title");

			GString *artLocation = g_string_new("");

			g_string_assign(db->parent->artist,
					g_value_get_string(tmpArtist));
			g_string_assign(db->parent->album,
					g_value_get_string(tmpAlbum));
			g_string_assign(db->parent->title,
					g_value_get_string(tmpTitle));
			g_string_truncate(db->parent->albumArt, 0);
			g_string_printf(artLocation,
					"%s/.cache/rhythmbox/covers/%s - %s.jpg",
					g_get_home_dir(),
					db->parent->artist->str,
					db->parent->album->str);
			LOG("Check 1: '%s'", artLocation->str);

			gboolean bFound = FALSE;
			if (g_file_test(artLocation->str, G_FILE_TEST_EXISTS)) {
				bFound = TRUE;
			} else {
				//rb 0.11 can read folder.jpg and so should we
				gchar *str =
				    g_filename_from_uri(uri, NULL, NULL);
				if (str && str[0])
					db->parent->FindAlbumArtByFilePath(db->parent->sd, str);
				g_free(str);
			}
			if (bFound) {
				// just assign here, scaling is done in callee
				g_string_assign(db->parent->albumArt, artLocation->str);
				LOG("Found :'%s'", artLocation->str);
			}

			g_hash_table_destroy(table);
			g_string_free(artLocation, TRUE);

			db->parent->Update(db->parent->sd, TRUE, estPlay, NULL);
		}
		if (err) {
			fprintf(stderr, "Unable to get Properties: '%s'",
				err->message);
			g_error_free(err);
		}
	}

}

gboolean rbIsPlaying(gpointer thsPtr) {
	MKTHIS;
	gboolean bRes = FALSE;
	if (!rbAssure(db, TRUE))
		return FALSE;
	if (!dbus_g_proxy_call(db->rbPlayer, "getPlaying", NULL,
			       G_TYPE_INVALID,
			       G_TYPE_BOOLEAN, &bRes, G_TYPE_INVALID)) {
		LOGERR("Failed to complete getPlaying");
		return FALSE;
	}
	return bRes;
}

gboolean rbIsVisible(gpointer thsPtr) {
	MKTHIS;
	return db->Visibility;
}

static void rbCallbackFake(gpointer thsPtr) {
	MKTHIS;
	char *uri = NULL;
	rbCallbackPlayPause(db->rbPlayer, rbIsPlaying(db), db);

	if (org_gnome_Rhythmbox_Player_get_playing_uri(db->rbPlayer, &uri, NULL)
	    && uri) {
		LOG("!!!");
		rbCallback(db->rbPlayer, uri, db);
	}
	rbCallbackVisibility(db->rbPlayer, rbIsVisible(db), db);
}

static gboolean rbUpdateDBUS(gpointer thsPtr, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("Rhythmbox has started");
		if (!db->rbPlayer && rbAssure(thsPtr, TRUE))
			rbCallbackFake(thsPtr);
	} else {
		LOG("Rhythmbox has died");
		if (db->rbPlayer) {
			g_object_unref(G_OBJECT(db->rbPlayer));
			db->rbPlayer = NULL;
		}
		if (db->rbShell) {
			g_object_unref(G_OBJECT(db->rbShell));
			db->rbShell = NULL;
		}
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		db->parent->Update(db->parent->sd, TRUE, estStop, NULL);
	}
	return TRUE;
}

gboolean rbAssure(gpointer thsPtr, gboolean noCreate) {
	gboolean bRet = TRUE;
	MKTHIS;
	LOG("Enter rbAssure");
	if (db->parent->bus && !db->rbShell) {
		GError *error = NULL;
		db->rbShell = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							      RB_dbusName(),
							      "/org/gnome/Rhythmbox/Shell",
							      "org.gnome.Rhythmbox.Shell",
							      &error);
		if (error) {
			LOGWARN("Couldn't connect to DBUS proxy '%s' ",
				error->message);
			g_error_free(error);
			error = NULL;
			if (noCreate)
				bRet = FALSE;
			else {
				bRet = db->parent->StartService(db->parent->sd);
			}
		}
		if (db->rbShell && !db->rbPlayer) {
			db->rbPlayer = dbus_g_proxy_new_from_proxy(db->rbShell,
								   "org.gnome.Rhythmbox.Player",
								   "/org/gnome/Rhythmbox/Player");
			if (!db->rbPlayer) {
				g_object_unref(G_OBJECT(db->rbShell));
				db->rbShell = NULL;
				db->rbPlayer = NULL;
				LOGERR("Couldn't connect to player proxy");
				bRet = FALSE;
			} else {
				// state changes
				//  playing change
				dbus_g_proxy_add_signal(db->rbPlayer,
							"playingChanged",
							G_TYPE_BOOLEAN,
							G_TYPE_INVALID);
				dbus_g_proxy_connect_signal(db->rbPlayer,
							    "playingChanged",
							    G_CALLBACK
							    (rbCallbackPlayPause),
							    db, NULL);

				//  song change
				dbus_g_proxy_add_signal(db->rbPlayer,
							"playingUriChanged",
							G_TYPE_STRING,
							G_TYPE_INVALID);
				dbus_g_proxy_connect_signal(db->rbPlayer,
							    "playingUriChanged",
							    G_CALLBACK
							    (rbCallback), db,
							    NULL);

				//  player change
				dbus_g_proxy_add_signal(db->rbShell,
							"visibilityChanged",
							G_TYPE_BOOLEAN,
							G_TYPE_INVALID);
				dbus_g_proxy_connect_signal(db->rbShell,
							    "visibilityChanged",
							    G_CALLBACK
							    (rbCallbackVisibility),
							    db, NULL);
			}
		}
	}
	// reflect UI
	if (bRet == FALSE) {
		db->parent->Update(db->parent->sd, FALSE,
				   (noCreate)?estStop:estErr, NULL);
	}
	LOG("Leave rbAssure");
	return bRet;
}

gboolean rbNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter rbNext");
	if (!rbAssure(db, TRUE))
		return FALSE;
	if (!dbus_g_proxy_call(db->rbPlayer, "next", NULL,
			       G_TYPE_INVALID, G_TYPE_INVALID)) {
		LOGERR("Failed to complete Next");
		return FALSE;
	}
	LOG("Leave rbNext");
	return TRUE;
}

gboolean rbPrevious(gpointer thsPtr) {
	MKTHIS;
	if (!rbAssure(db, TRUE))
		return FALSE;
	if (!dbus_g_proxy_call(db->rbPlayer, "previous", NULL,
			       G_TYPE_INVALID, G_TYPE_INVALID)) {
		LOGERR("Failed to complete Prev");
		return FALSE;
	}
	return TRUE;
}

gboolean rbPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (!rbAssure(db, FALSE))
		return FALSE;
	if (!dbus_g_proxy_call(db->rbPlayer, "playPause", NULL,
			       G_TYPE_BOOLEAN, newState, G_TYPE_INVALID,
			       G_TYPE_INVALID)) {
		LOGERR("Failed to complete playPause");
		return FALSE;
	}
	return TRUE;
}

gboolean rbToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean oldState = FALSE;
	if (!rbAssure(db, FALSE))
		return FALSE;
	oldState = rbIsPlaying(db);
	if (!rbPlayPause(db, !oldState))
		return FALSE;
	if (newState)
		*newState = !oldState;

	return TRUE;
}

gboolean rbDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter rbDetach");
	if (db->rbPlayer) {
		g_object_unref(G_OBJECT(db->rbPlayer));
		db->rbPlayer = NULL;
	}
	if (db->rbShell) {
		g_object_unref(G_OBJECT(db->rbShell));
		db->rbShell = NULL;
	}
	//g_free(db);
	LOG("Leave rbDetach");

	return TRUE;
}

gboolean rbShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (rbAssure(thsPtr, FALSE)) {
		org_gnome_Rhythmbox_Shell_present(db->rbPlayer,
						  (newState) ? 1 : 0, NULL);
		return TRUE;
	}
	return FALSE;
}

/*
gboolean rbGetRepeat(gpointer thsPtr) {
	MKTHIS;
	org_gnome_Rhythmbox_Shell_get_playlist_manager(
}
*/
rbData *RB_attach(SPlayer * parent) {
	rbData *db = NULL;

	LOG("Enter RB_attach");
	RB_MAP(Assure);
	RB_MAP(Next);
	RB_MAP(Previous);
	RB_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	RB_MAP(IsPlaying);
	RB_MAP(Toggle);
	NOMAP(Configure);	// no settings
	RB_MAP(Detach);
	NOMAP(Persist);
	RB_MAP(IsVisible);
	RB_MAP(Show);
	RB_MAP(UpdateDBUS);
	//The DBUS API does not provide:
	NOMAP(GetRepeat);
	NOMAP(SetRepeat);
	NOMAP(GetShuffle);
	NOMAP(SetShuffle);
	NOMAP(UpdateWindow);

	db = g_new0(rbData, 1);
	db->parent = parent;
	db->rbPlayer = NULL;

	LOG("Leave RB_attach");
	return db;
}
#endif
