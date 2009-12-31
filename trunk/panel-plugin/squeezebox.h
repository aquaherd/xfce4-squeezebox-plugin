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
    gboolean(* UpdateDBUS)(gpointer thsPtr, const gchar *name, gboolean appeared);
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
	gint playerPID;

	// frontend callbacks
    gboolean(* StartService)(gpointer thsPlayer, const gchar* serviceName);
    gboolean(* IsServiceRunning)(gpointer thsPlayer, const gchar* dbusName);
	void(* Update)(gpointer thsPlayer, gboolean SongChanged, eSynoptics State, 
                   const gchar* playerMessage);
    void(* UpdatePlaylists)(gpointer thsPlayer);
    void(* UpdateRepeat)(gpointer thsPlayer, gboolean newRepeat);
    void(* UpdateShuffle)(gpointer thsPlayer, gboolean newShuffle);
    void(* UpdateVisibility)(gpointer thsPlayer, gboolean newVisibility);
    void(* AddSubItem)(gpointer thsPlayer, gpointer newPlayer);
    void(* FindAlbumArtByFilePath)(gpointer thsPlayer, const gchar * path);
}SPlayer;

// Backend definitions
typedef enum eBackendType{
	noMoreBackends = 0,
	dbusBackend,
	networkBackend,
	wizard,
	otherBackend,
	numBackendTypes
}eBackendType;

typedef struct Backend{
    const gchar* basename;
	const eBackendType BACKEND_TYPE;
    gpointer( *BACKEND_attach)(SPlayer *player);
    const gchar*( *BACKEND_name)();
    GdkPixbuf*( *BACKEND_icon)();
    const gchar**( *BACKEND_dbusNames)();
    const gchar*( *BACKEND_commandLine)();
}Backend;

#define DEFINE_DBUS_BACKEND(t,n,d,c)  \
	const Backend *backend_info(void); \
    static const gchar* t##_name(void){ \
        return _(n); \
    } \
    static GdkPixbuf *t##_icon(void){ \
		return gdk_pixbuf_new_from_file(BACKENDDIR "/" BASENAME "/" BASENAME ".png", NULL); \
    } \
    static const gchar** t##_dbusNames(void){ \
        static const gchar* names[] = {d}; \
		return &names[0]; \
    } \
    static const gchar* t##_commandLine(void){ \
    	return c; \
	} \
	EXPORT const Backend *backend_info(void) { \
		static const Backend backend[2] = { \
			{BASENAME, dbusBackend, t##_attach, t##_name, \
			t##_icon, t##_dbusNames, t##_commandLine}, \
			{NULL} \
		}; \
		return &backend[0]; \
	}
#define DEFINE_BACKEND(t,n) \
	const Backend *backend_info(void); \
    static const gchar* t##_name(void){ \
        return _(n); \
    } \
    static GdkPixbuf *t##_icon(void){ \
		return gdk_pixbuf_new_from_file(BACKENDDIR "/" BASENAME "/" BASENAME ".png", NULL); \
    } \
	EXPORT const Backend *backend_info(void) { \
		static const Backend backend[2] = { \
			{BASENAME, networkBackend, t##_attach, t##_name, \
			t##_icon, NULL, NULL}, \
			{NULL} \
		}; \
		return &backend[0]; \
	}
    

#if DEBUG_TRACE
#define LOG g_message
#define LOGERR g_error
#define LOGWARN g_warning
#else
#define LOG(...)
#define LOGERR(...)
#define LOGWARN(...)
#endif
	
    
#endif //defined XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER
