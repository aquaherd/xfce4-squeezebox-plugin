/***************************************************************************
 *            sqqueezebox-cs.c
 *
 *  Sat Dec 06 22:00:00 2008
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
#ifdef HAVE_BACKEND_CONSONANCE

// default
#include "squeezebox.h"

// dbus-lowlevels for consonance remote
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#define CS_MAP(a) player->a = cs##a;

// pixmap
#include "squeezebox-cs.png.h"

DEFINE_BACKEND(CS, _("consonance 0.3.x (via DBUS)"))

typedef struct {
	SPlayer			*parent;
	gboolean		noCreate;
    gboolean        Visibility;
    gboolean        Shuffle;
    gboolean        Repeat;
    GString         *file;
	DBusConnection  *con_dbus;			/* DBUS connection */
    guint           intervalID;
    GQuark          csPlaying, csPaused, csStopped;
    eSynoptics      oldStat;
}csData;

// MFCish property map -- currently none
BEGIN_PROP_MAP(CS)
END_PROP_MAP()                

#define MKTHIS csData *db = (csData *)thsPtr;

/* taken from consonance/consonance.h */

#define DBUS_PATH "/org/consonance/DBus"
#define DBUS_NAME "org.consonance.DBus"
#define DBUS_INTERFACE "org.consonance.DBus"

#define DBUS_SIG_PLAY     "play"
#define DBUS_SIG_STOP     "stop"
#define DBUS_SIG_PAUSE    "pause"
#define DBUS_SIG_NEXT     "next"
#define DBUS_SIG_PREV     "prev"
#define DBUS_SIG_INC_VOL  "inc_vol"
#define DBUS_SIG_DEC_VOL  "dec_vol"
#define DBUS_SIG_SHOW_OSD "show_osd"
#define DBUS_SIG_ADD_FILE "add_files"

#define DBUS_METHOD_CURRENT_STATE "curent_state"

/* Send a signal to a running instance */

void dbus_send_signal(const gchar *signal, void *thsPtr)
{
    MKTHIS;
    DBusMessage *msg = NULL;

	msg = dbus_message_new_signal(DBUS_PATH, DBUS_INTERFACE, signal);

	if (!msg) {
		g_critical("(%s): Unable to allocate memory for DBUS message", __func__);
		return;
	}

	if (!dbus_connection_send(db->con_dbus, msg, NULL)) {
		g_critical("(%s): Unable to send DBUS message", __func__);
		goto exit;
	}

	dbus_connection_flush(db->con_dbus);
exit:
	dbus_message_unref(msg);
}


// implementation

static gboolean csPoll(gpointer thsPtr){
    MKTHIS;
	gboolean ret = TRUE;
    gboolean fileChanged = FALSE;
	DBusMessage *msg = NULL;
	DBusMessage *reply_msg = NULL;
	DBusError d_error;
	const char *state, *file, *title, *artist, *album;
    eSynoptics eStat = estStop;
	dbus_error_init(&d_error);

	msg = dbus_message_new_method_call(DBUS_NAME,
					   DBUS_PATH,
					   DBUS_INTERFACE,
					   DBUS_METHOD_CURRENT_STATE);
	if (!msg) {
		g_critical("(%s): Unable to allocate memory for DBUS message",
			   __func__);
		ret = FALSE;
		exit(0);
	}

	reply_msg = dbus_connection_send_with_reply_and_block(db->con_dbus, msg,
							      -1, &d_error);
	if (!reply_msg) {
		g_critical("(%s): Unable to send DBUS message", __func__);
		dbus_error_free(&d_error);
		ret = FALSE;
		goto bad;
	}

	if (!dbus_message_get_args(reply_msg, &d_error,
				   DBUS_TYPE_STRING, &state,
				   DBUS_TYPE_INVALID)) {
		g_critical("(%s): Unable to get player state", __func__);
		dbus_error_free(&d_error);
		ret = FALSE;
		goto bad;
	}
    
    GQuark csAct = g_quark_from_string(state);
    if(csAct == db->csStopped)
        eStat = estStop;
    else if(csAct == db->csPaused)
        eStat = estPause;
    else if(csAct == db->csPlaying)
        eStat = estPlay;
    else 
        eStat = estErr;
    
	if (g_ascii_strcasecmp(state, "Stopped")) {
		dbus_message_get_args(reply_msg, &d_error,
				      DBUS_TYPE_STRING, &state,
				      DBUS_TYPE_STRING, &file,
				      DBUS_TYPE_STRING, &title,
				      DBUS_TYPE_STRING, &artist,
				      DBUS_TYPE_STRING, &album,
				      DBUS_TYPE_INVALID);
		if (!dbus_message_get_args(reply_msg, &d_error,
					   DBUS_TYPE_STRING, &state,
					   DBUS_TYPE_INVALID)) {
			g_critical("(%s): Unable to get player state details", __func__);
			dbus_error_free(&d_error);
			ret = FALSE;
			goto bad;
		}
        
        if(!g_str_equal(db->file->str, file)) {
            g_string_assign(db->file, file);
            fileChanged = TRUE;
            if(!g_str_equal(db->parent->artist->str, artist))
                g_string_assign(db->parent->artist, artist);
            if(!g_str_equal(db->parent->album->str, album))
                g_string_assign(db->parent->album, album);
            if(!g_str_equal(db->parent->title->str, title))
                g_string_assign(db->parent->title, title);
            db->parent->FindAlbumArtByFilePath(db->parent->sd, db->file->str);
        }
	}
    else if(db->file->len) {
        g_string_truncate(db->file, 0);
        g_string_truncate(db->parent->artist, 0);
        g_string_truncate(db->parent->album, 0);
        g_string_truncate(db->parent->title, 0);
        g_string_truncate(db->parent->albumArt, 0);
        fileChanged = TRUE;
    }
        
    if(eStat != db->oldStat || fileChanged) {
        LOG("State change detected: %s: '%s'", state, db->file->str);
        db->oldStat = eStat;
        db->parent->Update(db->parent->sd, fileChanged, eStat, NULL);
    }
    
bad:
	dbus_message_unref(msg);
    
    return ret;
}

static gboolean csAssure(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter csAssure");
    if(NULL == db->con_dbus) {
	    DBusConnection *conn = NULL;
	    DBusError error;
	    gint ret = 0;

	    dbus_error_init(&error);
	    conn = dbus_bus_get(DBUS_BUS_SESSION, &error);
	    if (!conn) {
		    g_critical("(%s): Unable to get a DBUS connection", __func__);
		    dbus_error_free(&error);
		    return FALSE;
	    }

	    ret = dbus_bus_request_name(conn, DBUS_NAME, 0, &error);
	    if (ret == -1) {
		    g_critical("(%s): Unable to request for DBUS service name", __func__);
		    dbus_error_free(&error);
		    return FALSE;
	    }

	    dbus_connection_setup_with_g_main(conn, NULL);
	    db->con_dbus = conn;

    }
	LOG("Leave csAssure");
	return (NULL != db->con_dbus);
}

static gint csCallback(gpointer thsPtr) {
    MKTHIS;
    if(NULL != db->con_dbus)
        csPoll(thsPtr);
	return TRUE;	
}

static gboolean csNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter csNext");
    if( !csAssure(db) )
		return FALSE;
	dbus_send_signal(DBUS_SIG_NEXT, db);
	LOG("Leave csNext");
	return TRUE;
}

static gboolean csPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter csPrevious");
    if( !csAssure(db) )
		return FALSE;
	dbus_send_signal(DBUS_SIG_PREV, db);
	LOG("Leave csPrevious");
	return TRUE;
}

static gboolean csPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter csPlayPause %d", newState);
    if(csAssure(thsPtr)) {
        dbus_send_signal((newState)?DBUS_SIG_PLAY:DBUS_SIG_PAUSE, db);
    }
	LOG("Leave csPlayPause");
	return TRUE;
}

static gboolean csIsPlaying(gpointer thsPtr) {
	MKTHIS;
	return (db->oldStat == estPlay);
}

static gboolean csToggle(gpointer thsPtr, gboolean *newState) {
    MKTHIS;
	LOG("Enter csToggle");
    if( !csAssure(db) )
		return FALSE;
    gboolean newStat = FALSE;
    switch(db->oldStat) {
        case estPlay: newStat = FALSE; break;
        case estPause: newStat = TRUE; break;
        case estStop: newStat = TRUE; break;
        default:
            return FALSE;
    }
    csPlayPause(db, newStat);
    if(newState)
        *newState = csIsPlaying(db);
	LOG("Leave csToggle");
    return TRUE;
}

static gboolean csDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter csDetach");
	if( db->con_dbus ){
		g_object_unref (G_OBJECT (db->con_dbus));		
		db->con_dbus = NULL;
	}
    if( db->file ) {
        g_string_free(db->file, TRUE);
        db->file = NULL;
    }
	if( db->intervalID )
	{
		g_source_remove(db->intervalID);
		db->intervalID = 0;
	}
	LOG("Leave csDetach");
	return TRUE;
}

csData * CS_attach(SPlayer *player) {
	csData *db = NULL;
	
	LOG("Enter CS_attach");
	CS_MAP(Assure);
	CS_MAP(Next);
	CS_MAP(Previous);
	CS_MAP(PlayPause);
	CS_MAP(IsPlaying);
	CS_MAP(Toggle);
	CS_MAP(Detach);
	 NOMAP(Configure); // no settings
	 NOMAP(Persist); // no settings
     NOMAP(IsVisible);
     NOMAP(Show);
     NOMAP(GetRepeat);
     NOMAP(SetRepeat);
     NOMAP(GetShuffle);
     NOMAP(SetShuffle);
	
	db = g_new0(csData, 1);
	db->parent = player;
	db->noCreate = TRUE;
    db->file = g_string_new("");
	
    // quarks
    db->csPlaying = g_quark_from_string("Playing");
    db->csPaused = g_quark_from_string("Paused");
    db->csStopped = g_quark_from_string("Stopped");
    
    // establish the callback function
    db->intervalID = 
	    g_timeout_add(1000, csCallback, db);

    // check if consonance is running
	if( csAssure(db) ){
	}
	db->noCreate = FALSE;
	LOG("Leave CS_attach");
	return db;
}

#endif
