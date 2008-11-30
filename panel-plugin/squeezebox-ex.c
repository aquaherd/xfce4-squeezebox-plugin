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

DEFINE_BACKEND(EX, _("exaile 0.2.x (via DBUS)"))

typedef struct {
	SPlayer			*parent;
	DBusGConnection *bus;
	DBusGProxy 		*exPlayer;
    DBusGProxy		*dbService;
	gboolean		noCreate;
    gboolean        Visibility;
}exData;

#define MKTHIS exData *db = (exData *)thsPtr;

// implementation

static void
exCallbackNameOwnerChanged(DBusGProxy *proxy, const gchar* Name, 
	const gchar *OldOwner, const gchar* NewOwner, gpointer thsPtr) {
	MKTHIS;
    if( !g_ascii_strcasecmp(Name, "org.exaile.DBusInterface") && !strlen(NewOwner) )
	{
		LOGF("Exaile has died? %s|%s|%s\n", Name, OldOwner, NewOwner);
        if( db->exPlayer )
        {
            g_object_unref (G_OBJECT (db->exPlayer));		
            db->exPlayer = NULL;
        }
        if( db->dbService )
        {
            g_object_unref (G_OBJECT (db->dbService));		
            db->dbService = NULL;
        }
        db->parent->Update(db->parent->sd, FALSE, estStop, NULL);
	}
}

static gboolean exAssure(gpointer thsPtr) {
	gboolean bRet = TRUE;
    gchar *errLine = NULL;
	exData *db = (exData*)thsPtr;
	LOG("Enter exAssure\n");
	if( !db->bus )
	{
		db->bus = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
		if( !db->bus )
		{
			LOGERR("\tCouldn't connect to dbus\n");
			bRet = FALSE;
		}
		
	}
	if( db->bus && !db->exPlayer )
	{
		GError *error = NULL;
		db->exPlayer = dbus_g_proxy_new_for_name_owner(db->bus,
							  "org.exaile.DBusInterface",
							  "/DBusInterfaceObject",
							  "org.exaile.DBusInterface",
							  &error);
		
		
		if( error )
		{
			LOGWARN("\tCouldn't connect to shell proxy '%s' \n",
				error->message);
			if( db->noCreate )
				bRet = FALSE;
			else 
			{
				DBusGProxy *bus_proxy;
				guint start_service_reply;
				LOG("\tstarting new instance\n");

				bus_proxy = dbus_g_proxy_new_for_name (db->bus,
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
			// user close notification
			db->dbService = dbus_g_proxy_new_for_name(db->bus,
				DBUS_SERVICE_DBUS,
				DBUS_PATH_DBUS,
				DBUS_INTERFACE_DBUS);
			
			dbus_g_proxy_add_signal(db->dbService, "NameOwnerChanged",
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->dbService, "NameOwnerChanged", 
				G_CALLBACK(exCallbackNameOwnerChanged),
				db,
				NULL);
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

	LOG("Leave exAssure\n");
	return bRet;
}

static gboolean exNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exNext\n");
	if( !exAssure(db) )
		return FALSE;
	if (!org_exaile_DBusInterface_next_track (db->exPlayer, NULL)){
		LOGERR("Failed to complete Next\n");
	}
	LOG("Leave exNext\n");
	return TRUE;
}

static gboolean exPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exPrevious\n");
	if( !exAssure(db) )
		return FALSE;
	if (!org_exaile_DBusInterface_prev_track (db->exPlayer, NULL)){
		LOGERR("Failed to complete Previous\n");
	}
	LOG("Leave exPrevious\n");
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
    gchar *status = NULL;
	if( !exAssure(db) )
		return FALSE;
	if( !org_exaile_DBusInterface_status(db->exPlayer, &status, NULL)){
		LOGERR("Failed to complete get_status\n");
		return FALSE;
	}
	return (0 == g_ascii_strncasecmp(status, "playing", 7));
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

static gboolean exDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter exDetach\n");
	if( db->exPlayer )
	{
		g_object_unref (G_OBJECT (db->exPlayer));		
		db->exPlayer = NULL;
	}
	if( db->dbService )
	{
		g_object_unref (G_OBJECT (db->dbService));		
		db->dbService = NULL;
	}
	if( db->bus )
	{
		//some reading shows that this should not be freed
		//but its not clear if meant server only.
		//g_object_unref(G_OBJECT(db->bus));
		db->bus = NULL;		
	}
	//g_free(db);
	LOG("Leave exDetach\n");
	
	return TRUE;
}

void exPersist(gpointer thsPtr, XfceRc *rc, gboolean bIsStoring) {
    // no settings at the moment
	//MKTHIS;
}

exData * EX_attach(SPlayer *player) {
	exData *db = NULL;
	
	LOG("Enter EX_attach\n");
	EX_MAP(Assure);
	EX_MAP(Next);
	EX_MAP(Previous);
	EX_MAP(PlayPause);
	EX_MAP(IsPlaying);
	EX_MAP(Toggle);
	EX_MAP(Detach);
	EX_MAP(Persist);
    //The DBUS API does not yet provide:
       NOMAP(IsVisible);
       NOMAP(Show);
	   NOMAP(GetRepeat);
       NOMAP(SetRepeat);
       NOMAP(GetShuffle);
       NOMAP(SetShuffle);
	
	db = g_new0(exData, 1);
	db->parent = player;
	db->bus = NULL;
	db->exPlayer = NULL;
	db->noCreate = TRUE;
	
	// check if exaile is running
	
	if( exAssure(db) ){
        // emulate state change
	}
	db->noCreate = FALSE;
	LOG("Leave EX_attach\n");
	return db;
}

#endif
