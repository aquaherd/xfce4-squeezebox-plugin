/***************************************************************************
 *            squeezebox-pr.c
 *
 *  Wed Jan 12 22:29:00 2011
 *  Copyright  2008 by Hakan Erduman
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
#ifdef HAVE_BACKEND_PRAGHA

// default
#include "squeezebox.h"

// dbus-lowlevels for pragha remote
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#define PR_MAP(a) parent->a = pr##a;

typedef struct prData{
	SPlayer *parent;
	gboolean Visibility;
	gboolean Shuffle;
	gboolean Repeat;
	GString *file;
	DBusConnection *con_dbus;	/* DBUS connection */
	DBusGProxy *prPlayer;	/* currently for removal detection */
	guint intervalID;
	GQuark prPlaying, prPaused, prStopped;
	eSynoptics oldStat;
} prData;

#define MKTHIS prData *db = (prData *)thsPtr
/* taken from pragha/pragha.h */
#define DBUS_PATH           "/org/pragha/DBus"
#define DBUS_NAME           "org.pragha.DBus"
#define DBUS_INTERFACE      "org.pragha.DBus"
#define DBUS_SIG_PLAY       "play"
#define DBUS_SIG_STOP       "stop"
#define DBUS_SIG_PAUSE      "pause"
#define DBUS_SIG_NEXT       "next"
#define DBUS_SIG_PREV       "prev"
#define DBUS_SIG_INC_VOL    "inc_vol"
#define DBUS_SIG_DEC_VOL    "dec_vol"
#define DBUS_SIG_SHOW_OSD   "show_osd"
#define DBUS_SIG_TOGGLE     "toggle_view"
#define DBUS_SIG_ADD_FILE   "add_files"
#define DBUS_METHOD_CURRENT_STATE "curent_state"
    
gpointer PR_attach(SPlayer * parent);	
#define BASENAME "pragha"
DEFINE_DBUS_BACKEND(PR, _("Pragha"), DBUS_NAME, "pragha");

/* Send a signal to a running instance */
static void dbus_send_signal(const gchar * signalName, void *thsPtr) {
	MKTHIS;
	DBusMessage *msg = NULL;

	msg = dbus_message_new_signal(DBUS_PATH, DBUS_INTERFACE, signalName);

	if (!msg) {
		g_critical("(%s): Unable to allocate memory for DBUS message",
			   __func__);
		return;
	}

	if (!dbus_connection_send(db->con_dbus, msg, NULL)) {
		g_critical("(%s): Unable to send DBUS message", __func__);
		goto exit;
	}
	//dbus_connection_flush(db->con_dbus);
exit:
	dbus_message_unref(msg);
}

// implementation

static gboolean prPoll(gpointer thsPtr) {
	MKTHIS;
	gboolean ret = TRUE;
	gboolean fileChanged = FALSE;
	DBusMessage *msg = NULL;
	DBusMessage *reply_msg = NULL;
	DBusError d_error;
	const char *state, *file, *title, *artist, *album;
	eSynoptics eStat = estStop;
	GQuark prAct;

	dbus_error_init(&d_error);
	msg = dbus_message_new_method_call(DBUS_NAME,
					   DBUS_PATH,
					   DBUS_INTERFACE,
					   DBUS_METHOD_CURRENT_STATE);
	if (!msg) {
		g_critical("(%s): Unable to allocate memory for DBUS message",
			   __func__);
		ret = FALSE;
		goto bad;
	}

	reply_msg = dbus_connection_send_with_reply_and_block(db->con_dbus, msg,
							      1000, &d_error);
	if (!reply_msg) {
		g_critical("(%s): Unable to send DBUS message '%s'", __func__,
			   d_error.message);
		dbus_error_free(&d_error);
		ret = FALSE;
		goto bad;
	}

	if (!dbus_message_get_args(reply_msg, &d_error,
				   DBUS_TYPE_STRING, &state,
				   DBUS_TYPE_INVALID)) {
		g_critical("(%s): Unable to get player state '%s'", __func__,
			   d_error.message);
		dbus_error_free(&d_error);
		ret = FALSE;
		goto bad;
	}

	prAct = g_quark_from_string(state);
	if (prAct == db->prStopped)
		eStat = estStop;
	else if (prAct == db->prPaused)
		eStat = estPause;
	else if (prAct == db->prPlaying)
		eStat = estPlay;
	else
		eStat = estErr;

	if (g_ascii_strcasecmp(state, "Stopped")) {
		dbus_message_get_args(reply_msg, &d_error, DBUS_TYPE_STRING,
				      &state, DBUS_TYPE_STRING, &file,
				      DBUS_TYPE_STRING, &title,
				      DBUS_TYPE_STRING, &artist,
				      DBUS_TYPE_STRING, &album,
				      DBUS_TYPE_INVALID);
		if (!dbus_message_get_args
		    (reply_msg, &d_error, DBUS_TYPE_STRING, &state,
		     DBUS_TYPE_INVALID)) {
			g_critical("(%s): Unable to get player state details",
				   __func__);
			dbus_error_free(&d_error);
			ret = FALSE;
			goto bad;
		}

		if (!g_str_equal(db->file->str, file)) {
			g_string_assign(db->file, file);
			fileChanged = TRUE;
			if (!g_str_equal(db->parent->artist->str, artist))
				g_string_assign(db->parent->artist, artist);
			if (!g_str_equal(db->parent->album->str, album))
				g_string_assign(db->parent->album, album);
			if (!g_str_equal(db->parent->title->str, title))
				g_string_assign(db->parent->title, title);
			db->parent->FindAlbumArtByFilePath(db->parent->sd,
							   db->file->str);
		}
	} else if (db->file->len) {
		g_string_truncate(db->file, 0);
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		fileChanged = TRUE;
	}

	if (eStat != db->oldStat || fileChanged) {
		LOG("State change detected: %s: '%s'", state, db->file->str);
		db->oldStat = eStat;
		db->parent->Update(db->parent->sd, fileChanged, eStat, NULL);
	}

bad:
	dbus_message_unref(msg);
	if (!ret && db->intervalID) {
		g_source_remove(db->intervalID);
		db->intervalID = 0;
	}
	return ret;
}

static gint prCallback(gpointer thsPtr) {
	MKTHIS;
	static gboolean inTimer = FALSE;
	gboolean ret = TRUE;
	if (!inTimer) {
		inTimer = TRUE;
		if (NULL != db->prPlayer) {
			if (!prPoll(thsPtr)) {
				if (db->prPlayer) {
					g_object_unref(G_OBJECT(db->prPlayer));
					db->prPlayer = NULL;
				}
				ret = FALSE;
			}
		}
		inTimer = FALSE;
	}
	return ret;
}

static gboolean prAssure(gpointer thsPtr, gboolean noCreate) {
	MKTHIS;
	gboolean bRet = TRUE;
	LOG("Enter prAssure");
	if (NULL != db->parent->bus && !db->con_dbus) {
		db->con_dbus =
		    dbus_g_connection_get_connection(db->parent->bus);
	}
	if (db->con_dbus && !db->prPlayer) {
		GError *error = NULL;
		db->prPlayer = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							       DBUS_NAME,
							       DBUS_PATH,
							       DBUS_INTERFACE,
							       &error);
		if (error) {
			bRet = FALSE;
			LOGWARN("Could'n connect to pragha '%s'",
				error->message);
			g_error_free(error);
			if (!noCreate) {
				bRet = db->parent->StartService(db->parent->sd, DBUS_NAME);
			}
		}
	}
	if (db->con_dbus && db->prPlayer && !db->intervalID) {
		// establish the callback functions
		db->intervalID = g_timeout_add(1000, prCallback, db);
	}
	// reflect UI
	if (bRet == FALSE) {
		db->parent->Update(db->parent->sd, FALSE,
				   (noCreate)?estStop:estErr, NULL);
	}
	LOG("Leave prAssure");
	return (NULL != db->con_dbus);
}

static gboolean prNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter prNext");
	if (!prAssure(db, TRUE))
		return FALSE;
	dbus_send_signal(DBUS_SIG_NEXT, db);
	LOG("Leave prNext");
	return TRUE;
}

static gboolean prPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter prPrevious");
	if (!prAssure(db, TRUE))
		return FALSE;
	dbus_send_signal(DBUS_SIG_PREV, db);
	LOG("Leave prPrevious");
	return TRUE;
}

static gboolean prPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter prPlayPause %d", newState);
	if (prAssure(thsPtr, FALSE)) {
		dbus_send_signal((newState) ? DBUS_SIG_PLAY : DBUS_SIG_PAUSE,
				 db);
	}
	LOG("Leave prPlayPause");
	return TRUE;
}

static gboolean prIsPlaying(gpointer thsPtr) {
	MKTHIS;
	return (db->oldStat == estPlay);
}

static gboolean prShow(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	LOG("Enter prPrevious");
	if (!prAssure(db, TRUE))
		return FALSE;
	dbus_send_signal(DBUS_SIG_TOGGLE, db);
	LOG("Leave prPrevious");
	return TRUE;
}

static gboolean prToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean newStat = FALSE;
	LOG("Enter prToggle");
	if (!prAssure(db, TRUE))
		return FALSE;
	switch (db->oldStat) {
	    case estPlay:
		    newStat = FALSE;
		    break;
	    case estPause:
		    newStat = TRUE;
		    break;
	    case estStop:
		    newStat = TRUE;
		    break;
	    default:
		    return FALSE;
	}
	prPlayPause(db, newStat);
	if (newState)
		*newState = prIsPlaying(db);
	LOG("Leave prToggle");
	return TRUE;
}

static gboolean prDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter prDetach");
	if (db->prPlayer) {
		g_object_unref(G_OBJECT(db->prPlayer));
		db->prPlayer = NULL;
	}
	if (db->file) {
		g_string_free(db->file, TRUE);
		db->file = NULL;
	}
	if (db->intervalID) {
		g_source_remove(db->intervalID);
		db->intervalID = 0;
	}
	//g_free(db);
	LOG("Leave prDetach");
	return TRUE;
}

static gboolean prUpdateDBUS(gpointer thsPtr, const gchar *name, gboolean appeared) {
	MKTHIS;
	if (appeared) {
		LOG("pragha has started");
		if (!db->prPlayer && prAssure(thsPtr, TRUE))
			prPoll(thsPtr);
	} else {
		LOG("pragha has died");
		if (db->prPlayer) {
			g_object_unref(G_OBJECT(db->prPlayer));
			db->prPlayer = NULL;
		}
		g_string_truncate(db->parent->artist, 0);
		g_string_truncate(db->parent->album, 0);
		g_string_truncate(db->parent->title, 0);
		g_string_truncate(db->parent->albumArt, 0);
		db->parent->Update(db->parent->sd, TRUE, estStop, NULL);
	}
	return TRUE;
}

gpointer PR_attach(SPlayer * parent) {
	prData *db = NULL;

	LOG("Enter PR_attach");
	PR_MAP(Assure);
	PR_MAP(Next);
	PR_MAP(Previous);
	PR_MAP(PlayPause);
	NOMAP(PlayPlaylist);
	PR_MAP(IsPlaying);
	PR_MAP(Toggle);
	PR_MAP(Detach);
	NOMAP(Configure);	// no settings
	NOMAP(Persist);	// no settings
	NOMAP(IsVisible);
	PR_MAP(Show);
	PR_MAP(UpdateDBUS);
	NOMAP(GetRepeat);
	NOMAP(SetRepeat);
	NOMAP(GetShuffle);
	NOMAP(SetShuffle);
	NOMAP(UpdateWindow);

	db = g_new0(prData, 1);
	db->parent = parent;
	db->file = g_string_new("");

	// quarks
	db->prPlaying = g_quark_from_string("Playing");
	db->prPaused = g_quark_from_string("Paused");
	db->prStopped = g_quark_from_string("Stopped");

	LOG("Leave PR_attach");
	return db;
}

#endif
