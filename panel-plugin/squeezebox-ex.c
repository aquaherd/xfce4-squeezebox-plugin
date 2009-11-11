/***************************************************************************
 *            squeezebox-ex.c
 *
 *  Sat Nov 30 22:11:23 2008
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
#ifdef HAVE_BACKEND_EXAILE

// default
#include "squeezebox.h"

// libdbus-glib for exaile remote
#include "exaile-player-binding.h"

#ifndef DBUS_TYPE_G_STRING_VALUE_HASHTABLE
#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

#define EX_MAP(a) parent->a = ex##a;

// pixmap
#include "squeezebox-ex.png.h"

DEFINE_DBUS_BACKEND(EX, _("Exaile"), "org.exaile.DBusInterface", "exaile")

typedef struct exData{
	SPlayer *parent;
	DBusGProxy *exPlayer;
	gboolean Visibility;
	gboolean isPlaying;
	gboolean needsPolling;
	gint intervalID;
	gchar *lastTrack;
	gchar *lastStatus;
} exData;

#define MKTHIS exData *db = (exData *)thsPtr;
// init Quarks
GQuark stopped = 0;
GQuark paused = 0;
GQuark playing = 0;

// MFCish property map -- currently none
BEGIN_PROP_MAP(EX)
    END_PROP_MAP()
// implementation

static eSynoptics exTranslateStatus(gchar * exStatus) {

	GQuark newStat = g_quark_from_string(exStatus);
	if (newStat == playing)
		return estPlay;
	else if (newStat == paused)
		return estPause;
	if (newStat == stopped)
		return estStop;

	return estErr;
}

static void exCallbackStatusChange(DBusGProxy * proxy, gpointer thsPtr) {
	MKTHIS;
	eSynoptics eStat = estErr;
	LOG("Enter auCallback: StatusChange");
	gchar *status = NULL;
	if (org_exaile_DBusInterface_status(db->exPlayer, &status, NULL)) {
		eStat = exTranslateStatus(status);
		g_free(status);
	}
	db->parent->Update(db->parent->sd, FALSE, eStat, NULL);
	LOG("Leave auCallback: StatusChange");
}

static void exCallbackTrackChange(DBusGProxy * proxy, gpointer thsPtr) {
	MKTHIS;
	gchar *artist = NULL, *album = NULL, *title = NULL, *cover = NULL;
	gboolean act = FALSE;
	gchar *status = NULL;

	LOG("Enter auCallback: TrackChange");
	eSynoptics eStat = estErr;
	if (org_exaile_DBusInterface_status(db->exPlayer, &status, NULL)) {
		eStat = exTranslateStatus(status);
		g_free(status);
	}
	switch (eStat) {
	    case estPlay:
	    case estPause:
		    if (org_exaile_DBusInterface_get_artist
			(db->exPlayer, &artist, NULL)) {
			    g_string_assign(db->parent->artist, artist);
			    g_free(artist);
			    act = TRUE;
		    }
		    if (org_exaile_DBusInterface_get_album
			(db->exPlayer, &album, NULL)) {
			    g_string_assign(db->parent->album, album);
			    g_free(album);
			    act = TRUE;
		    }
		    if (org_exaile_DBusInterface_get_title
			(db->exPlayer, &title, NULL)) {
			    g_string_assign(db->parent->title, title);
			    g_free(title);
			    act = TRUE;
		    }
		    if (org_exaile_DBusInterface_get_cover_path
			(db->exPlayer, &cover, NULL)) {
			    if (g_file_test(cover, G_FILE_TEST_EXISTS) && !g_str_has_suffix(cover, "/nocover.png")) {
				    g_string_assign(db->parent->albumArt,
						    cover);
					LOG("Cover art: %s", cover);
			    } else {
			    	if(org_exaile_DBusInterface_get_track_attr(db->exPlayer, "loc", &db->lastTrack, NULL)) {
						g_string_truncate(db->parent->albumArt, 0);
						db->parent->FindAlbumArtByFilePath(db->parent->sd, db->lastTrack);
			    	}
			    }
			    g_free(cover);
			    act = TRUE;
		    }
		    break;
	    case estErr:
	    default:
		    g_string_truncate(db->parent->artist, 0);
		    g_string_truncate(db->parent->album, 0);
		    g_string_truncate(db->parent->title, 0);
		    act = TRUE;
	}
	if (act) {
		db->parent->Update(db->parent->sd, TRUE, eStat, NULL);
	}
	LOG("Leave auCallback: TrackChange");
}

static gint exCallbackFake(gpointer thsPtr) {
	MKTHIS;
	if(db->needsPolling) {
		gchar *loc = NULL;
		if(org_exaile_DBusInterface_get_track_attr(db->exPlayer, "loc", &loc, NULL)) {
			if(!db->lastTrack || g_ascii_strcasecmp(db->lastTrack, loc)) {
				if(db->lastTrack)
					g_free(db->lastTrack);
				db->lastTrack = loc;
				exCallbackTrackChange(db->exPlayer, thsPtr);
			} else
				g_free(loc);
		}
		gchar *status = NULL;
		if(org_exaile_DBusInterface_status(db->exPlayer, &status, NULL)) {
			if(!db->lastStatus || g_ascii_strcasecmp(db->lastStatus, loc)) {
				if(db->lastStatus)
					g_free(db->lastStatus);
				db->lastStatus = status;
				exCallbackStatusChange(db->exPlayer, thsPtr);
			} else
				g_free(status);
		}
	} else {
		exCallbackTrackChange(db->exPlayer, thsPtr);
		exCallbackStatusChange(db->exPlayer, thsPtr);
	}
	return TRUE;
}

static gboolean exAssure(gpointer thsPtr, gboolean noCreate) {
	MKTHIS;
	gboolean bRet = TRUE;
	LOG("Enter exAssure");
	if (db->parent->bus && !db->exPlayer) {
		GError *error = NULL;
		db->exPlayer = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							       EX_dbusName(),
							       "/DBusInterfaceObject",
							       "org.exaile.DBusInterface",
							       &error);

		if (error) {
			LOGWARN("\tCouldn't connect to shell proxy '%s' ",
				error->message);
			if (noCreate)
				bRet = FALSE;
			else {
				bRet = db->parent->StartService(db->parent->sd);
			}
			g_error_free(error);
			error = NULL;
		}
		if (db->exPlayer) {
			gchar *version = NULL;
			db->needsPolling = TRUE;
			if(org_exaile_DBusInterface_get_version(db->exPlayer, &version, NULL)) {
				LOG("Attached to exaile version '%s'", version);
				if(!g_ascii_strcasecmp(version, "0.2.14devel")) {
					LOG("Lucky us we have signals");
					db->needsPolling = FALSE;
					// state changes
					//  playing change
					dbus_g_proxy_add_signal(db->exPlayer, "state_changed",
								G_TYPE_INVALID);
					dbus_g_proxy_connect_signal(db->exPlayer,
									"state_changed",
									G_CALLBACK
									(exCallbackStatusChange),
									db, NULL);

					//  song change
					dbus_g_proxy_add_signal(db->exPlayer, "track_changed",
								G_TYPE_INVALID);
					dbus_g_proxy_connect_signal(db->exPlayer,
									"track_changed",
									G_CALLBACK
									(exCallbackTrackChange), db,
									NULL);
				}
				g_free(version);
			}
			if(db->needsPolling) {
				db->intervalID =
					g_timeout_add(1500, exCallbackFake, db);
			}
		}

	}
	// reflect UI
	if (bRet == FALSE) {
		db->parent->Update(db->parent->sd, FALSE,
				   (noCreate)?estStop:estErr, NULL);
	}
	LOG("Leave exAssure");
	return bRet;
}

static gboolean exNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exNext");
	if (!exAssure(db, FALSE))
		return FALSE;
	if (!org_exaile_DBusInterface_next_track(db->exPlayer, NULL)) {
		LOGERR("Failed to complete Next");
	}
	LOG("Leave exNext");
	return TRUE;
}

static gboolean exPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exPrevious");
	if (!exAssure(db, FALSE))
		return FALSE;
	if (!org_exaile_DBusInterface_prev_track(db->exPlayer, NULL)) {
		LOGERR("Failed to complete Previous");
	}
	LOG("Leave exPrevious");
	return TRUE;
}

static gboolean exPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (!exAssure(db, TRUE))
		return FALSE;
	return org_exaile_DBusInterface_play_pause(db->exPlayer, NULL);
}

static gboolean exIsPlaying(gpointer thsPtr) {
	MKTHIS;
	return db->isPlaying;
}

static gboolean exToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean oldState = FALSE;
	if (!exAssure(db, FALSE))
		return FALSE;
	oldState = exIsPlaying(db);
	exPlayPause(db, !oldState);
	if (newState)
		*newState = exIsPlaying(db);
	return TRUE;
}

gboolean exIsVisible(gpointer thsPtr) {
	MKTHIS;
	return db->Visibility;
}


gboolean exShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if (exAssure(thsPtr, FALSE)) {
		org_exaile_DBusInterface_toggle_visibility(db->exPlayer, NULL);
		return TRUE;
	}
	return FALSE;
}

static gboolean exDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exDetach");
	if (db->exPlayer) {
		if(!db->needsPolling) {
			dbus_g_proxy_disconnect_signal(db->exPlayer,
							"state_changed",
							G_CALLBACK
							(exCallbackStatusChange),
							db);
			dbus_g_proxy_disconnect_signal(db->exPlayer,
							"track_changed",
							G_CALLBACK
							(exCallbackTrackChange), db);
		} else {
			if(db->lastTrack)
				g_free(db->lastTrack);
			if(db->lastStatus)
				g_free(db->lastStatus);
		}
		g_object_unref(G_OBJECT(db->exPlayer));
		db->exPlayer = NULL;
	}
	//g_free(db);
	LOG("Leave exDetach");

	return TRUE;
}

static gboolean exUpdateDBUS(gpointer thsPtr, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("Exaile has started");
		if (!db->exPlayer && exAssure(thsPtr, FALSE))
			exCallbackFake(thsPtr);
	} else {
		LOG("Exaile has died");
		if (db->exPlayer) {
			g_object_unref(G_OBJECT(db->exPlayer));
			db->exPlayer = NULL;
		}
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		db->parent->Update(db->parent->sd, TRUE, estStop, NULL);
	}
	return TRUE;
}

void exUpdateWindow(gpointer thsPtr, WnckWindow *window, gboolean appeared) {
	MKTHIS;
	LOG("exaile is %s", (appeared)?"visible":"invisible");
	db->Visibility = appeared;
}


exData *EX_attach(SPlayer * parent) {
	exData *db = NULL;

	LOG("Enter EX_attach");
	EX_MAP(Assure);
	EX_MAP(Next);
	EX_MAP(Previous);
	EX_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	EX_MAP(IsPlaying);
	EX_MAP(Toggle);
	EX_MAP(Detach);
	NOMAP(Configure);
	NOMAP(Persist);
	EX_MAP(IsVisible);
	EX_MAP(Show);
	EX_MAP(UpdateDBUS);
	NOMAP(GetRepeat);
	NOMAP(SetRepeat);
	NOMAP(GetShuffle);
	NOMAP(SetShuffle);
	EX_MAP(UpdateWindow);

	db = g_new0(exData, 1);
	db->parent = parent;
	db->exPlayer = NULL;

	// init Quarks
	stopped = g_quark_from_static_string("stopped");
	paused = g_quark_from_static_string("paused");
	playing = g_quark_from_static_string("playing");

	LOG("Leave EX_attach");
	return db;
}

#endif
