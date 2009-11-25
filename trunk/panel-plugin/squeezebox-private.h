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
	GtkWidget *mnuShuffle, *mnuRepeat, *mnuPlayer, *mnuPlayLists;
	gboolean noUI;

	gint toolTipStyle;
#ifndef HAVE_GTK_2_12
	GtkTooltips *tooltips;
#endif
	GString *toolTipText;

	gint backend;
	SPlayer player;
	eSynoptics state;

	// shortcuts
	gboolean grabmedia;
    
    // property handling
    GHashTable *properties;
    GHashTable *propertyAddresses;
    
    WnckScreen *wnckScreen;
    
    // settings dialog
    XfconfChannel *channel;
    GtkWidget *dlg;
} SqueezeBoxData;

static void config_toggle_next(GtkToggleButton * tb, SqueezeBoxData * sd);
static void config_toggle_prev(GtkToggleButton * tb, SqueezeBoxData * sd);
static void squeezebox_dbus_update(DBusGProxy * proxy, const gchar * Name,
				   const gchar * OldOwner,
				   const gchar * NewOwner,
				   SqueezeBoxData * sd);

void on_keyPrev_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
void on_keyStop_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
void on_keyPlay_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
void on_keyNext_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
void squeezebox_ungrab_key(guint accel_key, guint accel_mods, SqueezeBoxData *sd);

/* Panel Plugin Interface */

static void squeezebox_properties_dialog(XfcePanelPlugin * plugin,
					 SqueezeBoxData * sd);
static void squeezebox_construct(XfcePanelPlugin * plugin);

const Backend* squeezebox_get_backends();


#endif

