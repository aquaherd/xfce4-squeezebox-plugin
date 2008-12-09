/***************************************************************************
 *            squeezebox.h - frontend of xfce4-squeezebox-plugin
 *
 *  Wed Aug 30 17:37:58 2006
 *  Copyright  2006  Hakan Erduman
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

#ifndef XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER
#define XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER

// stdafx.hish
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#if HAVE_DBUS
#include <dbus/dbus-glib.h>
#endif

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#define EXO_API_SUBJECT_TO_CHANGE
#include <exo/exo.h>
typedef enum {
	estPlay = 0,
	estPause = 1,
	estStop = 2,
	estErr = 10
}eSynoptics;

#define NOMAP(a) player->a = NULL;

typedef struct {
	// this is the 'API'
	
    // yet unused
    gint updateRateMS;
    gint secPos;     // Current track position in seconds
    gint secTot;     // Current track length in seconds
	
	// backend implementations
	gboolean(* Assure)(gpointer thsPtr);
	gboolean(* Next)(gpointer thsPtr);
	gboolean(* Previous)(gpointer thsPtr);
	gboolean(* PlayPause)(gpointer thsPtr, gboolean newState);
	gboolean(* IsPlaying)(gpointer thsPtr);
	gboolean(* Toggle)(gpointer thsPtr, gboolean *newState);
	gboolean(* Detach)(gpointer thsPtr);
    gboolean(* GetRepeat)(gpointer thsPtr);
    gboolean(* SetRepeat)(gpointer thsPtr, gboolean newShuffle);
    gboolean(* GetShuffle)(gpointer thsPtr);
    gboolean(* SetShuffle)(gpointer thsPtr, gboolean newShuffle);
	gboolean(* IsVisible)(gpointer thsPtr);
	gboolean(* Show)(gpointer thsPtr, gboolean newState);
#if HAVE_DBUS
    gboolean(* UpdateDBUS)(gpointer thsPtr, gboolean appeared);
#endif
	void(* Persist)(gpointer thsPtr, XfceRc *rc, gboolean bIsStoring);
	void(* Configure)(gpointer thsPtr, GtkWidget *parent);
	
	// data provided by backend
	GString *artist;
	GString *album;
	GString *title;
	GString *albumArt;  // path to image file
	
	// backend "this" pointer, first param of above functions
	gpointer db;
	
	
    // frontend "this" pointer
	gpointer sd;

    // frontend globals
#if HAVE_DBUS
	DBusGConnection *bus;
    DBusGProxy		*dbService;
#endif

	// frontend callbacks
	void(* Update)(gpointer thsPlayer, gboolean SongChanged, eSynoptics State, 
                   const gchar* playerMessage);
    void(* UpdateRepeat)(gpointer thsPlayer, gboolean newRepeat);
    void(* UpdateShuffle)(gpointer thsPlayer, gboolean newShuffle);
    void(* UpdateVisibility)(gpointer thsPlayer, gboolean newVisibility);
    void(* AddSubItem)(gpointer thsPlayer, gpointer newPlayer);
    void(* MonitorFile)(gpointer thsPlayer, GString filePath); 
    void(* FindAlbumArtByFilePath)(gpointer thsPlayer, const gchar * path);
}SPlayer;

// Backend definitions

typedef struct {
    const gchar* Name;
    gint Type;
    gint Offset;
}PropDef;
 
typedef struct {
    void*( *BACKEND_attach)(SPlayer *player);
    gchar*( *BACKEND_name)();
    GdkPixbuf*( *BACKEND_icon)();
    PropDef*( *BACKEND_properties)();
    gchar*( *BACKEND_dbusName)();
}Backend;

#define IMPORT_BACKEND(t) \
 	extern void * t##_attach(SPlayer *player); \
 	extern gchar * t##_name(); \
    extern GdkPixbuf * t##_icon(); \
    extern PropDef* t##_properties();
 
#define IMPORT_DBUS_BACKEND(t) \
 	extern void * t##_attach(SPlayer *player); \
 	extern gchar * t##_name(); \
    extern GdkPixbuf * t##_icon(); \
    extern PropDef* t##_properties(); \
    extern gchar * t##_dbusName();

extern const Backend* squeezebox_get_backends();

#define BEGIN_BACKEND_MAP() const Backend* squeezebox_get_backends() \
{ \
    static const Backend ret[] = { 
#define BACKEND(t) {t##_attach, t##_name, t##_icon, t##_properties, NULL},
#define DBUS_BACKEND(t) {t##_attach, t##_name, t##_icon, t##_properties, t##_dbusName},
#define END_BACKEND_MAP() \
        {NULL, NULL} \
    }; \
    return &ret[0]; \
}

#define DEFINE_BACKEND(t,n) gchar* t##_name(){ return _(n);} \
    GdkPixbuf *t##_icon(){ return gdk_pixbuf_new_from_inline( \
        sizeof(my_pixbuf), my_pixbuf, TRUE, NULL); }
    
#define DEFINE_DBUS_BACKEND(t,n,d) gchar* t##_name(){ return _(n);} \
    GdkPixbuf *t##_icon(){ return gdk_pixbuf_new_from_inline( \
        sizeof(my_pixbuf), my_pixbuf, TRUE, NULL); } \
    gchar *t##_dbusName(){return d;}
    
// Properties

#define BEGIN_PROP_MAP(bk) const PropDef* bk##_properties() \
{ \
    static const PropDef props[] = {
#define PROP_ENTRY(name,type,struct,member) {name,type,G_STRUCT_OFFSET(struct,member)},
#define END_PROP_MAP() {"",0,0} \
    }; \
    return &props[0]; \
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
	
#define xfce_screen_position_is_right_ex(position) \
    (position >= XFCE_SCREEN_POSITION_NE_V && \
     position <= XFCE_SCREEN_POSITION_SE_V) || \
	(position == XFCE_SCREEN_POSITION_NE_H) || \
	(position == XFCE_SCREEN_POSITION_SE_H) || \
	(position == XFCE_SCREEN_POSITION_E)

    
#endif //defined XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER
