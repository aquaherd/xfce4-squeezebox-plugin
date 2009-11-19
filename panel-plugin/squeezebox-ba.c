/***************************************************************************
 *            squeezebox-ba.c
 *
 *  Fri Jan 02 23:00:09 2009
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
#ifdef HAVE_BACKEND_BANSHEE

// default
#include "squeezebox.h"

// libdbus-glib for banshee remote
#include "banshee-player-binding.h"

#ifndef DBUS_TYPE_G_STRING_VALUE_HASHTABLE
#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

#define BA_MAP(a) parent->a = ba##a;
#define BA_CLEAR(t) \
	if (db->t) { \
		g_object_unref(G_OBJECT(db->t)); \
		db->t = NULL; \
	}


// pixmap
#include "squeezebox-ba.png.h"

DEFINE_DBUS_BACKEND(BA, _("Banshee"), "org.bansheeproject.Banshee", "banshee")

typedef struct baData {
	SPlayer *parent;
	DBusGProxy 
		*baPlayerEngine,
		*baPlaybackController,
		*baClientWindow;
	gboolean Visibility;
	gboolean isPlaying;
	eSynoptics eStat;
	gchar* artLocation;
} baData;

#define MKTHIS baData *db = (baData *)thsPtr;
// implementation
static gboolean baAssure(gpointer thsPtr, gboolean noCreate);

const gchar* baFindTag(const gchar* tagName, const gchar* track) {
	gchar *ptr = g_strstr_len(track, -1, tagName);
	if(ptr) {
		ptr += strlen(tagName);
		gchar *ptrStop = g_strstr_len(ptr, -1, "\n");
		ptr = g_strndup(ptr, ptrStop-ptr);
	}		
	return (const gchar*)ptr;
}

static void baDumpProp(gpointer key, gpointer value, gpointer thsPtr) {
	GValue *val = (GValue*)value;
	if(G_VALUE_HOLDS_STRING(val))
		LOG("Property: %s=%s", (gchar*) key, (gchar*)g_value_get_string(val));
}

static const gchar *baGetProp(GHashTable *table, const gchar* propName) {
	gpointer val = NULL;
	if(g_hash_table_lookup_extended(table, propName, NULL, &val) 
	&& val && G_VALUE_HOLDS_STRING((GValue*)val) ) {
		return g_value_get_string((GValue*)val);
	} else {
		return "";
	}
}

static void baCallbackEventChanged(DBusGProxy * proxy, 
	const gchar* text1, const gchar* text2, gdouble dummy, gpointer thsPtr) {
	MKTHIS;
	LOG("Enter baCallback: EventChanged '%s'/'%s'/%lf", text1, text2, dummy);
	if(!g_ascii_strcasecmp(text1, "trackinfoupdated")) {
		GHashTable *table = NULL;
		GError *error = NULL;
		if(org_bansheeproject_Banshee_PlayerEngine_get_currenttrack(
			db->baPlayerEngine, &table, &error) && table) {
			g_hash_table_foreach(table, baDumpProp, thsPtr);
			g_string_assign(db->parent->artist, baGetProp(table, "artist"));
			g_string_assign(db->parent->album, baGetProp(table, "album"));
			g_string_assign(db->parent->title, baGetProp(table, "name"));
			gchar *path = g_filename_from_uri(baGetProp(table, "URI"), NULL, NULL);
			if(path && path[0]) {
				db->parent->FindAlbumArtByFilePath(db->parent->sd, path);
				g_free(path);
			}
			db->parent->Update(db->parent->sd, TRUE, db->eStat, NULL);
		} else {
			LOG("GetCurrentTrack failed '%s'", error->message);
			g_error_free(error);
		}
	}
	LOG("Leave baCallback: EventChanged");
}

static void baCallbackStateChanged(DBusGProxy * proxy, const gchar* newState, gpointer thsPtr) {
	MKTHIS;
	LOG("Enter baCallback: StateChanged '%s'", newState);
	db->eStat = estStop;
	if(!g_ascii_strcasecmp(newState, "paused"))
		db->eStat = estPause;
	else if(!g_ascii_strcasecmp(newState, "playing"))
		db->eStat = estPlay;
	else if(!g_ascii_strcasecmp(newState, "loaded"))
		baCallbackEventChanged(proxy, "trackinfoupdated", "", 0.0, thsPtr);
	db->parent->Update(db->parent->sd, FALSE, db->eStat, NULL);
	LOG("Leave baCallback: StateChanged");
}

static void baCallbackFake(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter baCallback: Fake");
	if(db->baPlayerEngine) {
		GError *error = NULL;
		gchar *state = NULL;
		if(org_bansheeproject_Banshee_PlayerEngine_get_currentstate(db->baPlayerEngine, 
			&state, &error)) {
			LOG("Got property");
			baCallbackStateChanged(db->baPlayerEngine, state, thsPtr);
		} else {
			LOG("Property fail: '%s'", error->message);
			g_error_free(error);
			baCallbackStateChanged(db->baPlayerEngine, "stopped", thsPtr);
		}
		baCallbackEventChanged(db->baPlayerEngine, "trackinfoupdated", "", 0.0, thsPtr);
	}
	LOG("Leave baCallback: Fake");
}

static gboolean baUpdateDBUS(gpointer thsPtr, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("Banshee has started");
		if (!db->baPlayerEngine && baAssure(thsPtr, FALSE))
			baCallbackFake(thsPtr);
	} else {
		LOG("Banshee has died");
		BA_CLEAR(baClientWindow);
		BA_CLEAR(baPlaybackController);
		BA_CLEAR(baPlayerEngine);
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		db->eStat = estStop;
		db->parent->Update(db->parent->sd, TRUE, db->eStat, NULL);
	}
	return TRUE;
}

static gboolean baAssure(gpointer thsPtr, gboolean noCreate) {
	MKTHIS;
	gboolean bRet = TRUE;
	LOG("Enter baAssure");

	if (db->parent->bus && !db->baPlayerEngine) {
		GError *error = NULL;
		db->baPlayerEngine = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							       BA_dbusName(),
							       "/org/bansheeproject/Banshee/PlayerEngine",
							       "org.bansheeproject.Banshee.PlayerEngine",
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
		if (db->baPlayerEngine && !db->baPlaybackController) {
			db->baPlaybackController = dbus_g_proxy_new_from_proxy(db->baPlayerEngine,
								   "org.bansheeproject.Banshee.PlaybackController",
								   "/org/bansheeproject/Banshee/PlaybackController");
		}
		if (db->baPlayerEngine && !db->baClientWindow) {
			db->baClientWindow = dbus_g_proxy_new_from_proxy(db->baPlayerEngine,
								   "org.bansheeproject.Banshee.ClientWindow",
								   "/org/bansheeproject/Banshee/ClientWindow");
		}
		if (db->baPlayerEngine) {
			// state changes
			//  playing change
			dbus_g_proxy_add_signal(db->baPlayerEngine, "StateChanged",
						G_TYPE_STRING,
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->baPlayerEngine, "StateChanged",
						    G_CALLBACK(baCallbackStateChanged),
						    db, NULL);

			//  song change does not seem to work:
			/*  No marshaller for signature of signal 'EventChanged'
			dbus_g_proxy_add_signal(db->baPlayerEngine, "EventChanged",
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_DOUBLE,
						G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->baPlayerEngine, "EventChanged",
						    G_CALLBACK(baCallbackEventChanged), db,
						    NULL);
			*/
		}

	}
	// reflect UI
	if (bRet == FALSE) {
		db->eStat = (noCreate)?estStop:estErr; 
		db->parent->Update(db->parent->sd, FALSE, db->eStat, NULL);
	}
	LOG("Leave baAssure");
	return bRet;
}

static gboolean baNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter baNext");
	if (!baAssure(db, FALSE) || NULL == db->baPlaybackController)
		return FALSE;
	if (!org_bansheeproject_Banshee_PlaybackController_next(db->baPlaybackController, TRUE, NULL)) {
		LOGERR("Failed to complete Next");
	}
	LOG("Leave baNext");
	return TRUE;
}

static gboolean baPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter baPrevious");
	if (!baAssure(db, FALSE) || NULL == db->baPlaybackController)
		return FALSE;
	if (!org_bansheeproject_Banshee_PlaybackController_previous(db->baPlaybackController, TRUE, NULL)) {
		LOGERR("Failed to complete Previous");
	}
	LOG("Leave baPrevious");
	return TRUE;
}

static gboolean baPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (!baAssure(db, FALSE))
		return FALSE;
	if(newState)
		return org_bansheeproject_Banshee_PlayerEngine_play(db->baPlayerEngine, NULL);
	else
		return org_bansheeproject_Banshee_PlayerEngine_pause(db->baPlayerEngine, NULL);
}

static gboolean baIsPlaying(gpointer thsPtr) {
	MKTHIS;
	return db->isPlaying;
}

static gboolean baToggle(gpointer thsPtr, gboolean * newState) {
	
	MKTHIS;
	if (!baAssure(db, FALSE))
		return FALSE;
	org_bansheeproject_Banshee_PlayerEngine_toggle_playing(db->baPlayerEngine, NULL);
	if (newState)
		*newState = baIsPlaying(db);
	return TRUE;
}

static gboolean baDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter baDetach");
	BA_CLEAR(baClientWindow);
	BA_CLEAR(baPlaybackController);
	BA_CLEAR(baPlayerEngine);
	if(db->artLocation)
		g_free(db->artLocation);
	//g_free(db);
	LOG("Leave baDetach");

	return TRUE;
}

gboolean baIsVisible(gpointer thsPtr) {
	MKTHIS;
	return db->Visibility;
}

gboolean baShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (baAssure(thsPtr, FALSE) && NULL != db->baClientWindow) {
		if(newState)
			return org_bansheeproject_Banshee_ClientWindow_present(db->baClientWindow, NULL);
		else
			return org_bansheeproject_Banshee_ClientWindow_hide(db->baClientWindow, NULL);
	}
	return FALSE;
}

void baUpdateWindow(gpointer thsPtr, WnckWindow *window, gboolean appeared) {
	MKTHIS;
	LOG("banshee is %s", (appeared)?"visible":"invisible");
	db->Visibility = appeared;
}

baData *BA_attach(SPlayer * parent) {
	baData *db = NULL;

	LOG("Enter BA_attach");
	BA_MAP(Assure);
	BA_MAP(Next);
	BA_MAP(Previous);
	BA_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	BA_MAP(IsPlaying);
	BA_MAP(Toggle);
	BA_MAP(Detach);
	NOMAP(Configure);	// no settings
	NOMAP(Persist);	// no settings
	BA_MAP(IsVisible);
	BA_MAP(Show);
	BA_MAP(UpdateDBUS);
	NOMAP(GetRepeat);
	NOMAP(SetRepeat);
	NOMAP(GetShuffle);
	NOMAP(SetShuffle);
	BA_MAP(UpdateWindow);

	db = g_new0(baData, 1);
	db->parent = parent;
	db->artLocation = g_strdup_printf("%s/banshee.pic", g_get_tmp_dir());
	LOG("Leave BA_attach");
	return db;
}

#endif
