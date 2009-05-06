/***************************************************************************
 *            squeezebox-bmp.c
 *
 *  Sat Jan 01 16:36:09 2009
 *  Copyright  2009 by Hakan Erduman
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
#ifdef HAVE_BACKEND_BMPX

// default
#include "squeezebox.h"

// libdbus-glib for beep remote
#include <dbus/dbus-glib.h>
#include "bmpx-player-binding.h"

#ifndef DBUS_TYPE_G_STRING_VALUE_HASHTABLE
#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

#define BMP_MAP(a) parent->a = bmp##a;

// pixmap
#include "squeezebox-bmp.png.h"

DEFINE_DBUS_BACKEND(BMP, _("Beep Media Player"), "org.mpris.bmp", "bmpx")

typedef struct bmpData {
	SPlayer *parent;
	DBusGProxy *bmpPlayer;
	DBusGProxy *bmpNative;
	gboolean Visibility;
	gboolean Shuffle;
	gboolean Repeat;
	eSynoptics eStat;
} bmpData;

// MFCish property map -- currently none
BEGIN_PROP_MAP(BMP)
    END_PROP_MAP()
#define MKTHIS bmpData *db = (bmpData *)thsPtr;
// implementation
static gboolean bmpAssure(gpointer thsPtr, gboolean noCreate);

static void bmpCallbackCapsChange(DBusGProxy * proxy, gint caps, gpointer thsPtr) {
	// MKTHIS;
	LOG("Enter bmpCallback: CapsChange %d", caps);
	LOG("Leave bmpCallback: CapsChange");
}

static eSynoptics bmpTranslateStatus(gint bmpStatus) {
	eSynoptics eStat;
	switch (bmpStatus) {
	    case -1: // this is wrong.
	    case 0:
		    eStat = estPlay;
		    break;
	    case 1:
		    eStat = estPause;
		    break;
	    case 2:
		    eStat = estStop;
		    break;
	    default:
		    eStat = estErr;
		    break;
	}
	return eStat;
}

static void bmpCallbackStatusChange(DBusGProxy * proxy, gint status,
				   gpointer thsPtr) {
	MKTHIS;
	LOG("Enter bmpCallback: StatusChange %d", status);
	db->eStat = bmpTranslateStatus(status);
	db->parent->Update(db->parent->sd, FALSE, db->eStat, NULL);
	LOG("Leave bmpCallback: StatusChange");
}

static void bmpCallbackTrackChange(DBusGProxy * proxy, GHashTable * table,
				  gpointer thsPtr) {
	MKTHIS;
	LOG("Enter bmpCallback: TrackChange");
	GValue *tmpArtist = g_hash_table_lookup(table, "artist");
	GValue *tmpAlbum = g_hash_table_lookup(table, "album");
	GValue *tmpTitle = g_hash_table_lookup(table, "title");
	GValue *tmpURI = g_hash_table_lookup(table, "location");

	g_string_assign(db->parent->artist, g_value_get_string(tmpArtist));
	g_string_assign(db->parent->album, g_value_get_string(tmpAlbum));
	g_string_assign(db->parent->title, g_value_get_string(tmpTitle));
	
	gchar *str = g_filename_from_uri(g_value_get_string(tmpURI), NULL, NULL);
	if (str && str[0])
		db->parent->FindAlbumArtByFilePath(db->parent->sd, str);
	db->parent->Update(db->parent->sd, TRUE, db->eStat, NULL);
	LOG("Leave bmpCallback: TrackChange");
}

static void bmpCallbackFake(gpointer thsPtr) {
	MKTHIS;
	// emulate state change
	gint caps = 0;
	gint status = 0;
	GHashTable *metaData = NULL;
	if (org_freedesktop_MediaPlayer_get_caps(db->bmpPlayer, &caps, NULL)) {
		bmpCallbackCapsChange(db->bmpPlayer, caps, db);
	}
	if (org_freedesktop_MediaPlayer_get_status(db->bmpPlayer, &status, NULL)) {
		bmpCallbackStatusChange(db->bmpPlayer, status, db);
	}
	if (org_freedesktop_MediaPlayer_get_metadata
	    (db->bmpPlayer, &metaData, NULL)) {
		bmpCallbackTrackChange(db->bmpPlayer, metaData, db);
		g_hash_table_unref(metaData);
	}
}

static gboolean bmpUpdateDBUS(gpointer thsPtr, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("bmpx has started");
		if (!db->bmpPlayer && bmpAssure(thsPtr, FALSE))
			bmpCallbackFake(thsPtr);
	} else {
		LOG("bmpx has died");
		if (db->bmpPlayer) {
			g_object_unref(G_OBJECT(db->bmpPlayer));
			db->bmpPlayer = NULL;
		}
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		db->parent->Update(db->parent->sd, TRUE, estStop, NULL);
	}
	return TRUE;
}

static gboolean bmpAssure(gpointer thsPtr, gboolean noCreate) {
	gboolean bRet = TRUE;
	bmpData *db = (bmpData *) thsPtr;
	LOG("Enter bmpAssure");

	if (db->parent->bus && !db->bmpPlayer) {
		GError *error = NULL;
		db->bmpPlayer = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							       BMP_dbusName(),
							       "/Player",
							       "org.freedesktop.MediaPlayer",
							       &error);
		if (error) {
			LOGWARN("\tCouldn't connect to shell proxy '%s' ",
				error->message);
			g_error_free(error);
			error = NULL;
			if (noCreate)
				bRet = FALSE;
			else {
				bRet = db->parent->StartService(db->parent->sd);
			}
		}
		if (db->bmpPlayer) {
			// state changes
			//  playing change
			dbus_g_proxy_add_signal(db->bmpPlayer, "StatusChange",
						G_TYPE_INT, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->bmpPlayer,
						    "StatusChange",
						    G_CALLBACK
						    (bmpCallbackStatusChange),
						    db, NULL);

			//  song change
			dbus_g_proxy_add_signal(db->bmpPlayer, "TrackChange",
						DBUS_TYPE_G_STRING_VALUE_HASHTABLE,
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->bmpPlayer, "TrackChange",
						    G_CALLBACK
						    (bmpCallbackTrackChange), db,
						    NULL);

			//  player change
			dbus_g_proxy_add_signal(db->bmpPlayer, "CapsChange",
						G_TYPE_INT, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->bmpPlayer, "CapsChange",
						    G_CALLBACK
						    (bmpCallbackCapsChange), db,
						    NULL);

			// extended properties
			if (!db->bmpNative) {
				db->bmpNative =
				    dbus_g_proxy_new_for_name_owner(db->
								    parent->bus,
								    "org.mpris.bmp",
								    "/BMP",
								    "org.beepmediaplayer.bmp",
								    NULL);
			}
		}

	}
	// reflect UI
	if (bRet == FALSE) {
		db->parent->Update(db->parent->sd, FALSE,
				   (noCreate)?estStop:estErr, NULL);
	}
	LOG("Leave bmpAssure");
	return bRet;
}

static gboolean bmpNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter bmpNext");
	if (!bmpAssure(db, FALSE))
		return FALSE;
	if (!org_freedesktop_MediaPlayer_next(db->bmpPlayer, NULL)) {
		LOGERR("Failed to complete Next");
	}
	LOG("Leave bmpNext");
	return TRUE;
}

static gboolean bmpPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter bmpPrevious");
	if (!bmpAssure(db, FALSE))
		return FALSE;
	if (!org_freedesktop_MediaPlayer_prev(db->bmpPlayer, NULL)) {
		LOGERR("Failed to complete Previous");
	}
	LOG("Leave bmpPrevious");
	return TRUE;
}

static gboolean bmpPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter bmpPlayPause %d", newState);
	if (!bmpAssure(db, FALSE))
		return FALSE;
	if (newState)
		org_freedesktop_MediaPlayer_play(db->bmpPlayer, NULL);
	else
		org_freedesktop_MediaPlayer_pause(db->bmpPlayer, NULL);
	LOG("Leave bmpPlayPause");
	return FALSE;
}

static gboolean bmpIsPlaying(gpointer thsPtr) {
	MKTHIS;
	if (!bmpAssure(db, FALSE))
		return FALSE;
	return (db->eStat == estPlay);
}

static gboolean bmpToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean oldState = FALSE;
	LOG("Enter bmpToggle");
	if (!bmpAssure(db, FALSE))
		return FALSE;
	oldState = (estPlay == db->eStat);
	bmpPlayPause(db, !oldState);
	if (newState)
		*newState = (estPlay == db->eStat);
	LOG("Leave bmpToggle");
	return TRUE;
}

static gboolean bmpDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter bmpDetach");
	if (db->bmpPlayer) {
		g_object_unref(G_OBJECT(db->bmpPlayer));
		db->bmpPlayer = NULL;
	}
	if(db->bmpNative) {
		g_object_unref(G_OBJECT(db->bmpNative));
		db->bmpNative = NULL;
	}
	//g_free(db);
	LOG("Leave bmpDetach");

	return TRUE;
}

gboolean bmpShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (bmpAssure(thsPtr, FALSE) && NULL != db->bmpNative) {
		org_beepmediaplayer_bmp_ui_raise(db->bmpNative, NULL);
		return TRUE;
	}
	return FALSE;
}

bmpData *BMP_attach(SPlayer * parent) {
	bmpData *db = NULL;

	LOG("Enter BMP_attach");
	BMP_MAP(Assure);
	BMP_MAP(Next);
	BMP_MAP(Previous);
	BMP_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	BMP_MAP(IsPlaying);
	BMP_MAP(Toggle);
	BMP_MAP(Detach);
	NOMAP(Configure);	// no settings
	NOMAP(Persist);	// no settings
	NOMAP(IsVisible);
	BMP_MAP(Show);
	BMP_MAP(UpdateDBUS);
	NOMAP(GetRepeat);
	NOMAP(SetRepeat);
	NOMAP(GetShuffle);
	NOMAP(SetShuffle);
	NOMAP(UpdateWindow);

	db = g_new0(bmpData, 1);
	db->parent = parent;
	db->bmpPlayer = NULL;
	db->bmpNative = NULL;
	LOG("Leave BMP_attach");
	return db;
}

#endif
