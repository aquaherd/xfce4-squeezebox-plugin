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
#include "mmkeys.h"
#if HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

typedef struct SqueezeBoxData{
	XfcePanelPlugin *plugin;
	gulong style_id;

	GtkWidget *button[3];
	GtkWidget *image[3];
	GtkWidget *btnDet;
	gboolean show[3];

	GtkWidget *table;

#if HAVE_NOTIFY
	gboolean notify;
	gboolean inEnter;
	gboolean inCreate;
	NotifyNotification *note;
	gint notifytimeout;
	gint timerCount;
	guint timerHandle;
#endif

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

	// mmkeys.h
	MmKeys *mmkeys;
	gboolean grabmedia;
	gulong mmhandlers[4];
    
    // property handling
    GHashTable *properties;
    GHashTable *propertyAddresses;
    
    WnckScreen *wnckScreen;
} SqueezeBoxData;



#endif

