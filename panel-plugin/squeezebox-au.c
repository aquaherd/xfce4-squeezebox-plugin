/***************************************************************************
 *            rythmbox-au.c
 *
 *  Sat Nov 29 23:00:09 2008
 *  Copyright  2008 by Hakan Erduman
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
#ifdef HAVE_BACKEND_AUDACIOUS

// default
#include "squeezebox.h"

// libdbus-glib for audacious remote
#include <dbus/dbus-glib.h>
#include "audacious-player-binding.h"

#ifndef DBUS_TYPE_G_STRING_VALUE_HASHTABLE
#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

#define AU_MAP(a) parent->a = au##a;

// pixmap
#include "squeezebox-au.png.h"

DEFINE_DBUS_BACKEND(AU, _("audacious 1.5.x"), "org.mpris.audacious", "audacious")

typedef struct auData {
	SPlayer *parent;
	DBusGProxy *auPlayer;
	DBusGProxy *auTheme;
	gboolean Visibility;
	gboolean Shuffle;
	gboolean Repeat;
} auData;

// MFCish property map -- currently none
BEGIN_PROP_MAP(AU)
    END_PROP_MAP()
#define MKTHIS auData *db = (auData *)thsPtr;
// implementation
static gboolean auAssure(gpointer thsPtr, gboolean noCreate);

static void auCallbackCapsChange(DBusGProxy * proxy, gint caps, gpointer thsPtr) {
	// MKTHIS;
	LOG("Enter auCallback: CapsChange %d", caps);
	LOG("Leave auCallback: CapsChange");
}

static eSynoptics auTranslateStatus(gint auStatus) {
	eSynoptics eStat;
	switch (auStatus) {
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

static void auCallbackStatusChange(DBusGProxy * proxy, gint status,
				   gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auCallback: StatusChange %d", status);
	eSynoptics eStat = auTranslateStatus(status);
	db->parent->Update(db->parent->sd, FALSE, eStat, NULL);
	LOG("Leave auCallback: StatusChange");
}

static void auCallbackTrackChange(DBusGProxy * proxy, GHashTable * table,
				  gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auCallback: TrackChange");
	GValue *tmpArtist = g_hash_table_lookup(table, "artist");
	GValue *tmpAlbum = g_hash_table_lookup(table, "album");
	GValue *tmpTitle = g_hash_table_lookup(table, "title");
	GValue *tmpURI = g_hash_table_lookup(table, "URI");

	g_string_assign(db->parent->artist, g_value_get_string(tmpArtist));
	g_string_assign(db->parent->album, g_value_get_string(tmpAlbum));
	g_string_assign(db->parent->title, g_value_get_string(tmpTitle));
	
	gchar *str = g_filename_from_uri(g_value_get_string(tmpURI), NULL, NULL);
	if (str && str[0])
		db->parent->FindAlbumArtByFilePath(db->parent->sd, str);
	eSynoptics eStat = estErr;
	gint iStat = 0;
	if (org_freedesktop_MediaPlayer_get_status(db->auPlayer, &iStat, NULL)) {
		eStat = auTranslateStatus(iStat);
	}
	db->parent->Update(db->parent->sd, TRUE, eStat, NULL);
	LOG("Leave auCallback: TrackChange");
}

static void auCallbackFake(gpointer thsPtr) {
	MKTHIS;
	// emulate state change
	gint caps = 0;
	gint status = 0;
	GHashTable *metaData = NULL;
	if (org_freedesktop_MediaPlayer_get_caps(db->auPlayer, &caps, NULL)) {
		auCallbackCapsChange(db->auPlayer, caps, db);
	}
	if (org_freedesktop_MediaPlayer_get_status(db->auPlayer, &status, NULL)) {
		auCallbackStatusChange(db->auPlayer, status, db);
	}
	if (org_freedesktop_MediaPlayer_get_metadata
	    (db->auPlayer, &metaData, NULL)) {
		auCallbackTrackChange(db->auPlayer, metaData, db);
		g_hash_table_unref(metaData);
	}
}

static gboolean auUpdateDBUS(gpointer thsPtr, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("Audacious has started");
		if (!db->auPlayer && auAssure(thsPtr, FALSE))
			auCallbackFake(thsPtr);
	} else {
		LOG("Audacious has died");
		if (db->auPlayer) {
			g_object_unref(G_OBJECT(db->auPlayer));
			db->auPlayer = NULL;
		}
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		db->parent->Update(db->parent->sd, TRUE, estStop, NULL);
	}
	return TRUE;
}

static gboolean auAssure(gpointer thsPtr, gboolean noCreate) {
	gboolean bRet = TRUE;
	auData *db = (auData *) thsPtr;
	LOG("Enter auAssure");

	if (db->parent->bus && !db->auPlayer) {
		GError *error = NULL;
		db->auPlayer = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							       AU_dbusName(),
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
		if (db->auPlayer) {
			// state changes
			//  playing change
			dbus_g_proxy_add_signal(db->auPlayer, "StatusChange",
						G_TYPE_INT, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->auPlayer,
						    "StatusChange",
						    G_CALLBACK
						    (auCallbackStatusChange),
						    db, NULL);

			//  song change
			dbus_g_proxy_add_signal(db->auPlayer, "TrackChange",
						DBUS_TYPE_G_STRING_VALUE_HASHTABLE,
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->auPlayer, "TrackChange",
						    G_CALLBACK
						    (auCallbackTrackChange), db,
						    NULL);

			//  player change
			dbus_g_proxy_add_signal(db->auPlayer, "CapsChange",
						G_TYPE_INT, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->auPlayer, "CapsChange",
						    G_CALLBACK
						    (auCallbackCapsChange), db,
						    NULL);

			// extended properties
			if (!db->auTheme) {
				db->auTheme =
				    dbus_g_proxy_new_for_name_owner(db->
								    parent->bus,
								    "org.atheme.audacious",
								    "/org/atheme/audacious",
								    "org.atheme.audacious",
								    NULL);
			}
		}

	}
	// reflect UI
	if (bRet == FALSE) {
		db->parent->Update(db->parent->sd, FALSE,
				   (noCreate)?estStop:estErr, NULL);
	}
	LOG("Leave auAssure");
	return bRet;
}

static gboolean auNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auNext");
	if (!auAssure(db, FALSE))
		return FALSE;
	if (!org_freedesktop_MediaPlayer_next(db->auPlayer, NULL)) {
		LOGERR("Failed to complete Next");
	}
	LOG("Leave auNext");
	return TRUE;
}

static gboolean auPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auPrevious");
	if (!auAssure(db, FALSE))
		return FALSE;
	if (!org_freedesktop_MediaPlayer_prev(db->auPlayer, NULL)) {
		LOGERR("Failed to complete Previous");
	}
	LOG("Leave auPrevious");
	return TRUE;
}

static gboolean auPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (!auAssure(db, FALSE))
		return FALSE;
	if (newState)
		org_freedesktop_MediaPlayer_play(db->auPlayer, NULL);
	else
		org_freedesktop_MediaPlayer_pause(db->auPlayer, NULL);
	return FALSE;
}

static gboolean auIsPlaying(gpointer thsPtr) {
	MKTHIS;
	gint status = 0;
	if (!auAssure(db, FALSE))
		return FALSE;
	if (!org_freedesktop_MediaPlayer_get_status
	    (db->auPlayer, &status, NULL)) {
		LOGERR("Failed to complete get_status");
		return FALSE;
	}
	return (status == 0);
}

static gboolean auToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean oldState = FALSE;
	if (!auAssure(db, FALSE))
		return FALSE;
	oldState = auIsPlaying(db);
	auPlayPause(db, !oldState);
	if (newState)
		*newState = auIsPlaying(db);
	return TRUE;
}

static gboolean auDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auDetach");
	if (db->auPlayer) {
		g_object_unref(G_OBJECT(db->auPlayer));
		db->auPlayer = NULL;
	}
	//g_free(db);
	LOG("Leave auDetach");

	return TRUE;
}

gboolean auIsVisible(gpointer thsPtr) {
	MKTHIS;
	if (auAssure(thsPtr, FALSE) && NULL != db->auTheme) {
		org_atheme_audacious_main_win_visible(db->auTheme,
						      &db->Visibility, NULL);
	}
	return db->Visibility;
}

gboolean auShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (auAssure(thsPtr, FALSE) && NULL != db->auTheme) {
		org_atheme_audacious_show_main_win(db->auTheme,
						   (newState) ? 1 : 0, NULL);
		return TRUE;
	}
	return FALSE;
}

gboolean auGetShuffle(gpointer thsPtr) {
	MKTHIS;
	if (db->auTheme) 
		org_atheme_audacious_shuffle(db->auTheme, &db->Shuffle, NULL);
	else
        db->Shuffle = FALSE;
	return db->Shuffle;
}

gboolean auSetShuffle(gpointer thsPtr, gboolean newShuffle) {
	MKTHIS;
	if (auAssure(thsPtr, FALSE) && NULL != db->auTheme) {
		org_atheme_audacious_toggle_shuffle(db->auTheme, NULL);
	}
	return TRUE;
}

gboolean auGetRepeat(gpointer thsPtr) {
	MKTHIS;
	if (db->auTheme)
		org_atheme_audacious_repeat(db->auTheme, &db->Repeat, NULL);
	else
        db->Repeat = FALSE;
	return db->Repeat;
}

gboolean auSetRepeat(gpointer thsPtr, gboolean newRepeat) {
	MKTHIS;
	if (auAssure(thsPtr, FALSE) && NULL != db->auTheme) {
		return org_atheme_audacious_toggle_repeat(db->auTheme, NULL);
	}
	return FALSE;
}

auData *AU_attach(SPlayer * parent) {
	auData *db = NULL;

	LOG("Enter AU_attach");
	AU_MAP(Assure);
	AU_MAP(Next);
	AU_MAP(Previous);
	AU_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	AU_MAP(IsPlaying);
	AU_MAP(Toggle);
	AU_MAP(Detach);
	NOMAP(Configure);	// no settings
	NOMAP(Persist);	// no settings
	AU_MAP(IsVisible);
	AU_MAP(Show);
	AU_MAP(UpdateDBUS);
	AU_MAP(GetRepeat);
	AU_MAP(SetRepeat);
	AU_MAP(GetShuffle);
	AU_MAP(SetShuffle);

	db = g_new0(auData, 1);
	db->parent = parent;
	db->auPlayer = NULL;
	db->auTheme = NULL;
	LOG("Leave AU_attach");
	return db;
}

#endif
