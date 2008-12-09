/***************************************************************************
 *            rythmbox-ex.c
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
#include <dbus/dbus-glib.h>
#include "exaile-player-binding.h"


#ifndef DBUS_TYPE_G_STRING_VALUE_HASHTABLE
#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

#define EX_MAP(a) player->a = ex##a;

// pixmap
#include "squeezebox-ex.png.h"

DEFINE_DBUS_BACKEND(EX, _("exaile 0.2.x (via DBUS)"), "org.exaile.DBusInterface")

typedef struct {
	SPlayer			*parent;
	DBusGProxy 		*exPlayer;
	gboolean		noCreate;
    gboolean        Visibility;
    gboolean        isPlaying;
    gint            intervalID;
    guchar          lastPosition;
}exData;

#define MKTHIS exData *db = (exData *)thsPtr;
// init Quarks
GQuark stopped = 0;
GQuark paused  = 0;
GQuark playing = 0;

// MFCish property map -- currently none
BEGIN_PROP_MAP(EX)
END_PROP_MAP()                

// implementation

gint exCallback(gpointer thsPtr) {
	MKTHIS;
    gboolean doAct = FALSE;
    /* -- current_position is currently broken in exaile 0.2.99.1
    guchar actPos = 0;
    gboolean actState = db->isPlaying;
	if( db->exPlayer != NULL ) {
        db->isPlaying = FALSE;
        if( org_exaile_ExaileInterface_current_position(db->exPlayer, &actPos, NULL)) {
            LOG(".");
            actState = (actPos != db->lastPosition);
            db->lastPosition = actPos;
            if(actState != db->isPlaying) {
                db->isPlaying = actState;
                doAct = TRUE;
            }
        }
    }
    if(doAct) {
        db->parent->Update(db->parent->sd, FALSE, 
                db->isPlaying?estPlay:estPause, NULL);        
    }
    */
    
    /* getting track attributes seems broken too :( */
	if( db->exPlayer != NULL ) {
        db->isPlaying = TRUE;
    }
    if(doAct) {
        db->parent->Update(db->parent->sd, TRUE, 
                estPlay, NULL);        
    }
    
	return TRUE;	
}

static eSynoptics exTranslateStatus(gchar* exStatus) {

    GQuark newStat = g_quark_from_string(exStatus);
    if(newStat == playing) 
        return estPlay;
    else if(newStat == paused) 
        return estPause;
    if(newStat == stopped) 
        return estStop;
    
    return estErr;
}

static void exCallbackStatusChange(DBusGProxy *proxy, gpointer thsPtr) {
	MKTHIS;
    eSynoptics eStat = estErr;
	LOG("Enter auCallback: StatusChange");
    gchar *status = NULL;
    if(org_exaile_DBusInterface_status(db->exPlayer, &status, NULL))
    {
        eStat = exTranslateStatus(status);
        g_free(status);
    }
	db->parent->Update(db->parent->sd, FALSE, eStat, NULL);
	LOG("Leave auCallback: StatusChange");
}

static void exCallbackTrackChange(DBusGProxy *proxy, gpointer thsPtr) {
    MKTHIS;
    gchar *artist = NULL, *album = NULL, *title = NULL, *cover = NULL;
    gboolean act = FALSE;
    gchar *status = NULL;
    
	LOG("Enter auCallback: TrackChange");
    eSynoptics eStat = estErr;
    if(org_exaile_DBusInterface_status(db->exPlayer, &status, NULL))
    {
        eStat = exTranslateStatus(status);
        g_free(status);
    }
    switch(eStat)
    {
        case estPlay:
        case estPause:
            if(org_exaile_DBusInterface_get_artist(db->exPlayer, &artist, NULL)) {
                g_string_assign(db->parent->artist, artist);
                g_free(artist);
                act = TRUE;
            }
            if(org_exaile_DBusInterface_get_album(db->exPlayer, &album, NULL)) {
                g_string_assign(db->parent->album, album);
                g_free(album);
                act = TRUE;
            }
            if(org_exaile_DBusInterface_get_title(db->exPlayer, &title, NULL)) {
                g_string_assign(db->parent->title, title);
                g_free(title);
                act = TRUE;
            }
            if(org_exaile_DBusInterface_get_cover_path(db->exPlayer, &cover, NULL)) {
                if( g_file_test(cover, G_FILE_TEST_EXISTS) ) 
                    g_string_assign(db->parent->albumArt, cover);
                else
                    g_string_truncate(db->parent->albumArt, 0);
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
    if(act) {
        db->parent->Update(db->parent->sd, TRUE, eStat, NULL);
    }
	LOG("Leave auCallback: TrackChange");
}

static void exCallbackFake(gpointer thsPtr) {
    MKTHIS;
    exCallbackTrackChange(db->exPlayer, thsPtr);
    exCallbackStatusChange(db->exPlayer, thsPtr);
}    

static gboolean exAssure(gpointer thsPtr) {
    MKTHIS;
	gboolean bRet = TRUE;
    gchar *errLine = NULL;
	LOG("Enter exAssure");
	if( db->parent->bus && !db->exPlayer )
	{
		GError *error = NULL;
		db->exPlayer = dbus_g_proxy_new_for_name_owner(db->parent->bus,
							  EX_dbusName(),
							  "/DBusInterfaceObject",
							  "org.exaile.DBusInterface",
							  &error);
		
		
		if( error )
		{
			LOGWARN("\tCouldn't connect to shell proxy '%s' ",
				error->message);
			if( db->noCreate )
				bRet = FALSE;
			else 
			{
				DBusGProxy *bus_proxy;
				guint start_service_reply;
				LOG("\tstarting new instance");

				bus_proxy = dbus_g_proxy_new_for_name (db->parent->bus,
							  "org.exaile.DBusInterface",
							  "/DBusInterfaceObject",
							  "org.exaile.DBusInterface");
		
                g_error_free(error); 
                error = NULL;

				if (!dbus_g_proxy_call (bus_proxy, "StartServiceByName", &error,
							G_TYPE_STRING, "org.exaile.DBusInterface",
							G_TYPE_UINT, 0,
							G_TYPE_INVALID,
							G_TYPE_UINT, &start_service_reply,
							G_TYPE_INVALID)) 
				{
					LOGWARN("Could'n start service '%s'", error->message);
					bRet = FALSE;
                    errLine = g_strdup(error->message);
				}
			}
            g_error_free(error); 
            error = NULL;
		}
		if( db->exPlayer )
		{
			// state changes
            //  playing change
			dbus_g_proxy_add_signal(db->exPlayer, "state_changed", 
				G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->exPlayer, "state_changed", 
				G_CALLBACK(exCallbackStatusChange), db, NULL);	
	
			//  song change
			dbus_g_proxy_add_signal(db->exPlayer, "track_changed", 
				G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->exPlayer, "track_changed", 
				G_CALLBACK(exCallbackTrackChange), db, NULL);
		}
			
	}

    // reflect UI
    if( bRet == FALSE )
    {        
        if( db->noCreate )
        {
            db->parent->Update(db->parent->sd, FALSE, 
                estStop, NULL);        
        }
        else
        {
            db->parent->Update(db->parent->sd, FALSE, 
                estErr, errLine);        
            if( errLine )
                g_free(errLine);
        }
    }
    else {
        if( db->noCreate )
            db->parent->Update(db->parent->sd, FALSE, estPlay, NULL);        

    }

	LOG("Leave exAssure");
	return bRet;
}

static gboolean exNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exNext");
	if( !exAssure(db) )
		return FALSE;
	if (!org_exaile_DBusInterface_next_track (db->exPlayer, NULL)){
		LOGERR("Failed to complete Next");
	}
	LOG("Leave exNext");
	return TRUE;
}

static gboolean exPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exPrevious");
	if( !exAssure(db) )
		return FALSE;
	if (!org_exaile_DBusInterface_prev_track (db->exPlayer, NULL)){
		LOGERR("Failed to complete Previous");
	}
	LOG("Leave exPrevious");
	return TRUE;
}

static gboolean exPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if( !exAssure(db) )
		return FALSE;
    return org_exaile_DBusInterface_play_pause(db->exPlayer, NULL);
}

static gboolean exIsPlaying(gpointer thsPtr) {
	MKTHIS;
	return db->isPlaying;
}

static gboolean exToggle(gpointer thsPtr, gboolean *newState) {
    MKTHIS;
    gboolean oldState = FALSE;
    if( !exAssure(db) )
		return FALSE;
    oldState = exIsPlaying(db);
    exPlayPause(db, !oldState);
    if(newState)
        *newState = exIsPlaying(db);
    return TRUE;
}

gboolean exShow(gpointer thsPtr, gboolean newState) {
    MKTHIS;
    if( exAssure(thsPtr) ) {
        org_exaile_DBusInterface_toggle_visibility(db->exPlayer, NULL);
        return TRUE;
    }
    return FALSE;
}

static gboolean exDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exDetach");
	if( db->exPlayer )
	{
		g_object_unref (G_OBJECT (db->exPlayer));		
		db->exPlayer = NULL;
	}
	//g_free(db);
	LOG("Leave exDetach");
	
	return TRUE;
}

static gboolean
exUpdateDBUS(gpointer thsPtr, gboolean appeared) {
	MKTHIS;
	if(appeared){
        LOG("Exaile has started");
        if( !db->exPlayer && exAssure(thsPtr))
           exCallbackFake(thsPtr);
    }
    else {
        LOG("Audacious has died");
        if( db->exPlayer )
        {
            g_object_unref (G_OBJECT (db->exPlayer));		
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

exData * EX_attach(SPlayer *player) {
	exData *db = NULL;
	
	LOG("Enter EX_attach");
	EX_MAP(Assure);
	EX_MAP(Next);
	EX_MAP(Previous);
	EX_MAP(PlayPause);
	EX_MAP(IsPlaying);
	EX_MAP(Toggle);
	EX_MAP(Detach);
	    NOMAP(Configure);
	    NOMAP(Persist);
    //The DBUS API does not yet provide:
        NOMAP(IsVisible);
    EX_MAP(Show);
    EX_MAP(UpdateDBUS);
	    NOMAP(GetRepeat);
        NOMAP(SetRepeat);
        NOMAP(GetShuffle);
        NOMAP(SetShuffle);
	
	db = g_new0(exData, 1);
	db->parent = player;
	db->exPlayer = NULL;
	db->noCreate = TRUE;
    
    // init Quarks
    stopped = g_quark_from_static_string("stopped");
    paused  = g_quark_from_static_string("paused");
    playing = g_quark_from_static_string("playing");
    
 	// check if exaile is running
	if( exAssure(db) ){
        // emulate state change
        exCallbackFake(db);
	}
	db->noCreate = FALSE;

	LOG("Leave EX_attach");
	return db;
}

#endif
