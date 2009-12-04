/***************************************************************************
 *            squeezebox.h - frontend of xfce4-squeezebox-plugin
 *
 *  Wed Aug 30 17:37:58 2006
 *  Copyright  2006  Hakan Erduman
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

#ifndef XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER
#define XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER

// stdafx.hish
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gmodule.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#define EXPORT __attribute__ ((visibility("default")))
#define HIDDEN __attribute__ ((visibility("hidden")))

typedef enum eSynoptics{
	estPlay = 0,
	estPause = 1,
	estStop = 2,
	estErr = 10
}eSynoptics;

#define NOMAP(a) parent->a = NULL

typedef struct SPlayer{
	// this is the 'API'
	
    // yet unused
    gint updateRateMS;
    gint secPos;     // Current track position in seconds
    gint secTot;     // Current track length in seconds
	
	// backend implementations
	gboolean(* Assure)(gpointer thsPtr, gboolean noCreate);
	gboolean(* Next)(gpointer thsPtr);
	gboolean(* Previous)(gpointer thsPtr);
	gboolean(* PlayPause)(gpointer thsPtr, gboolean newState);
	gboolean(* PlayPlaylist)(gpointer thsPtr, gchar *playListName);
	gboolean(* IsPlaying)(gpointer thsPtr);
	gboolean(* Toggle)(gpointer thsPtr, gboolean *newState);
	gboolean(* Detach)(gpointer thsPtr);
    gboolean(* GetRepeat)(gpointer thsPtr);
    gboolean(* SetRepeat)(gpointer thsPtr, gboolean newShuffle);
    gboolean(* GetShuffle)(gpointer thsPtr);
    gboolean(* SetShuffle)(gpointer thsPtr, gboolean newShuffle);
	gboolean(* IsVisible)(gpointer thsPtr);
	gboolean(* Show)(gpointer thsPtr, gboolean newState);
    gboolean(* UpdateDBUS)(gpointer thsPtr, gboolean appeared);
	void(* Persist)(gpointer thsPtr, gboolean bIsStoring);
	void(* Configure)(gpointer thsPtr, GtkWidget *parent);
	void(* UpdateWindow)(gpointer thsPtr, WnckWindow *window, gboolean appeared);
	
	// data provided by backend
	GString *artist;
	GString *album;
	GString *title;
	GString *albumArt;  // path to image file
	GHashTable *playLists; // Playlistname:State
	
	// backend "this" pointer, first param of above functions
	gpointer db;
	
	
    // frontend "this" pointer
	gpointer sd;

    // frontend globals
	DBusGConnection *bus;
    DBusGProxy		*dbService;
    gboolean(* StartService)(gpointer thsPlayer);
	guint playerPID;

	// frontend callbacks
	void(* Update)(gpointer thsPlayer, gboolean SongChanged, eSynoptics State, 
                   const gchar* playerMessage);
    void(* UpdatePlaylists)(gpointer thsPlayer);
    void(* UpdateRepeat)(gpointer thsPlayer, gboolean newRepeat);
    void(* UpdateShuffle)(gpointer thsPlayer, gboolean newShuffle);
    void(* UpdateVisibility)(gpointer thsPlayer, gboolean newVisibility);
    void(* AddSubItem)(gpointer thsPlayer, gpointer newPlayer);
    void(* FindAlbumArtByFilePath)(gpointer thsPlayer, const gchar * path);
    void(* MapProperty)(gpointer thsPlayer, const gchar *propName, gpointer address);
}SPlayer;

// Backend definitions
typedef enum eBackendType{
	dbusBackend,
	networkBackend,
	otherBackend,
	numBackendTypes
}eBackendType;

#define DEFINE_DBUS_BACKEND(t,n,d,c)  \
    G_MODULE_EXPORT const gchar* t##_name(){ \
        return _(n); \
    } \
    GdkPixbuf *t##_icon(){ \
		return gdk_pixbuf_new_from_file(#n ".png", NULL); \
    } \
    const gchar* t##_dbusName(){ \
        return d; \
    } \
    const gchar* t##_commandLine(){ \
    	return c; \
	}    
#define DEFINE_BACKEND(t,n) \
    G_MODULE_EXPORT const gchar* t##_name(){ \
        return _(n); \
    } \
    GdkPixbuf *t##_icon(){ \
		return gdk_pixbuf_new_from_file(#n ".png", NULL); \
    }
    

#if DEBUG_TRACE
#define LOGERR g_error
#define LOGWARN g_warning
#define LOG g_message
#else
#define LOG(...)
#define LOGERR(...)
#define LOGWARN(...)
#endif
	
    
#endif //defined XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER
