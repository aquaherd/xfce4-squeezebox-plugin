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

#define AU_MAP(a) player->a = au##a;

// pixmap
#include "squeezebox-au.png.h"

DEFINE_BACKEND(AU, _("audacious 1.5.x (via DBUS)"))

typedef struct {
	SPlayer			*parent;
	DBusGConnection *bus;
	DBusGProxy 		*auPlayer;
    DBusGProxy		*dbService;
	gboolean		noCreate;
    gboolean        Visibility;
}auData;

#define MKTHIS auData *db = (auData *)thsPtr;

// implementation

static void auCallbackCapsChange(DBusGProxy *proxy, gint caps, gpointer thsPtr) {
	// MKTHIS;
	LOGF("Enter auCallback: CapsChange %d\n", caps);
	LOG("Leave auCallback: CapsChange\n");
}

static eSynoptics auTranslateStatus(gint auStatus)
{
	eSynoptics eStat;
	switch(auStatus)
    {
        case 0: eStat = estPlay; break;
        case 1: eStat = estPause; break;
        case 2: eStat = estStop; break;
        default: eStat = estErr; break;
    }
    return eStat;   
}

static void auCallbackStatusChange(DBusGProxy *proxy, gint status, gpointer thsPtr) {
	MKTHIS;
	LOGF("Enter auCallback: StatusChange %d\n", status);
    eSynoptics eStat = auTranslateStatus(status);
	db->parent->Update(db->parent->sd, FALSE, eStat, NULL);
	LOG("Leave auCallback: StatusChange\n");
}

static void auCallbackTrackChange(DBusGProxy *proxy, GHashTable *table, gpointer thsPtr) {
    MKTHIS;
	LOG("Enter auCallback: TrackChange\n");
	GValue *tmpArtist = g_hash_table_lookup(table, "artist");
	GValue *tmpAlbum = g_hash_table_lookup(table, "album");
	GValue *tmpTitle = g_hash_table_lookup(table, "title");
	LOG("Enter auCallback: this far\n");
	GValue *tmpURI = g_hash_table_lookup(table, "URI");
	
	g_string_assign(db->parent->artist, g_value_get_string(tmpArtist));
	g_string_assign(db->parent->album, g_value_get_string(tmpAlbum));
	g_string_assign(db->parent->title, g_value_get_string(tmpTitle));
	gchar *str = g_filename_from_uri(g_value_get_string(tmpURI), NULL, NULL);    
    if(str && str[0])
        db->parent->FindAlbumArtByFilePath(db->parent->sd, str);
    eSynoptics eStat = estErr;
    gint iStat = 0;
    if(org_freedesktop_MediaPlayer_get_status(db->auPlayer, &iStat, NULL))
    {
        eStat = auTranslateStatus(iStat);
    }
	db->parent->Update(db->parent->sd, TRUE, eStat, NULL);
	LOG("Leave auCallback: TrackChange\n");
}

static void
auCallbackNameOwnerChanged(DBusGProxy *proxy, const gchar* Name, 
	const gchar *OldOwner, const gchar* NewOwner, gpointer thsPtr) {
	MKTHIS;
    if( !g_ascii_strcasecmp(Name, "org.mpris.Audacious") && !strlen(NewOwner) )
	{
		LOGF("Audacious has died? %s|%s|%s\n", Name, OldOwner, NewOwner);
        if( db->auPlayer )
        {
            g_object_unref (G_OBJECT (db->auPlayer));		
            db->auPlayer = NULL;
        }
        if( db->dbService )
        {
            g_object_unref (G_OBJECT (db->dbService));		
            db->dbService = NULL;
        }
        db->parent->Update(db->parent->sd, FALSE, estStop, NULL);
	}
}

static gboolean auAssure(gpointer thsPtr) {
	gboolean bRet = TRUE;
    gchar *errLine = NULL;
	auData *db = (auData*)thsPtr;
	LOG("Enter auAssure\n");
	if( !db->bus )
	{
		db->bus = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
		if( !db->bus )
		{
			LOGERR("\tCouldn't connect to dbus\n");
			bRet = FALSE;
		}
		
	}
	if( db->bus && !db->auPlayer )
	{
		GError *error = NULL;
		db->auPlayer = dbus_g_proxy_new_for_name_owner(db->bus,
							  "org.mpris.audacious",
							  "/Player",
							  "org.freedesktop.MediaPlayer",
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
							  "org.mpris.audacious",
							  "/Player",
							  "org.freedesktop.Mediaplayer");
		
                g_error_free(error); 
                error = NULL;

				if (!dbus_g_proxy_call (bus_proxy, "StartServiceByName", &error,
							G_TYPE_STRING, "org.mpris.audacious",
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
		if( db->auPlayer )
		{
			// state changes
            //  playing change
			dbus_g_proxy_add_signal(db->auPlayer, "StatusChange", 
				G_TYPE_INT, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->auPlayer, "StatusChange", 
				G_CALLBACK(auCallbackStatusChange), db, NULL);	
	
			//  song change
			dbus_g_proxy_add_signal(db->auPlayer, "TrackChange", 
                DBUS_TYPE_G_STRING_VALUE_HASHTABLE,
				G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->auPlayer, "TrackChange", 
				G_CALLBACK(auCallbackTrackChange), db, NULL);
			
			//  player change
			dbus_g_proxy_add_signal(db->auPlayer, "CapsChange", 
				G_TYPE_INT, G_TYPE_INVALID);
			dbus_g_proxy_connect_signal(db->auPlayer, "CapsChange", 
				G_CALLBACK(auCallbackCapsChange), db, NULL);
			
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
				G_CALLBACK(auCallbackNameOwnerChanged),
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

	LOG("Leave auAssure\n");
	return bRet;
}

static gboolean auNext(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auNext\n");
	if( !auAssure(db) )
		return FALSE;
	if (!org_freedesktop_MediaPlayer_next (db->auPlayer, NULL)){
		LOGERR("Failed to complete Next\n");
	}
	LOG("Leave auNext\n");
	return TRUE;
}

static gboolean auPrevious(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auPrevious\n");
	if( !auAssure(db) )
		return FALSE;
	if (!org_freedesktop_MediaPlayer_prev (db->auPlayer, NULL)){
		LOGERR("Failed to complete Previous\n");
	}
	LOG("Leave auPrevious\n");
	return TRUE;
}

static gboolean auPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	if( !auAssure(db) )
		return FALSE;
	if( newState )
        org_freedesktop_MediaPlayer_play(db->auPlayer, NULL);
    else
        org_freedesktop_MediaPlayer_pause(db->auPlayer, NULL);
	return FALSE;
}

static gboolean auIsPlaying(gpointer thsPtr) {
	MKTHIS;
    gint status = 0;
	if( !auAssure(db) )
		return FALSE;
	if( !org_freedesktop_MediaPlayer_get_status(db->auPlayer, &status, NULL)){
		LOGERR("Failed to complete get_status\n");
		return FALSE;
	}
	return (status == 0);
}

static gboolean auToggle(gpointer thsPtr, gboolean *newState) {
    MKTHIS;
    gboolean oldState = FALSE;
    if( !auAssure(db) )
		return FALSE;
    oldState = auIsPlaying(db);
    auPlayPause(db, !oldState);
    if(newState)
        *newState = auIsPlaying(db);
    return TRUE;
}

static gboolean auDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter auDetach\n");
	if( db->auPlayer )
	{
		g_object_unref (G_OBJECT (db->auPlayer));		
		db->auPlayer = NULL;
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
	LOG("Leave auDetach\n");
	
	return TRUE;
}

void auPersist(gpointer thsPtr, XfceRc *rc, gboolean bIsStoring) {
    // no settings at the moment
	//MKTHIS;
}

auData * AU_attach(SPlayer *player) {
	auData *db = NULL;
	
	LOG("Enter AU_attach\n");
	AU_MAP(Assure);
	AU_MAP(Next);
	AU_MAP(Previous);
	AU_MAP(PlayPause);
	AU_MAP(IsPlaying);
	AU_MAP(Toggle);
	AU_MAP(Detach);
	AU_MAP(Persist);
    //The DBUS API does not yet provide:
       NOMAP(IsVisible);
       NOMAP(Show);
	   NOMAP(GetRepeat);
       NOMAP(SetRepeat);
       NOMAP(GetShuffle);
       NOMAP(SetShuffle);
	
	db = g_new0(auData, 1);
	db->parent = player;
	db->bus = NULL;
	db->auPlayer = NULL;
	db->noCreate = TRUE;
	
	// check if audacious is running
	
	if( auAssure(db) ){
        // emulate state change
        gint caps = 0;
        gint status = 0;
        GHashTable *metaData = NULL;
        if(org_freedesktop_MediaPlayer_get_caps(db->auPlayer, &caps, NULL)) {
            auCallbackCapsChange(db->auPlayer, caps, db);
        }
        if(org_freedesktop_MediaPlayer_get_status(db->auPlayer, &status, NULL)) {
            auCallbackStatusChange(db->auPlayer, status, db);
        }
        if(org_freedesktop_MediaPlayer_get_metadata(db->auPlayer, &metaData, NULL)) {
            auCallbackTrackChange(db->auPlayer, metaData, db);
            g_hash_table_unref(metaData);
        }
	}
	db->noCreate = FALSE;
	LOG("Leave AU_attach\n");
	return db;
}

#endif
