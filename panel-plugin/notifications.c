/*
 *      notifications.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "notifications.h"
#if HAVE_DBUS

void squeezebox_update_UI_hide_toaster(gpointer thsPlayer) {
	LOG("hide_toaster");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	/*
	if (sd->note) {
		notify_notification_close(sd->note, NULL);
		sd->note = NULL;
	}
	*/
}

void squeezebox_update_UI_show_toaster(gpointer thsPlayer) {
	/*
	 "xfce", 
	 12, 
	 "/home/herd/Music/Faith No More/The Real Thing/folder.jpg", 
	 "Faith no more", 
	 "The Real Thing<br>The Real Thing",
	 "",
	 "",
	 1200	 
	 */

	LOG("show_toaster ");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	gboolean bAct = TRUE;
	gboolean bExisted = (sd->note != NULL);
	GdkPixbuf *pixbuf = NULL;
	gchar *pixPath = "/tmp/sqicon.png";

	if (bAct) {
		bAct = ((sd->player.title->str && sd->player.title->str[0]) ||
			(sd->player.album->str && sd->player.album->str[0]) ||
			(sd->player.artist->str && sd->player.artist->str[0])
		    );
	}

	if (bAct) {
		LOG("New track '%s' from '%s' by '%s'\n",
			sd->player.title->str, sd->player.artist->str,
			sd->player.album->str);
		GString *albumArt = g_string_new(sd->player.albumArt->str);
		gchar *ntTitle = g_strdup(sd->player.title->str);
		//happily, we easily can escape ampersands and other usual suspects.
		gchar *ntDetails =
		    g_markup_printf_escaped("by <b>%s</b><br>from <i>%s</i>",
					    sd->player.artist->str,
					    sd->player.album->str);

		if (albumArt->len) {
			pixbuf =
			    gdk_pixbuf_new_from_file_at_size(albumArt->str, 64, 64, NULL);
		}
		if(NULL == pixbuf) {
			const Backend *ptr = &squeezebox_get_backends()[sd->backend -1];
			pixbuf = ptr->BACKEND_icon();
		}
		if(NULL != pixbuf)
			gdk_pixbuf_save(pixbuf, "/tmp/sqicon.png", "png", NULL, NULL);
		else
			pixPath = NULL;
		
		
		//Let's show
		if (sd->note) {
			LOG("notify");
			if(sd->notifyID > 0)
				notifications_close_notification(sd->note, sd->notifyID, NULL);
			notifications_notify(sd->note, 
				"xfce4-squeezebox-plugin", ++sd->notifyID, pixPath, 
				ntTitle, ntDetails, NULL, NULL, sd->notifyTimeout * 1000, 
				&sd->notifyID, NULL);
		} 
		g_free(ntTitle);
		g_free(ntDetails);
		g_string_free(albumArt, TRUE);
	}
}

#endif
