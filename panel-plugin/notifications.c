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


#include "notifications.h"

#if HAVE_NOTIFY

void toaster_closed(NotifyNotification * notification, SqueezeBoxData * sd) {
	LOG("toaster_closed");
	sd->note = NULL;
}

void squeezebox_update_UI_hide_toaster(gpointer thsPlayer) {
	LOG("hide_toaster");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	if (sd->note) {
		notify_notification_close(sd->note, NULL);
		sd->note = NULL;
	}
}

gboolean on_timer(gpointer thsPlayer) {
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	if (NULL == sd->note) {
		return TRUE;
	}
	LOG("CountDown %d %d", sd->timerCount, sd->notifytimeout);
	if (sd->inEnter)
		sd->timerCount = sd->notifytimeout;
	else {
		sd->timerCount--;
		if (sd->timerCount < 1)
			squeezebox_update_UI_hide_toaster(thsPlayer);
	}

	return TRUE;
}


void squeezebox_update_UI_show_toaster(gpointer thsPlayer) {
	LOG("show_toaster ");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	gboolean bAct = TRUE;
	gboolean bExisted = (sd->note != NULL);
	GdkPixbuf *pixbuf = NULL;
	if (!notify_is_initted())
		if (!notify_init("xfce4-squeezebox-plugin"))
			bAct = FALSE;

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
		    g_markup_printf_escaped("by <b>%s</b>\nfrom <i>%s</i>\n",
					    sd->player.artist->str,
					    sd->player.album->str);

		if (albumArt->len) {
			pixbuf =
			    gdk_pixbuf_new_from_file_at_size(albumArt->str,
							     64,
							     64, NULL);
		}
		/*
		if(NULL == pixbuf) {
			const Backend *ptr = &squeezebox_get_backends()[sd->backend -1];
			pixbuf = ptr->BACKEND_icon();
		}
		*/
		//squeezebox_update_UI_hide_toaster(thsPlayer);
		if (!bExisted) {
			LOG("new");
			sd->note = notify_notification_new(ntTitle,
							   ntDetails, NULL,
							   NULL);
			g_signal_connect(G_OBJECT(sd->note), "closed",
					 G_CALLBACK(toaster_closed), sd);

		} else if (sd->note) {
			LOG("update");
			notify_notification_update(sd->note, ntTitle, ntDetails,
						   NULL);
		}
		if (sd->note) {
			gint x = 0, y = 0;
			GtkRequisition size;
			XfceScreenPosition pos =
			    xfce_panel_plugin_get_screen_position(sd->plugin);

			gdk_window_get_origin(GTK_WIDGET(sd->plugin)->window,
					      &x, &y);
			gtk_widget_size_request(GTK_WIDGET(sd->plugin), &size);
			x += size.width / 2;
			if (!xfce_screen_position_is_bottom(pos))
				y += size.height;

			notify_notification_set_hint_int32(sd->note, "x", x);
			notify_notification_set_hint_int32(sd->note, "y", y);

			//timeout? never - only on our control
			notify_notification_set_timeout(sd->note, 0);

			// did we get an icon?
			if (pixbuf) {
				LOG("We got an icon.");
#if (LIBNOTIFY_VERSION_MAJOR == 0 && \
    LIBNOTIFY_VERSION_MINOR <=6 && \
    LIBNOTIFY_VERSION_MICRO < 0)
				notify_notification_set_icon_data_from_pixbuf
				    (sd->note, pixbuf);
#else
				notify_notification_set_icon_from_pixbuf
				    (sd->note, pixbuf);
#endif
				g_object_unref(pixbuf);
			}
			// liftoff      
			if (sd->inCreate == FALSE)
				notify_notification_show(sd->note, NULL);

			sd->timerCount = sd->notifytimeout;
		}
		g_free(ntTitle);
		g_free(ntDetails);
		g_string_free(albumArt, TRUE);
	}
}
#endif

