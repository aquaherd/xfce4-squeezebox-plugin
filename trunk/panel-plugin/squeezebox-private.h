/*
 *      squeezebox-private.h
 *      
 *      Copyright 2009 Hakan Erduman <hakan@erduman.de>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


#ifndef SQUEEZEBOX_PRIVATE_H
#define SQUEEZEBOX_PRIVATE_H

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#define EXO_API_SUBJECT_TO_CHANGE
#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>
// have local copy until it's standardized
#include "xfce-shortcuts-grabber.h"
#define xfce_screen_position_is_right_ex(position) \
    (position >= XFCE_SCREEN_POSITION_NE_V && \
     position <= XFCE_SCREEN_POSITION_SE_V) || \
	(position == XFCE_SCREEN_POSITION_NE_H) || \
	(position == XFCE_SCREEN_POSITION_SE_H) || \
	(position == XFCE_SCREEN_POSITION_E)


static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_get_connection_unix_process_id (DBusGProxy *proxy, const char * IN_arg0, guint* OUT_arg1, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetConnectionUnixProcessID", error, G_TYPE_STRING, IN_arg0, G_TYPE_INVALID, G_TYPE_UINT, OUT_arg1, G_TYPE_INVALID);
}

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_get_name_owner (DBusGProxy *proxy, const char * IN_arg0, char ** OUT_arg1, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetNameOwner", error, G_TYPE_STRING, IN_arg0, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg1, G_TYPE_INVALID);
}

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_start_service_by_name (DBusGProxy *proxy, const char * IN_arg0, const guint IN_arg1, guint* OUT_arg2, GError **error)

{
  return dbus_g_proxy_call (proxy, "StartServiceByName", error, G_TYPE_STRING, IN_arg0, G_TYPE_UINT, IN_arg1, G_TYPE_INVALID, G_TYPE_UINT, OUT_arg2, G_TYPE_INVALID);
}

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_name_has_owner (DBusGProxy *proxy, const char * IN_arg0, gboolean* OUT_arg1, GError **error)

{
  return dbus_g_proxy_call (proxy, "NameHasOwner", error, G_TYPE_STRING, IN_arg0, G_TYPE_INVALID, G_TYPE_BOOLEAN, OUT_arg1, G_TYPE_INVALID);
}


#define BEGIN_BACKEND_MAP() const Backend* squeezebox_get_backends() \
{ \
    static const Backend ret[] = { 
#define BACKEND(t) {otherBackend, t##_attach, t##_name, t##_icon, NULL},
#define DBUS_BACKEND(t) {dbusBackend, t##_attach, t##_name, t##_icon, t##_dbusNames, t##_commandLine},
#define END_BACKEND_MAP() \
        {numBackendTypes, NULL} \
    }; \
    return &ret[0]; \
}

typedef struct SqueezeBoxData{
	XfcePanelPlugin *plugin;
	gulong style_id;

	GtkWidget *button[3];
	GtkWidget *image[3];
	GtkWidget *btnDet;
	gboolean show[3];

	GtkWidget *table;

	gboolean autoAttach;
	gboolean notify;
	gboolean inEnter;
	gboolean inCreate;
	DBusGProxy *note;
	gint notifyTimeout;
	guint notifyID;
	// menu items
	GtkWidget 
		*mnuShuffle,   // toggle shuffle
		*mnuRepeat,    // toggle repeat
		*mnuPlayer,    // show/hide player
		*mnuPlayLists, // expand playlists
		*mnuSong;	   // show song in thunar
	gboolean noUI;

	gint toolTipStyle;
	GString *toolTipText;

	SPlayer player;
	eSynoptics state;

	// shortcuts
	gboolean grabmedia;
	XfceShortcutsGrabber *grabber;
	GQuark shortcuts[5];
    
    WnckScreen *wnckScreen;
    
    // settings dialog
    XfconfChannel *channel;
    GtkWidget *dlg;
	
	// dynamic list of backends
	GList *list;
	const Backend *current;
	GModule *module;

} SqueezeBoxData;

typedef struct BackendCache{
	eBackendType type;
	gchar *basename; // real name of folder, library, icon
    gchar *name; // localized display name
	gchar *path; // full path to module
    GdkPixbuf *icon; 
    gchar **dbusNames;
    gchar *commandLine;
	gboolean autoAttach;
}BackendCache;
gboolean squeezebox_dbus_service_exists(gpointer thsPlayer, const gchar* dbusName);
void squeezebox_dbus_update(DBusGProxy * proxy, const gchar * Name,
				   const gchar * OldOwner,
				   const gchar * NewOwner,
				   SqueezeBoxData * sd);

EXPORT void on_keyPrev_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
EXPORT void on_keyStop_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
EXPORT void on_keyPlay_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
EXPORT void on_keyNext_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
EXPORT void on_dialogSettings_response(GtkWidget * dlg, int reponse, SqueezeBoxData * sd);
EXPORT void on_btnAdd_clicked(GtkButton * btn, SqueezeBoxData * sd);
EXPORT void on_btnRemove_clicked(GtkButton * btn, SqueezeBoxData * sd);
EXPORT void on_btnEdit_clicked(GtkButton * btn, SqueezeBoxData * sd);
EXPORT void on_tvPlayers_cursor_changed(GtkTreeView * tv, SqueezeBoxData * sd);
EXPORT void on_cellrenderertoggle1_toggled(GtkCellRendererToggle * crt, 
		gchar                *path_string,
		SqueezeBoxData * sd);
EXPORT void on_cellrenderShortCut_accel_cleared(GtkCellRendererAccel *accel,
		gchar                *path_string,
		SqueezeBoxData		 *sd);
EXPORT void on_cellrenderShortCut_accel_edited(GtkCellRendererAccel *accel,
		gchar                *path_string,
		guint                 accel_key,
		GdkModifierType       accel_mods,
		guint                 hardware_keycode,
		SqueezeBoxData		 *sd);
		
EXPORT void on_chkAutoAttach_toggled(GtkToggleButton *button, SqueezeBoxData * sd);
EXPORT void on_chkShowPrevious_toggled(GtkToggleButton *button, SqueezeBoxData *sd);
EXPORT void on_chkShowNext_toggled(GtkToggleButton *button, SqueezeBoxData *sd);
EXPORT void on_chkShowToolTips_toggled(GtkToggleButton *button, SqueezeBoxData *sd);
EXPORT void on_chkShowNotifications_toggled(GtkToggleButton *button, SqueezeBoxData *sd);
EXPORT void on_spinNotificationTimeout_change_value(GtkSpinButton *button, SqueezeBoxData *sd);

				
gboolean squeezebox_grab_key(guint accel_key, guint accel_mods, SqueezeBoxData *sd);
void squeezebox_ungrab_key(guint accel_key, guint accel_mods, SqueezeBoxData *sd);

/* Panel Plugin Interface */

void squeezebox_properties_dialog(XfcePanelPlugin * plugin,
					 SqueezeBoxData * sd);
EXPORT void squeezebox_construct(XfcePanelPlugin * plugin);

const Backend* squeezebox_get_backends();
const Backend* squeezebox_get_current_backend(SqueezeBoxData * sd);
const Backend *squeezebox_load_backend(SqueezeBoxData * sd, const gchar *name);

#endif

