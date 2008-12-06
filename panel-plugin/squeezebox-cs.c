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
            if(!g_str_equal(db->parent->artist->str, artist))
                g_string_assign(db->parent->artist, artist);
            if(!g_str_equal(db->parent->album->str, album))
                g_string_assign(db->parent->album, album);
            if(!g_str_equal(db->parent->title->str, title))
                g_string_assign(db->parent->title, title);
        }
	}
bad:
	dbus_message_unref(msg);
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

static gboolean csNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter csNext");
	dbus_send_signal(DBUS_SIG_NEXT, db);
	LOG("Leave csNext");
	return TRUE;
}

static gboolean csPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter csPrevious");
	dbus_send_signal(DBUS_SIG_PREV, db);
	LOG("Leave csPrevious");
	return TRUE;
}

static gboolean csPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter csPlayPause");
    if(newState)
		dbus_send_signal(DBUS_SIG_PAUSE, db);
    else
		dbus_send_signal(DBUS_SIG_PLAY, db);
	LOG("Leave csPlayPause");
	return TRUE;
}

static gboolean csIsPlaying(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter csIsPlaying");
	LOG("Leave csIsPlaying");
	return TRUE;
}

static gboolean csToggle(gpointer thsPtr, gboolean *newState) {
    MKTHIS;
	LOG("Enter csToggle");
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
	LOG("Leave csDetach");
	return TRUE;
}

gboolean csIsVisible(gpointer thsPtr) {
    MKTHIS;
	LOG("Enter csIsVisible");
	LOG("Leave csIsVisible");
	return TRUE;
}

gboolean csShow(gpointer thsPtr, gboolean newState) {
    MKTHIS;
	LOG("Enter csShow");
	LOG("Leave csShow");
	return TRUE;
}

gboolean csGetShuffle(gpointer thsPtr) {
    MKTHIS;
	LOG("Enter csGetShuffle");
	LOG("Leave csGetShuffle");
	return TRUE;
}    

gboolean csSetShuffle(gpointer thsPtr, gboolean newShuffle) {
    MKTHIS;
	LOG("Enter csSetShuffle");
	LOG("Leave csSetShuffle");
	return TRUE;
}    

gboolean csGetRepeat(gpointer thsPtr) {
    MKTHIS;
	LOG("Enter csGetRepeat");
	LOG("Leave csGetRepeat");
	return TRUE;
}    

gboolean csSetRepeat(gpointer thsPtr, gboolean newRepeat) {
    MKTHIS;
	LOG("Enter csSetRepeat");
	LOG("Leave csSetRepeat");
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
    CS_MAP(IsVisible);
    CS_MAP(Show);
    CS_MAP(GetRepeat);
    CS_MAP(SetRepeat);
    CS_MAP(GetShuffle);
    CS_MAP(SetShuffle);
	
	db = g_new0(csData, 1);
	db->parent = player;
	db->noCreate = TRUE;
    db->file = g_string_new("");
	
	// check if audacious is running
	
	if( csAssure(db) ){
        // emulate state change
	}
	db->noCreate = FALSE;
	LOG("Leave CS_attach");
	return db;
}

#endif
