/***************************************************************************
 *            squeezebox.c - frontend of xfce4-squeezebox-plugin
 *
 *  Fri Aug 25 17:20:09 2006
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "squeezebox.h"
#include "mmkeys.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

#include <libintl.h>

typedef enum tag_eButtons {
	ebtnPrev = 0,
	ebtnPlay = 1,
	ebtnNext = 2
} eButtons;

typedef enum tag_eTTStyle {
	ettNone = 0,
	ettSimple = 1,
#if HAVE_NOTIFY
	ettFull = 2
#endif
} eTTStyle;

typedef struct {
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
	GtkWidget *mnuShuffle, *mnuRepeat, *mnuPlayer;
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
} SqueezeBoxData;

/* some small helpers - unused for now
static void lose (const char *fmt, ...) G_GNUC_NORETURN G_GNUC_PRINTF (1, 2);
static void lose_gerror (const char *prefix, GError *error) G_GNUC_NORETURN;
*/
static void config_toggle_next(GtkToggleButton * tb, SqueezeBoxData * sd);
static void config_toggle_prev(GtkToggleButton * tb, SqueezeBoxData * sd);
static void
squeezebox_update_grab(gboolean bGrab, gboolean bShowErr, SqueezeBoxData * sd);

void on_keyPrev_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
void on_keyStop_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
void on_keyPlay_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);
void on_keyNext_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd);

/* Panel Plugin Interface */

static void squeezebox_properties_dialog(XfcePanelPlugin * plugin,
					 SqueezeBoxData * sd);
static void squeezebox_construct(XfcePanelPlugin * plugin);

//XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (squeezebox_construct);

/* Backend mapping */
#if HAVE_BACKEND_RHYTHMBOX
IMPORT_DBUS_BACKEND(RB)
#endif
#if HAVE_BACKEND_MPD
    IMPORT_BACKEND(MPD)
#endif
#if HAVE_BACKEND_QUODLIBET
    IMPORT_BACKEND(QL)
#endif
#if HAVE_BACKEND_AUDACIOUS
    IMPORT_DBUS_BACKEND(AU)
#endif
#if HAVE_BACKEND_EXAILE
    IMPORT_DBUS_BACKEND(EX)
#endif
#if HAVE_BACKEND_CONSONANCE
    IMPORT_DBUS_BACKEND(CS)
#endif
    BEGIN_BACKEND_MAP()
#if HAVE_BACKEND_RHYTHMBOX
    DBUS_BACKEND(RB)
#endif
#if HAVE_BACKEND_MPD
    BACKEND(MPD)
#endif
#if HAVE_BACKEND_QUODLIBET
    BACKEND(QL)
#endif
#if HAVE_BACKEND_AUDACIOUS
    DBUS_BACKEND(AU)
#endif
#if HAVE_BACKEND_EXAILE
    DBUS_BACKEND(EX)
#endif
#if HAVE_BACKEND_CONSONANCE
    DBUS_BACKEND(CS)
#endif
    END_BACKEND_MAP()

/* internal functions */
#define UNSET(t) sd->player.t = NULL
static void squeezebox_init_backend(SqueezeBoxData * sd, gint nBackend) {
	// clear previous backend
	const Backend *ptr = squeezebox_get_backends();
    PropDef *prop = NULL;
    LOG("Enter init backend");
    if (sd->player.Detach) {
		sd->player.Detach(sd->player.db);
		g_free(sd->player.db);
	}
	UNSET(Assure);
	UNSET(Next);
	UNSET(Previous);
	UNSET(PlayPause);
	UNSET(IsPlaying);
	UNSET(Toggle);
	UNSET(Detach);
	UNSET(GetRepeat);
	UNSET(SetRepeat);
	UNSET(GetShuffle);
	UNSET(SetShuffle);
	UNSET(IsVisible);
	UNSET(Show);
	UNSET(Persist);
	UNSET(Configure);
    
    // clear old property address map
    if(g_hash_table_size(sd->propertyAddresses)) {
        g_hash_table_remove_all(sd->propertyAddresses);
    }
    

	// clear current song info
	g_string_set_size(sd->player.artist, 0);
	g_string_set_size(sd->player.album, 0);
	g_string_set_size(sd->player.title, 0);
	g_string_set_size(sd->player.albumArt, 0);

	// call init of backend
	sd->backend = nBackend;
	sd->player.db = ptr[nBackend - 1].BACKEND_attach(&sd->player);

	// have menu populated
	gtk_widget_set_sensitive(sd->mnuPlayer, (NULL != sd->player.Show));
	gtk_check_menu_item_set_inconsistent(GTK_CHECK_MENU_ITEM(sd->mnuPlayer),
					     (NULL == sd->player.IsVisible));
	gtk_widget_set_sensitive(sd->mnuRepeat,
				 (NULL != sd->player.GetRepeat
				  && NULL != sd->player.SetRepeat));
	gtk_widget_set_sensitive(sd->mnuShuffle,
				 (NULL != sd->player.GetShuffle
				  && NULL != sd->player.SetShuffle));

    // apply new properties, if applicable
    prop = ptr[nBackend - 1].BACKEND_properties();  
    while(*prop->Name) {
        gchar *propValue = NULL;
        if(g_hash_table_lookup_extended(sd->properties, prop->Name, NULL, (gpointer)&propValue)) {
            switch(prop->Type) {
                case G_TYPE_BOOLEAN: {
                    gboolean *boolVal = (gboolean*)g_hash_table_lookup(sd->propertyAddresses, prop->Name);
                    if(boolVal)
                        *boolVal = ('0' == propValue[0]) ? FALSE : TRUE;
                }break;
                case G_TYPE_INT: {
                    gint *gintVal = (gint*)g_hash_table_lookup(sd->propertyAddresses, prop->Name);
                    if(gintVal)
                        *gintVal = (gint)g_ascii_strtoll(propValue, NULL, 10);
                }break;
                case G_TYPE_STRING: {
                    GString *strVal = (GString*)g_hash_table_lookup(sd->propertyAddresses, prop->Name);
                    if(strVal)
                        g_string_assign(strVal, propValue);
                }break;
            }
            //? g_free(propValue);
        }
        prop++;
    }
    
	// try connect happens in created backend
    LOG("Leave init backend");
}

static void squeezebox_update_playbtn(SqueezeBoxData * sd) {
	LOG("Enter squeezebox_update_playbtn");
	/* // hmmm.
	   GtkIconTheme *theme = gtk_icon_theme_get_default();
	   GError *err = NULL;
	   gint size = xfce_panel_plugin_get_size(sd->plugin) - 2;
	 */
	gtk_widget_destroy(sd->image[ebtnPlay]);

	switch (sd->state) {
	    case estPlay:
		    sd->image[ebtnPlay] =
			gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY,
						 GTK_ICON_SIZE_MENU);
		    break;
	    case estPause:
		    sd->image[ebtnPlay] =
			gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE,
						 GTK_ICON_SIZE_MENU);
		    break;
	    case estStop:
		    sd->image[ebtnPlay] =
			gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP,
						 GTK_ICON_SIZE_MENU);
		    break;
	    default:
		    sd->image[ebtnPlay] =
			gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR,
						 GTK_ICON_SIZE_MENU);
		    break;
	}
	gtk_widget_show(sd->image[ebtnPlay]);
	gtk_container_add(GTK_CONTAINER(sd->button[ebtnPlay]),
			  sd->image[ebtnPlay]);
	LOG("Leave squeezebox_update_playbtn");
}

#if HAVE_NOTIFY

static void
toaster_closed(NotifyNotification * notification, SqueezeBoxData * sd) {
	LOG("toaster_closed");
	sd->note = NULL;
}

static void squeezebox_update_UI_hide_toaster(gpointer thsPlayer) {
	LOG("hide_toaster");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	if (sd->note) {
		notify_notification_close(sd->note, NULL);
		sd->note = NULL;
	}
}

static gboolean on_timer(gpointer thsPlayer) {
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

static void squeezebox_update_UI_show_toaster(gpointer thsPlayer) {
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
		GString *albumArt = g_string_new(sd->player.albumArt->str);
		gint icon_size;
		GtkIconTheme *theme;

		theme = gtk_icon_theme_get_default();
		gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &icon_size, NULL);
		gchar *ntTitle = sd->player.title->str;
		//happily, we easily can escape ampersands and other usual suspects.
		gchar *ntDetails =
		    g_markup_printf_escaped("by <b>%s</b>\nfrom <i>%s</i>\n",
					    sd->player.artist->str,
					    sd->player.album->str);

		if (albumArt->len) {
			pixbuf =
			    gdk_pixbuf_new_from_file_at_size(albumArt->str,
							     icon_size,
							     icon_size, NULL);
		} else {
			pixbuf = gtk_icon_theme_load_icon(theme,
							  "media-cdrom",
							  icon_size, 0, NULL);
			if (NULL == pixbuf)
				LOG("Stock Icon mismatch!");;
		}
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
#if (LIBNOTIFY_VERSION_MAJOR == 0 && \
    LIBNOTIFY_VERSION_MINOR <=3 && \
    LIBNOTIFY_VERSION_MICRO < 2)
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
		//g_free(ntTitle);
		g_free(ntDetails);
		g_string_free(albumArt, TRUE);
	}
}
#endif

static void squeezebox_update_repeat(gpointer thsPlayer, gboolean newRepeat) {
	LOG("Enter squeezebox_update_repeat");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	sd->noUI = TRUE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuRepeat),
				       newRepeat);
	sd->noUI = FALSE;
	LOG("Leave squeezebox_update_repeat");
}

static void squeezebox_update_shuffle(gpointer thsPlayer, gboolean newShuffle) {
	LOG("Enter squeezebox_update_shuffle");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	sd->noUI = TRUE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuShuffle),
				       newShuffle);
	sd->noUI = FALSE;
	LOG("Enter squeezebox_update_shuffle");
}

static void
squeezebox_update_visibility(gpointer thsPlayer, gboolean newVisible) {
	LOG("Enter squeezebox_update_visibility");
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	sd->noUI = TRUE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuPlayer),
				       newVisible);
	sd->noUI = FALSE;
	LOG("Leave squeezebox_update_visibility");
}

static void
squeezebox_find_albumart_by_filepath(gpointer thsPlayer, const gchar * path) {
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	gchar *strNext = g_path_get_dirname(path);
	gboolean bFound = FALSE;
#if HAVE_ID3TAG	// how to query artist with id3tag
	LOG("SB Enter Check:'%s' embedded art", strNext);
	struct id3_file *fp = id3_file_open(str, ID3_FILE_MODE_READONLY);
	if (fp) {
		struct id3_tag *tag = id3_file_tag(fp);
		if (tag) {
			struct id3_frame *frm = id3_tag_findframe(tag,
								  ID3_FRAME_ARTIST,
								  0);
			if (frm) {
				id3_ucs4_t const *str =
				    id3_field_getstrings(&frm->fields[1], 0);
				if (str) {
					gchar *artist = (gchar *)
					    id3_ucs4_utf8duplicate(str);
					LOG(artist);
				}
			}
		}
		id3_file_close(fp);
	}
#endif

	LOG("SB Enter Check:'%s/[.][folder|cover|front].jpg'", strNext);

	GDir *dir = g_dir_open(strNext, 0, NULL);
	if (NULL != dir) {
		const gchar *fnam = NULL;
		while ((fnam = g_dir_read_name(dir))) {
			const gchar *fnam2 = fnam;
			if ('.' == *fnam2)
				fnam2++;
			if (!g_ascii_strcasecmp(fnam2, "folder.jpg") ||
			    !g_ascii_strcasecmp(fnam2, "front.jpg") ||
			    !g_ascii_strcasecmp(fnam2, "cover.jpg")) {
				bFound = TRUE;
				break;
			}
		}
		if (bFound) {
			gchar *fnam3 = g_build_filename(strNext, fnam, NULL);
			g_string_assign(sd->player.albumArt, fnam3);
			g_free(fnam3);
		} else {
			g_string_truncate(sd->player.albumArt, 0);
		}
		g_dir_close(dir);
	}
	g_free(strNext);
	LOG("SB: Leave Check");
}

static void
squeezebox_update_UI(gpointer thsPlayer, gboolean updateSong,
		     eSynoptics State, const gchar * playerMessage) {
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;

	if (sd->state != State) {
		sd->state = State;
		squeezebox_update_playbtn(sd);
	}

	if (updateSong) {
		if (sd->toolTipStyle == ettSimple) {
			if (sd->player.artist->len + sd->player.album->len +
			    sd->player.title->len)
				g_string_printf(sd->toolTipText, "%s: %s - %s",
						sd->player.artist->str,
						sd->player.album->str,
						sd->player.title->str);
			else {
				const Backend *ptr = squeezebox_get_backends();
				g_string_printf(sd->toolTipText,
						_("%s: No info"),
						ptr[sd->backend -
						    1].BACKEND_name());
			}
#if HAVE_GTK_2_12
			gtk_tooltip_trigger_tooltip_query
			    (gdk_display_get_default());
#else
			if (sd->show[ebtnPrev])
				gtk_tooltips_set_tip(sd->tooltips,
						     sd->button[ebtnPrev],
						     sd->toolTipText->str,
						     NULL);

			if (sd->show[ebtnPlay])
				gtk_tooltips_set_tip(sd->tooltips,
						     sd->button[ebtnPlay],
						     sd->toolTipText->str,
						     NULL);

			if (sd->show[ebtnNext])
				gtk_tooltips_set_tip(sd->tooltips,
						     sd->button[ebtnNext],
						     sd->toolTipText->str,
						     NULL);
#endif
		} else {
			g_string_assign(sd->toolTipText, "");
		}

#if HAVE_NOTIFY
		if (sd->notify) {
			squeezebox_update_UI_show_toaster(thsPlayer);
		}
#endif
	}
}

static gboolean squeezebox_set_size(XfcePanelPlugin * plugin, int size,
				    SqueezeBoxData * sd) {
	int items = 1;

	if (sd->show[ebtnPrev]) {
		gtk_widget_set_size_request(GTK_WIDGET(sd->button[ebtnPrev]),
					    size, size);
		items++;
	}

	if (sd->show[ebtnNext]) {
		gtk_widget_set_size_request(GTK_WIDGET(sd->button[ebtnNext]),
					    size, size);
		items++;
	}
	gtk_widget_set_size_request(GTK_WIDGET(sd->button[ebtnPlay]), size,
				    size);
	gtk_widget_set_size_request(GTK_WIDGET(sd->table), 3 * size, size);
	gtk_widget_set_size_request(GTK_WIDGET(plugin), items * size, size);

	return TRUE;
}

static void squeezebox_free_data(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {
	LOG("Enter squeezebox_free_data");
#ifndef HAVE_GTK_2_12
	if (sd->tooltips) {
		g_object_unref(sd->tooltips);
		sd->tooltips = NULL;
	}
#endif
#if HAVE_DBUS
	if (sd->player.dbService) {
		g_object_unref(G_OBJECT(sd->player.dbService));
		sd->player.dbService = NULL;
	}
	if (sd->player.bus) {
		//some reading shows that this should not be freed
		//but its not clear if meant server only.
		//g_object_unref(G_OBJECT(db->bus));
		sd->player.bus = NULL;
	}
#endif
	if (sd->player.Detach) {
		sd->player.Detach(sd->player.db);
		g_free(sd->player.db);
	}
#if HAVE_NOTIFY
	if (sd->timerHandle)
		g_source_remove(sd->timerHandle);
#endif
	squeezebox_update_grab(FALSE, FALSE, sd);
	g_free(sd);
	LOG("Leave squeezebox_free_data");
}

static void
squeezebox_style_set(XfcePanelPlugin * plugin, gpointer ignored,
		     SqueezeBoxData * sd) {
	squeezebox_set_size(plugin, xfce_panel_plugin_get_size(plugin), sd);
}

static void
squeezebox_persist_properties(SqueezeBoxData * sd, XfceRc *rc, gboolean isStoring) {
    // apply property map
    const Backend *ptr = squeezebox_get_backends();
    LOG("Enter persist");
    while(ptr->BACKEND_attach) {
        LOG("Properties of %s", ptr->BACKEND_name());
        PropDef *prop = ptr->BACKEND_properties();
        gchar *propValue =  NULL;
        while(*prop->Name) {
            if(isStoring) {
                if(g_hash_table_lookup_extended(sd->properties, prop->Name, NULL, (gpointer)&propValue)) {
                    xfce_rc_write_entry(rc, prop->Name, propValue);
                } else {
                    xfce_rc_write_entry(rc, prop->Name, prop->Default);
                }
            } else {
                if(rc)
                    propValue =  strdup(xfce_rc_read_entry(rc, prop->Name, prop->Default));
                else
                    propValue = strdup(prop->Default);
                g_hash_table_insert(sd->properties, strdup(prop->Name), propValue);
            }
            LOG("\t%s->'%s'", prop->Name, propValue);
            prop++;
        }
        ptr++;
    }
    LOG("Leave persist");
}

static void
squeezebox_read_rc_file(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {
	char *file;
	XfceRc *rc = NULL;
	gint nBackend = 1;
	gboolean bShowNext = TRUE;
	gboolean bShowPrev = TRUE;
	gboolean bGrabMedia = TRUE;
	gint toolTipStyle = 1;
#if HAVE_NOTIFY
	gboolean bNotify = TRUE;
	gdouble dNotifyTimeout = 5.0;
#endif
	LOG("Enter squeezebox_read_rc_file");

	if ((file = xfce_panel_plugin_lookup_rc_file(plugin)) != NULL) {
		rc = xfce_rc_simple_open(file, TRUE);
		g_free(file);

		if (rc != NULL) {
			nBackend =
			    xfce_rc_read_int_entry(rc, "squeezebox_backend", 2);

			bShowNext = xfce_rc_read_int_entry(rc, "show_next", 1);
			bShowPrev = xfce_rc_read_int_entry(rc, "show_prev", 1);
			bGrabMedia =
			    xfce_rc_read_int_entry(rc, "grab_media", 0);
#if HAVE_NOTIFY
			bNotify = xfce_rc_read_int_entry(rc, "notify", 1);
			dNotifyTimeout =
			    xfce_rc_read_int_entry(rc, "notify_timeout", 5);
#endif
			sd->player.updateRateMS =
			    xfce_rc_read_int_entry(rc, "updateRateMS", 500);
			toolTipStyle =
			    xfce_rc_read_int_entry(rc, "tooltips", 1);
			if (toolTipStyle < 0)
				toolTipStyle = 0;

		}
	}
    // backend properties
    squeezebox_persist_properties(sd, rc, FALSE);
    
	// Always init backend
	sd->player.sd = sd;
	squeezebox_init_backend(sd, nBackend);
	sd->show[ebtnNext] = bShowNext;
	sd->show[ebtnPlay] = TRUE;	// well, maybe not later
	sd->show[ebtnPrev] = bShowPrev;
	sd->grabmedia = bGrabMedia;
	squeezebox_update_grab(sd->grabmedia, FALSE, sd);
	/*
	   if(GTK_IS_WIDGET(sd->btnDet))
	   gtk_widget_set_sensitive (sd->btnDet, (NULL != sd->player.Configure));
	 */
#if HAVE_NOTIFY
	sd->notify = bNotify;
	sd->notifytimeout = dNotifyTimeout;
	if (toolTipStyle > ettFull)
		toolTipStyle = ettFull;
	sd->timerHandle = g_timeout_add(1000, on_timer, sd);
	LOG("Attach %d", sd->timerHandle);
#else
	if (toolTipStyle > ettSimple)
		toolTipStyle = ettSimple;

	if (toolTipStyle < ettNone)
		toolTipStyle = ettNone;
#endif
	sd->toolTipStyle = toolTipStyle;
#ifndef HAVE_GTK_2_12
	if (sd->toolTipStyle == ettSimple)
		gtk_tooltips_enable(sd->tooltips);
	else
		gtk_tooltips_disable(sd->tooltips);
#endif

	if (rc != NULL) {
		//if (sd->player.Persist)
			//sd->player.Persist(sd->player.db, rc, FALSE);
		xfce_rc_close(rc);

		if (bShowPrev)
			gtk_widget_show(sd->button[ebtnPrev]);
		else
			gtk_widget_hide(sd->button[ebtnPrev]);

		if (bShowNext)
			gtk_widget_show(sd->button[ebtnNext]);
		else
			gtk_widget_hide(sd->button[ebtnNext]);
	}
    if(sd->player.Assure)
        sd->player.Assure(sd->player.db, TRUE);
	LOG("Leave squeezebox_read_rc_file");
}

static void
squeezebox_write_rc_file(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {

	char *file;
	XfceRc *rc;

	LOG("Enter squeezebox_write_rc_file");

	if ((file = xfce_panel_plugin_save_location(plugin, TRUE))) {

		rc = xfce_rc_simple_open(file, FALSE);
		g_free(file);

		if (rc != NULL) {
			LOG("Writing to file '%s'", file);
			xfce_rc_write_int_entry(rc, "squeezebox_backend",
						sd->backend);
			xfce_rc_write_int_entry(rc, "show_next",
						(sd->show[ebtnNext]) ? 1 : 0);
			xfce_rc_write_int_entry(rc, "show_prev",
						(sd->show[ebtnPrev]) ? 1 : 0);
			xfce_rc_write_int_entry(rc, "grab_media",
						(sd->grabmedia) ? 1 : 0);
			xfce_rc_write_int_entry(rc, "tooltips",
						sd->toolTipStyle);
#if HAVE_NOTIFY
			xfce_rc_write_int_entry(rc, "notify",
						(sd->notify) ? 1 : 0);
			xfce_rc_write_int_entry(rc, "notify_timeout",
						sd->notifytimeout);
#endif

			squeezebox_persist_properties(sd, rc, TRUE);
            
            //if (sd->player.Persist)
				//sd->player.Persist(sd->player.db, rc, TRUE);

			xfce_rc_close(rc);
			LOG("OK");
		}
	}
	LOG("Leave squeezebox_write_rc_file");
}

static void
squeezebox_dialog_response(GtkWidget * dlg, int reponse, SqueezeBoxData * sd) {
	g_object_set_data(G_OBJECT(sd->plugin), "dialog", NULL);
	gtk_widget_destroy(dlg);
    LOG("Enter DialogResponse");
	xfce_panel_plugin_unblock_menu(sd->plugin);
    // apply new properties, if applicable
	const Backend *ptr = squeezebox_get_backends();
    PropDef *prop = ptr[sd->backend - 1].BACKEND_properties();  
    while(*prop->Name) {
        gchar *propValue = NULL;
        switch(prop->Type) {
            case G_TYPE_BOOLEAN: {
                gboolean *boolVal = (gboolean*)g_hash_table_lookup(sd->propertyAddresses, prop->Name);
                LOG("Apply property %s, %p", prop->Name, boolVal);
                if(boolVal)
                    propValue = g_strdup_printf("%d", *boolVal);
            }break;
            case G_TYPE_INT: {
                gint *gintVal = g_hash_table_lookup(sd->propertyAddresses, prop->Name);
                if(gintVal)
                    propValue = g_strdup_printf("%d", *gintVal);
            }break;
            case G_TYPE_STRING: {
                GString *strVal = g_hash_table_lookup(sd->propertyAddresses, prop->Name);
                if(strVal)
                    propValue = g_strdup(strVal->str);
            }break;
        }
        if(propValue) {
            g_hash_table_insert(sd->properties, g_strdup(prop->Name), propValue);
            //g_free(propValue);
        }
        prop++;
    }
	squeezebox_write_rc_file(sd->plugin, sd);
    LOG("Leave DialogResponse");
}

static void config_show_backend_properties(GtkButton * btn, SqueezeBoxData * sd) {
	LOG("Enter config_show_backend_properties");
	if (sd->player.Configure)
		sd->player.Configure(sd->player.db, GTK_WIDGET(sd->plugin));
	else {
		GtkWidget *dlg =
		    gtk_message_dialog_new(GTK_WINDOW
					   (g_object_get_data
					    (G_OBJECT(sd->plugin), "dialog")),
					   GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
					   GTK_BUTTONS_OK,
					   _
					   ("This backend has no configurable properties"));

		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
	}
	LOG("Leave config_show_backend_properties");
}

enum {
	PIXBUF_COLUMN,
	TEXT_COLUMN,
    INDEX_COLUMN,
	N_COLUMNS,
};

static void config_show_grab_properties(GtkButton * btn, SqueezeBoxData * sd) {
	GtkWidget *dlg = xfce_titled_dialog_new_with_buttons(_("Media buttons"),
							     GTK_WINDOW
							     (g_object_get_data
							      (G_OBJECT
							       (sd->plugin),
							       "dialog")),
							     GTK_DIALOG_MODAL,
							     GTK_STOCK_CANCEL,
							     0, GTK_STOCK_OK, 1,
							     NULL);

	xfce_titled_dialog_set_subtitle(XFCE_TITLED_DIALOG(dlg),
					_("Grab state of multimedia keys"));
	GtkWidget *vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox), vbox);
	GtkWidget *label =
	    gtk_label_new
	    ("Currently not implemented\nBut may as well show things to come.");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 16);

	gtk_widget_show_all(dlg);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

static void config_change_backend(GtkComboBox * cb, SqueezeBoxData * sd) {
	GtkTreeIter iter = {0};
    LOG("Backend change...");
    if( gtk_combo_box_get_active_iter(cb, &iter)) {
        gint nBackend = -1;
        LOG("Have iter %d", iter.stamp);
        GtkTreeModel *model = gtk_combo_box_get_model(cb);
        if(model) {
            gchar *backend = NULL;
            gtk_tree_model_get(model, &iter, TEXT_COLUMN, &backend, INDEX_COLUMN, &nBackend, -1);
            LOG("Have model %s %d", backend, nBackend);
	        squeezebox_init_backend(sd, nBackend + 1);
	        gtk_widget_set_sensitive(sd->btnDet, (NULL != sd->player.Configure));
        }
    }
}

#if HAVE_NOTIFY
static void
config_change_notify_timeout(GtkSpinButton * sb, SqueezeBoxData * sd) {
	sd->notifytimeout = gtk_spin_button_get_value(sb);
}

static void config_toggle_notify(GtkToggleButton * tb, SqueezeBoxData * sd) {
	sd->notify = gtk_toggle_button_get_active(tb);
#if HAVE_NOTIFY
	if (sd->toolTipStyle == ettFull) {
		sd->timerHandle = g_timeout_add(1000, on_timer, sd);
	} else if (sd->timerHandle != 0) {
		g_source_remove(sd->timerHandle);
        sd->timerHandle = 0;
    }
#endif

}
#endif

static void
config_toggle_tooltips_none(GtkToggleButton * opt, SqueezeBoxData * sd) {
	if (gtk_toggle_button_get_active(opt)) {
		sd->toolTipStyle = ettNone;
#ifndef HAVE_GTK_2_12
		gtk_tooltips_disable(sd->tooltips);
#endif
	}
}

static void
config_toggle_tooltips_simple(GtkToggleButton * opt, SqueezeBoxData * sd) {
	if (gtk_toggle_button_get_active(opt)) {
		sd->toolTipStyle = ettSimple;
#ifndef HAVE_GTK_2_12
		gtk_tooltips_enable(sd->tooltips);
#endif
	}
}

#if HAVE_NOTIFY
static void
config_toggle_tooltips_full(GtkToggleButton * opt, SqueezeBoxData * sd) {
	if (gtk_toggle_button_get_active(opt)) {
		sd->toolTipStyle = ettFull;
#ifndef HAVE_GTK_2_12
		gtk_tooltips_disable(sd->tooltips);
#endif
	}
}
#endif

static void
squeezebox_update_grab(gboolean bGrab, gboolean bShowErr, SqueezeBoxData * sd) {
	if (bGrab) {
		if (sd->mmkeys) {
			// ungrab before grab
			squeezebox_update_grab(FALSE, FALSE, sd);
		}
		// grab
		LOG("grab");
		sd->mmkeys = mmkeys_new();
		//g_object_new(TYPE_MMKEYS, NULL);

		// connections are go
		sd->mmhandlers[0] = g_signal_connect(sd->mmkeys, "mm_prev",
						     G_CALLBACK
						     (on_keyPrev_clicked), sd);
		sd->mmhandlers[2] =
		    g_signal_connect(sd->mmkeys, "mm_playpause",
				     G_CALLBACK(on_keyPlay_clicked), sd);
		sd->mmhandlers[3] =
		    g_signal_connect(sd->mmkeys, "mm_next",
				     G_CALLBACK(on_keyNext_clicked), sd);

		if (bShowErr) {
			// tell user what we found, hint at xmodmap if necessary.
		}
	} else {
		// ungrab
		LOG("ungrab");
		if (sd->mmkeys) {
			int i;
			for (i = 0; i < 4; i++) {
				if (sd->mmhandlers[i]) {
					g_signal_handler_disconnect(sd->mmkeys,
								    sd->
								    mmhandlers
								    [i]);
					sd->mmhandlers[i] = 0;
				}
			}
			LOG("unref-");
			g_object_unref(sd->mmkeys);
			sd->mmkeys = NULL;
			LOG("-");
		}
	}
}

static void config_toggle_grabmedia(GtkToggleButton * tb, SqueezeBoxData * sd) {
	gboolean *pBtn = &sd->grabmedia;

	*pBtn = gtk_toggle_button_get_active(tb);

	squeezebox_update_grab(*pBtn, TRUE, sd);
}

static void config_toggle_next(GtkToggleButton * tb, SqueezeBoxData * sd) {
	gboolean *pBtn = &sd->show[ebtnNext];
	gint size = xfce_panel_plugin_get_size(sd->plugin);

	*pBtn = gtk_toggle_button_get_active(tb);

	if (*pBtn)
		gtk_widget_show(sd->button[ebtnNext]);
	else
		gtk_widget_hide(sd->button[ebtnNext]);

	squeezebox_set_size(sd->plugin, size, sd);
}

static void config_toggle_prev(GtkToggleButton * tb, SqueezeBoxData * sd) {
	gboolean *pBtn = &sd->show[ebtnPrev];
	gint size = xfce_panel_plugin_get_size(sd->plugin);

	*pBtn = gtk_toggle_button_get_active(tb);

	if (*pBtn)
		gtk_widget_show(sd->button[ebtnPrev]);
	else
		gtk_widget_hide(sd->button[ebtnPrev]);

	squeezebox_set_size(sd->plugin, size, sd);
}

static void
squeezebox_properties_dialog(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {
	GtkWidget *dlg, *header, *vbox, *hbox1, *label0, *label1,
	    *cb1, *cb2, *cb3, *btnView;
	GtkWidget *cbBackend;
	GtkWidget *opt[3];
#if HAVE_NOTIFY
	GtkWidget *squeezebox_delay_spinner;
	GtkWidget *cb4, *label2, *hbox2, *cbNotLoc;
#endif

	xfce_panel_plugin_block_menu(plugin);

	dlg = gtk_dialog_new_with_buttons(_("Properties"),
					  GTK_WINDOW(gtk_widget_get_toplevel
						     (GTK_WIDGET(plugin))),
					  GTK_DIALOG_DESTROY_WITH_PARENT |
					  GTK_DIALOG_NO_SEPARATOR,
					  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
					  NULL);

	g_object_set_data(G_OBJECT(plugin), "dialog", dlg);

	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(GTK_WINDOW(dlg), "xfce-sound");

	g_signal_connect(dlg, "response",
			 G_CALLBACK(squeezebox_dialog_response), sd);

	gtk_container_set_border_width(GTK_CONTAINER(dlg), 2);

	header = xfce_heading_new();
	xfce_heading_set_title(XFCE_HEADING(header), _("Squeezebox"));
	xfce_heading_set_icon_name(XFCE_HEADING(header), "xfce-sound");
	xfce_heading_set_subtitle(XFCE_HEADING(header),
				  _("media player remote"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), header, FALSE, TRUE,
			   0);

	vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), vbox, TRUE, TRUE, 0);

	label0 = gtk_label_new(NULL);
	GValue val = { 0 };
	g_value_init(&val, G_TYPE_DOUBLE);
	g_object_set_property(G_OBJECT(label0), "xalign", &val);
	gchar *markup0 = g_markup_printf_escaped("<b>%s</b>", _("Appearance"));
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label0), markup0);
	g_free(markup0);
	gtk_box_pack_start(GTK_BOX(vbox), label0, FALSE, FALSE, 0);

	//check1+2
	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);

	//check1
	cb1 = gtk_check_button_new_with_mnemonic(_("Show p_revious button"));
	gtk_box_pack_start(GTK_BOX(hbox1), cb1, FALSE, FALSE, 16);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb1),
				     sd->show[ebtnPrev]);
	g_signal_connect(cb1, "toggled", G_CALLBACK(config_toggle_prev), sd);
	//check2
	cb2 = gtk_check_button_new_with_mnemonic(_("Show _next button"));
	gtk_box_pack_start(GTK_BOX(hbox1), cb2, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb2),
				     sd->show[ebtnNext]);
	g_signal_connect(cb2, "toggled", G_CALLBACK(config_toggle_next), sd);
	//check3+btnView
	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
	//check3
	cb3 =
	    gtk_check_button_new_with_mnemonic(_
					       ("Grab _media buttons, if available"));
	gtk_box_pack_start(GTK_BOX(hbox1), cb3, TRUE, TRUE, 16);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb3), sd->grabmedia);
	g_signal_connect(cb3, "toggled", G_CALLBACK(config_toggle_grabmedia),
			 sd);
	//btnView
	btnView = gtk_button_new_with_mnemonic(_("_Details..."));
	gtk_box_pack_start(GTK_BOX(hbox1), btnView, FALSE, FALSE, 0);
	g_signal_connect(btnView, "clicked",
			 G_CALLBACK(config_show_grab_properties), sd);

	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
	opt[0] = gtk_radio_button_new_with_mnemonic(NULL, _("No _tooltips"));
	gtk_box_pack_start(GTK_BOX(hbox1), opt[0], FALSE, FALSE, 16);
	g_signal_connect(opt[0], "toggled",
			 G_CALLBACK(config_toggle_tooltips_none), sd);

	opt[1] =
	    gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON
							   (opt[0]),
							   _
							   ("_Simple tooltips"));
	gtk_box_pack_start(GTK_BOX(hbox1), opt[1], FALSE, FALSE, 0);
	g_signal_connect(opt[1], "toggled",
			 G_CALLBACK(config_toggle_tooltips_simple), sd);

#if HAVE_NOTIFY
	opt[2] =
	    gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON
							   (opt[0]),
							   _
							   ("N_otification tooltips"));
	g_signal_connect(opt[2], "toggled",
			 G_CALLBACK(config_toggle_tooltips_full), sd);

	// 0, 1 or 2
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(opt[sd->toolTipStyle]),
				     TRUE);

	gtk_box_pack_start(GTK_BOX(hbox1), opt[2], FALSE, FALSE, 0);

	//check4+cbNotLoc
	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
	//cb4
	cb4 =
	    gtk_check_button_new_with_mnemonic(_
					       ("Show noti_fications (notify daemon)"));
	gtk_box_pack_start(GTK_BOX(hbox1), cb4, FALSE, FALSE, 16);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb4), sd->notify);
	g_signal_connect(cb4, "toggled", G_CALLBACK(config_toggle_notify), sd);
	//cbNotLoc
	cbNotLoc = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbNotLoc), "Located");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbNotLoc), "System default");
	gtk_combo_box_set_active(GTK_COMBO_BOX(cbNotLoc), 0);
	gtk_widget_set_sensitive(cbNotLoc, FALSE);	//tbd: implement location supp.
	gtk_box_pack_start(GTK_BOX(hbox1), cbNotLoc, TRUE, TRUE, 0);

	hbox2 = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

	label2 = gtk_label_new_with_mnemonic(_("Notification _timeout (sec)"));
	gtk_box_pack_start(GTK_BOX(hbox2), label2, FALSE, FALSE, 16);

	squeezebox_delay_spinner =
	    gtk_spin_button_new_with_range(0.0, 60.0, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(squeezebox_delay_spinner),
				  sd->notifytimeout);
	gtk_box_pack_start(GTK_BOX(hbox2), squeezebox_delay_spinner, FALSE,
			   FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label2),
				      squeezebox_delay_spinner);

	g_signal_connect(squeezebox_delay_spinner, "value-changed",
			 G_CALLBACK(config_change_notify_timeout), sd);
#else
	// force 0 or 1
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				     (opt[(sd->toolTipStyle) ? 1 : 0]), TRUE);
#endif
	label1 = gtk_label_new_with_mnemonic(NULL);
	gchar *markup1 = g_markup_printf_escaped("<b>%s</b>", _("_Backend"));
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label1), markup1);
	g_free(markup1);
	g_object_set_property(G_OBJECT(label1), "xalign", &val);
	gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);

	GtkListStore *store;

	/* make a new list store */
	store = gtk_list_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), TEXT_COLUMN, GTK_SORT_ASCENDING);

	/* fill the store with data */
	GtkTreeIter iter = { 0 };
	const Backend *ptr = squeezebox_get_backends();
    gint idx = 0;
	for (;;) {
		LOG("Have %s", ptr->BACKEND_name());
		gtk_list_store_append(store, &iter);
		GdkPixbuf *pix =
		    exo_gdk_pixbuf_scale_down(ptr->BACKEND_icon(), TRUE, 24,
					      32);
		gtk_list_store_set(store, &iter, 
                   PIXBUF_COLUMN, pix,
				   TEXT_COLUMN, ptr->BACKEND_name(), 
                   INDEX_COLUMN, idx++, -1);
		ptr++;
		if (!ptr->BACKEND_name)
			break;
	}

	cbBackend =
	    GTK_WIDGET(gtk_combo_box_new_with_model(GTK_TREE_MODEL(store)));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), cbBackend);

	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_fixed_size(renderer, 36, 24);

	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbBackend), renderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbBackend), renderer,
				      "pixbuf", PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(cbBackend), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbBackend), renderer,
				      "text", TEXT_COLUMN);

	g_object_unref(G_OBJECT(store));
    
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    do {
        gint nBackend = -1;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, INDEX_COLUMN, &nBackend, -1);
        if(nBackend == (sd->backend -1)) {
	        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(cbBackend), &iter);
            break;
        }
    }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));

	g_signal_connect(cbBackend, "changed",
			 G_CALLBACK(config_change_backend), sd);

	gtk_box_pack_start(GTK_BOX(hbox1), cbBackend, TRUE, TRUE, 16);

	sd->btnDet = gtk_button_new_with_mnemonic(_("_Settings..."));
	gtk_box_pack_start(GTK_BOX(hbox1), sd->btnDet, FALSE, FALSE, 8);
	g_signal_connect(sd->btnDet, "clicked",
			 G_CALLBACK(config_show_backend_properties), sd);
	gtk_widget_set_sensitive(sd->btnDet, (NULL != sd->player.Configure));

	gtk_widget_show_all(dlg);
}

void squeezebox_prev(SqueezeBoxData * sd) {
	if (sd->player.Previous)
		sd->player.Previous(sd->player.db);
}
void on_btnPrev_clicked(GtkButton * button, SqueezeBoxData * sd) {
	squeezebox_prev(sd);
}
void on_keyPrev_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd) {
	squeezebox_prev(sd);
}

void on_keyStop_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd) {
	gboolean bRet = FALSE;
	if (sd->player.IsPlaying && sd->player.IsPlaying(sd->player.db))
		if (sd->player.Toggle)
			sd->player.Toggle(sd->player.db, &bRet);
}

gboolean squeezebox_play(SqueezeBoxData * sd) {
	LOG("Enter squeezebox_play");
	gboolean bRet = FALSE;
	if (sd->player.Toggle && sd->player.Toggle(sd->player.db, &bRet)) {
		squeezebox_update_playbtn(sd);
	}
	LOG("Leave squeezebox_play");
	return bRet;
}
gboolean on_btn_clicked(GtkWidget * button, GdkEventButton * event,
			SqueezeBoxData * sd) {
	if (3 == event->button && sd->state != estStop) {
        LOG("RightClick");
	    gtk_widget_set_sensitive(sd->mnuPlayer, (NULL != sd->player.Show));
	    gtk_check_menu_item_set_inconsistent(GTK_CHECK_MENU_ITEM(sd->mnuPlayer),
					         (NULL == sd->player.IsVisible));
		sd->noUI = TRUE;
		if (NULL != sd->player.GetRepeat)
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
						       (sd->mnuRepeat),
						       sd->player.GetRepeat
						       (sd->player.db));
		if (NULL != sd->player.GetShuffle)
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
						       (sd->mnuShuffle),
						       sd->player.
						       GetShuffle(sd->player.
								  db));
		sd->noUI = FALSE;
	} else if (2 == event->button) {
        LOG("MiddleClick");
		if (NULL != sd->player.IsVisible && NULL != sd->player.Show) {
			sd->player.Show(sd->player.db,
					!sd->player.IsVisible(sd->player.db));
			squeezebox_update_visibility(sd,
						     sd->player.IsVisible(sd->
									  player.
									  db));
		}
	}
	return FALSE;
}

void on_btnPlay_clicked(GtkButton * button, SqueezeBoxData * sd) {
	squeezebox_play(sd);
}
void on_keyPlay_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd) {
	squeezebox_play(sd);
}

void squeezebox_next(SqueezeBoxData * sd) {
	LOG("Enter squeezebox_next");
	if (sd->player.Next)
		sd->player.Next(sd->player.db);
	LOG("Leave squeezebox_next");
}

void squeezebox_map_property(gpointer thsPlayer, const gchar *propName, gpointer address) {
    LOG("Mapping property %s to %p", propName, address);
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
    g_hash_table_insert(sd->propertyAddresses, g_strdup(propName), address);
}

void on_btnNext_clicked(GtkButton * button, SqueezeBoxData * sd) {
	squeezebox_next(sd);
}
void on_keyNext_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd) {
	squeezebox_next(sd);
}

#if HAVE_NOTIFY
static gboolean
on_btn_any_enter(GtkWidget * widget, GdkEventCrossing * event,
		 gpointer thsPlayer) {
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	if (sd->toolTipStyle == ettFull) {
		sd->inEnter = TRUE;
		if (NULL == sd->note)
			squeezebox_update_UI_show_toaster(thsPlayer);
	}
	return FALSE;
}

static gboolean
on_btn_any_leave(GtkWidget * widget, GdkEventCrossing * event,
		 gpointer thsPlayer) {
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	if (sd->toolTipStyle == ettFull) {
		sd->inEnter = FALSE;

		if (sd->timerCount > 0)
			sd->timerCount = 1;
	}
	return FALSE;
}
#endif

void on_mnuPlayerToggled(GtkCheckMenuItem * checkmenuitem, SqueezeBoxData * sd) {
	if (sd->noUI == FALSE && sd->player.Show) {
		sd->player.Show(sd->player.db, checkmenuitem->active);
		if (sd->player.IsVisible)
			squeezebox_update_visibility(sd,
						     sd->player.IsVisible(sd->
									  player.
									  db));
	}
}

void on_mnuShuffleToggled(GtkCheckMenuItem * checkmenuitem, SqueezeBoxData * sd) {
	if (sd->noUI == FALSE && sd->player.SetShuffle) {
		sd->player.SetShuffle(sd->player.db, checkmenuitem->active);
		if (sd->player.GetShuffle)
			squeezebox_update_shuffle(sd,
						  sd->player.GetShuffle(sd->
									player.
									db));
	}
}

void on_mnuRepeatToggled(GtkCheckMenuItem * checkmenuitem, SqueezeBoxData * sd) {
	if (sd->noUI == FALSE && sd->player.SetRepeat)
		sd->player.SetRepeat(sd->player.db, checkmenuitem->active);
}

#if HAVE_GTK_2_12
gboolean on_query_tooltip(GtkWidget * widget, gint x, gint y,
			  gboolean keyboard_mode, GtkTooltip * tooltip,
			  SqueezeBoxData * sd) {
	if (sd->toolTipStyle == ettSimple && sd->toolTipText->str != NULL
	    && sd->toolTipText->str[0] != 0) {
		gtk_tooltip_set_text(tooltip, sd->toolTipText->str);
		gtk_tooltip_set_icon(tooltip,
				     squeezebox_get_backends(sd)[sd->backend -
								 1].BACKEND_icon());
		return TRUE;
	}
	return FALSE;
}
#endif

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

static GtkContainer *squeezebox_create(SqueezeBoxData * sd) {
	LOG("Enter squeezebox_create");

	GtkContainer *window1 = GTK_CONTAINER(sd->plugin);
	sd->table = gtk_table_new(1, 3, FALSE);
	gtk_widget_show(sd->table);
	gtk_container_add(GTK_CONTAINER(window1), sd->table);

#ifndef HAVE_GTK_2_12
	sd->tooltips = gtk_tooltips_new();
	g_object_ref(sd->tooltips);
	gtk_object_sink(GTK_OBJECT(sd->tooltips));
#endif

	sd->button[ebtnPrev] = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(sd->button[ebtnPrev]),
			      GTK_RELIEF_NONE);
	gtk_widget_show(sd->button[ebtnPrev]);
	gtk_table_attach(GTK_TABLE(sd->table), sd->button[ebtnPrev], 0, 1, 0, 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(sd->button[ebtnPrev]), FALSE);

	sd->image[ebtnPrev] =
	    gtk_image_new_from_stock("gtk-media-rewind", GTK_ICON_SIZE_MENU);
	gtk_widget_show(sd->image[ebtnPrev]);
	gtk_container_add(GTK_CONTAINER(sd->button[ebtnPrev]),
			  sd->image[ebtnPrev]);

	sd->button[ebtnPlay] = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(sd->button[ebtnPlay]),
			      GTK_RELIEF_NONE);
	gtk_widget_show(sd->button[ebtnPlay]);
	gtk_table_attach(GTK_TABLE(sd->table), sd->button[ebtnPlay], 1, 2, 0, 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(sd->button[ebtnPlay]), FALSE);

	sd->image[ebtnPlay] =
	    gtk_image_new_from_stock("gtk-media-play", GTK_ICON_SIZE_MENU);
	gtk_widget_show(sd->image[ebtnPlay]);
	gtk_container_add(GTK_CONTAINER(sd->button[ebtnPlay]),
			  sd->image[ebtnPlay]);

	sd->button[ebtnNext] = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(sd->button[ebtnNext]),
			      GTK_RELIEF_NONE);
	gtk_widget_show(sd->button[ebtnNext]);
	gtk_table_attach(GTK_TABLE(sd->table), sd->button[ebtnNext], 2, 3, 0, 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(sd->button[ebtnNext]), FALSE);

	sd->image[ebtnNext] =
	    gtk_image_new_from_stock("gtk-media-forward", GTK_ICON_SIZE_MENU);
	gtk_widget_show(sd->image[ebtnNext]);
	gtk_container_add(GTK_CONTAINER(sd->button[ebtnNext]),
			  sd->image[ebtnNext]);

	// connect signals of buttons ...
	g_signal_connect((gpointer) sd->button[ebtnPrev], "clicked",
			 G_CALLBACK(on_btnPrev_clicked), sd);
	g_signal_connect((gpointer) sd->button[ebtnPlay], "clicked",
			 G_CALLBACK(on_btnPlay_clicked), sd);
	g_signal_connect((gpointer) sd->button[ebtnNext], "clicked",
			 G_CALLBACK(on_btnNext_clicked), sd);

	g_signal_connect((gpointer) sd->button[ebtnPrev], "button-press-event",
			 G_CALLBACK(on_btn_clicked), sd);
	g_signal_connect((gpointer) sd->button[ebtnPlay], "button-press-event",
			 G_CALLBACK(on_btn_clicked), sd);
	g_signal_connect((gpointer) sd->button[ebtnNext], "button-press-event",
			 G_CALLBACK(on_btn_clicked), sd);

	// toaster handling
#if HAVE_NOTIFY
	g_signal_connect(sd->button[ebtnPrev], "enter-notify-event",
			 G_CALLBACK(on_btn_any_enter), sd);
	g_signal_connect(sd->button[ebtnPrev], "leave-notify-event",
			 G_CALLBACK(on_btn_any_leave), sd);

	g_signal_connect(sd->button[ebtnPlay], "enter-notify-event",
			 G_CALLBACK(on_btn_any_enter), sd);
	g_signal_connect(sd->button[ebtnPlay], "leave-notify-event",
			 G_CALLBACK(on_btn_any_leave), sd);

	g_signal_connect(sd->button[ebtnNext], "enter-notify-event",
			 G_CALLBACK(on_btn_any_enter), sd);
	g_signal_connect(sd->button[ebtnNext], "leave-notify-event",
			 G_CALLBACK(on_btn_any_leave), sd);

#endif

	/* Store pointers to all widgets, for use by lookup_widget().
	   GLADE_HOOKUP_OBJECT_NO_REF (window1, window1, "window1");
	   GLADE_HOOKUP_OBJECT (window1, table1, "table1");
	   GLADE_HOOKUP_OBJECT (window1, btnPrev, "btnPrev");
	   GLADE_HOOKUP_OBJECT (window1, image1, "image1");
	   GLADE_HOOKUP_OBJECT (window1, btnPlay, "btnPlay");
	   GLADE_HOOKUP_OBJECT (window1, image4, "image4");
	   GLADE_HOOKUP_OBJECT (window1, btnNext, "btnNext");
	   GLADE_HOOKUP_OBJECT (window1, alignment2, "alignment2");
	   GLADE_HOOKUP_OBJECT (window1, hbox2, "hbox2");
	   GLADE_HOOKUP_OBJECT (window1, image3, "image3");
	   GLADE_HOOKUP_OBJECT (window1, label2, "label2");
	 */

	if (sd->state != ebtnPlay)
		squeezebox_update_playbtn(sd);

	LOG("Leave squeezebox_create");
	return window1;
}

#if HAVE_DBUS
static void squeezebox_update_dbus(DBusGProxy * proxy, const gchar * Name,
				   const gchar * OldOwner,
				   const gchar * NewOwner,
				   SqueezeBoxData * sd) {
	if (sd->backend) {
		const Backend *ptr = squeezebox_get_backends();
		ptr += (sd->backend - 1);
		if (ptr->BACKEND_dbusName) {
			gboolean appeared = (NULL != NewOwner
					     && 0 != NewOwner[0]);
			const gchar *dbusName = ptr->BACKEND_dbusName();
			if (sd->player.UpdateDBUS
			    && !g_ascii_strcasecmp(Name, dbusName)) {
				LOG("DBUS name change %s: '%s'->'%s'", Name,
				    OldOwner, NewOwner);
				sd->player.UpdateDBUS(sd->player.db, appeared);
			}
		}
	}
}
#endif
static void squeezebox_construct(XfcePanelPlugin * plugin) {
	int i = 0;

	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	LOG("Enter squeezebox_construct");

	SqueezeBoxData *sd = g_new0(SqueezeBoxData, 1);

	sd->plugin = plugin;
    sd->properties = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    sd->propertyAddresses = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	sd->player.artist = g_string_new(_("(unknown)"));
	sd->player.album = g_string_new(_("(unknown)"));
	sd->player.title = g_string_new(_("(unknown)"));
	sd->player.albumArt = g_string_new("");
	sd->toolTipText = g_string_new("");
	sd->player.Update = squeezebox_update_UI;
	sd->player.UpdateShuffle = squeezebox_update_shuffle;
	sd->player.UpdateRepeat = squeezebox_update_repeat;
	sd->player.UpdateVisibility = squeezebox_update_visibility;
#if HAVE_DBUS
	sd->player.bus = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	if (!sd->player.bus) {
		LOGERR("\tCouldn't connect to dbus");
	} else {
		// user close and appear notification
		sd->player.dbService = dbus_g_proxy_new_for_name(sd->player.bus,
								 DBUS_SERVICE_DBUS,
								 DBUS_PATH_DBUS,
								 DBUS_INTERFACE_DBUS);
		dbus_g_proxy_add_signal(sd->player.dbService,
					"NameOwnerChanged", G_TYPE_STRING,
					G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(sd->player.dbService,
					    "NameOwnerChanged",
					    G_CALLBACK(squeezebox_update_dbus),
					    sd, NULL);
	}
#endif
	sd->player.MonitorFile = NULL;	//tbd
	sd->player.FindAlbumArtByFilePath =
	    squeezebox_find_albumart_by_filepath;
    sd->player.MapProperty = squeezebox_map_property;
#if HAVE_NOTIFY
	sd->inEnter = FALSE;
	sd->notifytimeout = 5;
	sd->timerHandle = 0;
	sd->inCreate = TRUE;
#endif

	squeezebox_create(sd);

	xfce_panel_plugin_add_action_widget(plugin, sd->button[ebtnPrev]);
	xfce_panel_plugin_add_action_widget(plugin, sd->button[ebtnPlay]);
	xfce_panel_plugin_add_action_widget(plugin, sd->button[ebtnNext]);
	xfce_panel_plugin_add_action_widget(plugin, sd->table);

	g_signal_connect(plugin, "free-data",
			 G_CALLBACK(squeezebox_free_data), sd);

	g_signal_connect(plugin, "size-changed",
			 G_CALLBACK(squeezebox_set_size), sd);

	sd->style_id =
	    g_signal_connect(plugin, "style-set",
			     G_CALLBACK(squeezebox_style_set), sd);

	xfce_panel_plugin_menu_show_configure(plugin);
	g_signal_connect(plugin, "configure-plugin",
			 G_CALLBACK(squeezebox_properties_dialog), sd);

	// add menu items
	sd->mnuPlayer = gtk_check_menu_item_new_with_label(_("Show player"));
	gtk_widget_show(sd->mnuPlayer);
	xfce_panel_plugin_menu_insert_item(sd->plugin,
					   GTK_MENU_ITEM(sd->mnuPlayer));
	g_signal_connect(G_OBJECT(sd->mnuPlayer), "toggled",
			 G_CALLBACK(on_mnuPlayerToggled), sd);
	g_object_ref(sd->mnuPlayer);

	sd->mnuShuffle = gtk_check_menu_item_new_with_label(_("Shuffle"));
	gtk_widget_show(sd->mnuShuffle);
	xfce_panel_plugin_menu_insert_item(sd->plugin,
					   GTK_MENU_ITEM(sd->mnuShuffle));
	g_signal_connect(G_OBJECT(sd->mnuShuffle), "toggled",
			 G_CALLBACK(on_mnuShuffleToggled), sd);
	g_object_ref(sd->mnuShuffle);

	sd->mnuRepeat = gtk_check_menu_item_new_with_label(_("Repeat"));
	gtk_widget_show(sd->mnuRepeat);
	xfce_panel_plugin_menu_insert_item(sd->plugin,
					   GTK_MENU_ITEM(sd->mnuRepeat));
	g_signal_connect(G_OBJECT(sd->mnuRepeat), "toggled",
			 G_CALLBACK(on_mnuRepeatToggled), sd);
	g_object_ref(sd->mnuRepeat);

#if HAVE_GTK_2_12
// newish tooltips
	for (i = 0; i < 3; i++) {
		g_object_set(sd->button[i], "has-tooltip", TRUE, NULL);
		g_signal_connect(G_OBJECT(sd->button[i]), "query-tooltip",
				 G_CALLBACK(on_query_tooltip), sd);
	}
#endif
	squeezebox_read_rc_file(plugin, sd);

	// the above will init & create the actual player backend
	// and also init menu states
#if HAVE_NOTIFY
	sd->inCreate = FALSE;
#endif
	LOG("Leave squeezebox_construct");

}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(squeezebox_construct);
