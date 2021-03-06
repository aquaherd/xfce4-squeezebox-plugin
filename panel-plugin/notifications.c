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

void squeezebox_update_UI_show_toaster(gpointer thsPlayer) {
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	gboolean bAct = TRUE;
	GdkPixbuf *pixbuf = NULL;
	gchar *pixPath = "/tmp/sqicon.png"; //use g_get_tmp_dir();
	GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
	LOG("show_toaster ");

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

	if (bAct) {
		bAct = ((sd->player.title->str && sd->player.title->str[0]) ||
		        (sd->player.album->str && sd->player.album->str[0]) ||
		        (sd->player.artist->str && sd->player.artist->str[0])
		       );
	}

	if (bAct) {
		GString *albumArt = g_string_new(sd->player.albumArt->str);
		gchar *ntTitle = g_strdup(sd->player.title->str);
		//happily, we easily can escape ampersands and other usual suspects.
		gchar *ntDetails =
		    g_markup_printf_escaped(_("by <b>%s</b>\nfrom <i>%s</i>"),
		                            sd->player.artist->str,
		                            sd->player.album->str);
		LOG("New track '%s' from '%s' by '%s'",
		    sd->player.title->str, sd->player.artist->str,
		    sd->player.album->str);

		if (albumArt->len > 0) {
			pixbuf =
			    gdk_pixbuf_new_from_file_at_size(albumArt->str, 64, 64, NULL);
		}
		if(NULL == pixbuf) {
			pixbuf = sd->current->BACKEND_icon();
		}
		if(NULL != pixbuf) {
			gdk_pixbuf_save(pixbuf, pixPath, "png", NULL, NULL);
			g_object_unref(pixbuf);
		} else
			pixPath = NULL;


		//Let's show
		if (sd->note) {
			LOG("notify-ignition %d", sd->notifyID);
			//if(sd->notifyID > 0)
				//notifications_close_notification(sd->note, sd->notifyID, NULL);
			notifications_notify(sd->note,
			                     "xfce4-squeezebox-plugin", sd->notifyID, pixPath,
			                     ntTitle, ntDetails, NULL, table, sd->notifyTimeout * 1000,
			                     &sd->notifyID, NULL);
		}
		g_free(ntTitle);
		g_free(ntDetails);
		g_string_free(albumArt, TRUE);
	}
}
