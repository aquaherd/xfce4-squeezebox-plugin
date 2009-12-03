/***************************************************************************
 *            squeezebox.c - frontend of xfce4-squeezebox-plugin
 *
 *  Fri Aug 25 17:20:09 2006
 *  Copyright  2006-2009  Hakan Erduman
 *  Email hakan@erduman.de
 ****************************************************************************
 *  $Rev::             $: Revision of last commit
 *	$Author::          $: Author of last commit
 *	$Date::            $: Date of last commit
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

#include "notifications.h"
#include "squeezebox.h"
#include "squeezebox-private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if HAVE_ID3TAG
#include <id3tag.h>
#endif
#include <libintl.h>

typedef enum eButtons {
	ebtnPrev = 0,
	ebtnPlay = 1,
	ebtnNext = 2
} eButtons;

typedef enum eTTStyle {
	ettNone = 0,
	ettSimple = 1,
} eTTStyle;


/* some small helpers - unused for now
static void lose (const char *fmt, ...) G_GNUC_NORETURN G_GNUC_PRINTF (1, 2);
static void lose_gerror (const char *prefix, GError *error) G_GNUC_NORETURN;
*/
#define MKTHIS SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer

/* Backend import */
//~ #if HAVE_BACKEND_RHYTHMBOX
	//~ IMPORT_DBUS_BACKEND(RB)
//~ #endif
//~ #if HAVE_BACKEND_MPD
    //~ IMPORT_BACKEND(MPD)
//~ #endif
//~ #if HAVE_BACKEND_QUODLIBET
    //~ IMPORT_DBUS_BACKEND(QL)
//~ #endif
//~ #if HAVE_BACKEND_AUDACIOUS
    //~ IMPORT_DBUS_BACKEND(AU)
//~ #endif
//~ #if HAVE_BACKEND_EXAILE
    //~ IMPORT_DBUS_BACKEND(EX)
//~ #endif
//~ #if HAVE_BACKEND_CONSONANCE
    //~ IMPORT_DBUS_BACKEND(CS)
//~ #endif
//~ #if HAVE_BACKEND_MUINE
    //~ IMPORT_DBUS_BACKEND(MU)
//~ #endif
//~ #if HAVE_BACKEND_BANSHEE
    //~ IMPORT_DBUS_BACKEND(BA)
//~ #endif
/* Backend mapping */
BEGIN_BACKEND_MAP()
//~ #if HAVE_BACKEND_RHYTHMBOX
    //~ DBUS_BACKEND(RB)
//~ #endif
//~ #if HAVE_BACKEND_MPD
    //~ BACKEND(MPD)
//~ #endif
//~ #if HAVE_BACKEND_QUODLIBET
    //~ DBUS_BACKEND(QL)
//~ #endif
//~ #if HAVE_BACKEND_AUDACIOUS
    //~ DBUS_BACKEND(AU)
//~ #endif
//~ #if HAVE_BACKEND_EXAILE
    //~ DBUS_BACKEND(EX)
//~ #endif
//~ #if HAVE_BACKEND_CONSONANCE
    //~ DBUS_BACKEND(CS)
//~ #endif
//~ #if HAVE_BACKEND_MUINE
    //~ DBUS_BACKEND(MU)
//~ #endif
//~ #if HAVE_BACKEND_BANSHEE
    //~ DBUS_BACKEND(BA)
//~ #endif
END_BACKEND_MAP()

/* internal functions */
#define UNSET(t) sd->player.t = NULL
static void squeezebox_init_backend(SqueezeBoxData * sd, gint nBackend) {
	// clear previous backend
	const Backend *ptr = &squeezebox_get_backends()[nBackend - 1];
    LOG("Enter squeezebox_init_backend");
    if (sd->player.Detach) {
		sd->player.Detach(sd->player.db);
		g_free(sd->player.db);
	}
	UNSET(Assure);
	UNSET(Next);
	UNSET(Previous);
	UNSET(PlayPause);
	UNSET(PlayPlaylist);
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
	UNSET(UpdateDBUS);
    // clear old property address map
    if(g_hash_table_size(sd->propertyAddresses)) {
        g_hash_table_remove_all(sd->propertyAddresses);
    }

	// clear current song info
	g_string_set_size(sd->player.artist, 0);
	g_string_set_size(sd->player.album, 0);
	g_string_set_size(sd->player.title, 0);
	g_string_set_size(sd->player.albumArt, 0);
	
	// clear playlists
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sd->mnuPlayLists), NULL);
    if(g_hash_table_size(sd->player.playLists)) {
        g_hash_table_remove_all(sd->player.playLists);
    }
	

	// call init of backend
	sd->backend = nBackend;
	sd->player.db = ptr->BACKEND_attach(&sd->player);

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

	// try connect happens in created backend
    if(sd->player.Persist)
        sd->player.Persist(sd->player.db, FALSE);
    
    // dbus backends need a trigger to awake
    // BOOLEAN NameHasOwner (in STRING name)
    if(dbusBackend == ptr->BACKEND_TYPE) {
    	// this type must have ptr->BACKEND_dbusName && sd->player.UpdateDBUS
		const gchar *dbusName = ptr->BACKEND_dbusName();
		gboolean hasOwner = FALSE;
		GError *error = NULL;
		if(org_freedesktop_DBus_name_has_owner(sd->player.dbService, dbusName, &hasOwner, NULL)) {
			if(hasOwner) {
				gchar *ownerName = NULL;
				org_freedesktop_DBus_get_name_owner(sd->player.dbService, dbusName, &ownerName, NULL);
				squeezebox_dbus_update(sd->player.dbService, dbusName, "", ownerName, sd);
			} else {
				squeezebox_dbus_update(sd->player.dbService, dbusName, "zzz", "", sd);
			}
		} else {
			LOGWARN("Can't ask DBUS: %s", error->message);
			g_error_free(error);
		}
    }
	gtk_widget_set_sensitive(sd->mnuPlayLists, g_hash_table_size(sd->player.playLists));
    
    LOG("Leave squeezebox_init_backend");
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

void on_window_opened(WnckScreen * screen, WnckWindow * window, SqueezeBoxData *sd) {
	if(sd->player.playerPID && sd->player.UpdateWindow ) 
		if(sd->player.playerPID == wnck_window_get_pid (window))
			sd->player.UpdateWindow(sd->player.db, window, TRUE);
}

void on_window_closed(WnckScreen * screen, WnckWindow * window, SqueezeBoxData *sd) {
	if(sd->player.playerPID && sd->player.UpdateWindow ) 
		if(sd->player.playerPID == wnck_window_get_pid (window))
			sd->player.UpdateWindow(sd->player.db, window, FALSE);
}

void on_mnuPlaylistItemActivated(GtkMenuItem *menuItem, SqueezeBoxData *sd) {
	gchar *playlistName = (gchar*)gtk_object_get_data(GTK_OBJECT(menuItem), "listname");
	LOG("Switch to playlist '%s'", playlistName);
	if(sd->player.PlayPlaylist)
		sd->player.PlayPlaylist(sd->player.db, playlistName);
}
void addMenu(gchar *key, gchar *value, SqueezeBoxData *sd) {
	GtkWidget *menu, *subMenuItem, *image;
	LOG("Adding submenu '%s' with icon '%s'", key, value);
	menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(sd->mnuPlayLists));
	subMenuItem = gtk_image_menu_item_new_with_label(key);
	/*
	GtkPixmap *pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
							  value, GTK_ICON_SIZE_MENU, 0, NULL);
							  */
	image = gtk_image_new_from_icon_name(value, GTK_ICON_SIZE_MENU);
	if(image)
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subMenuItem), image);
	gtk_widget_show(subMenuItem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), subMenuItem);
	g_object_ref(subMenuItem);
	gtk_object_set_data(GTK_OBJECT(subMenuItem), "listname", g_strdup(key));
	g_signal_connect(G_OBJECT(subMenuItem), "activate",
			 G_CALLBACK(on_mnuPlaylistItemActivated), sd);
}

void add(gchar *key, SqueezeBoxData *sd) {
	addMenu(key, g_hash_table_lookup(sd->player.playLists, key), sd);
}

void addList(gchar *key, gpointer *value, GList **list) {
	*list = g_list_append(*list, g_strdup(key));
}

static void squeezebox_update_playlists(gpointer thsPlayer) {
	SqueezeBoxData *sd = (SqueezeBoxData *) thsPlayer;
	gboolean hasItems = sd->player.playLists != NULL &&
		g_hash_table_size(sd->player.playLists) > 0;
	LOG("Enter squeezebox_update_playlists");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(sd->mnuPlayLists), NULL);
	if(hasItems) {
		GtkWidget *menu = gtk_menu_new();
		GList *list = NULL;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(sd->mnuPlayLists), menu);
		g_hash_table_foreach(sd->player.playLists, (GHFunc)addList, &list);
		list = g_list_sort(list, (GCompareFunc)g_ascii_strcasecmp);
		g_list_foreach(list, (GFunc)add, sd);
		g_list_free(list);
	}
	gtk_widget_set_sensitive(sd->mnuPlayLists, hasItems);
	LOG("Leave squeezebox_update_playlists");
}

static void squeezebox_update_repeat(gpointer thsPlayer, gboolean newRepeat) {
	MKTHIS;
	LOG("Enter squeezebox_update_repeat");
	sd->noUI = TRUE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuRepeat),
				       newRepeat);
	sd->noUI = FALSE;
	LOG("Leave squeezebox_update_repeat");
}

static void squeezebox_update_shuffle(gpointer thsPlayer, gboolean newShuffle) {
	MKTHIS;
	LOG("Enter squeezebox_update_shuffle");
	sd->noUI = TRUE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuShuffle),
				       newShuffle);
	sd->noUI = FALSE;
	LOG("Enter squeezebox_update_shuffle");
}

static void
squeezebox_update_visibility(gpointer thsPlayer, gboolean newVisible) {
	MKTHIS;
	LOG("Enter squeezebox_update_visibility");
	sd->noUI = TRUE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuPlayer),
				       newVisible);
	sd->noUI = FALSE;
	LOG("Leave squeezebox_update_visibility");
}

#if HAVE_ID3TAG
// this shippet is from gtkpod
static const gchar* id3_get_binary (struct id3_tag const *tag,
				    char *frame_name,
				    id3_length_t *len,
				    int index)
{
    const id3_byte_t *binary = NULL;
    struct id3_frame *frame;
    union id3_field *field;

    g_return_val_if_fail (len, NULL);

    *len = 0;

    frame = id3_tag_findframe (tag, frame_name, index);

    if (!frame) 
    	return NULL;

    /* The last field contains the data */
    field = id3_frame_field (frame, frame->nfields-1);

    if (!field) 
    	return NULL;

    switch (field->type) {
		case ID3_FIELD_TYPE_BINARYDATA:
			binary = id3_field_getbinarydata(field, len);
			break;
		default:
			break;
    }

    return (gchar*)binary;
}

#endif

static void
squeezebox_find_albumart_by_filepath(gpointer thsPlayer, const gchar * path) {
	MKTHIS;
	gboolean bFound = FALSE;
#if HAVE_ID3TAG	// how to query artist with id3tag

	LOG("SB Enter Check #1:'%s' embedded art", path);
	struct id3_file *fp = id3_file_open(path, ID3_FILE_MODE_READONLY);
	if (fp) {
		struct id3_tag const *tag = id3_file_tag(fp);
		if (tag) {
			const gchar *coverart = NULL;
			int i;
			id3_length_t len = 0;
			struct id3_frame *frame = NULL;
			/* Loop through APIC tags and set coverart. The picture type should be
			* 3 -- Cover (front), but iTunes has been known to use 0 -- Other. */
			for (i = 0; (frame = id3_tag_findframe(tag, "APIC", i)) != NULL; i++) {
				union id3_field *field = id3_frame_field (frame, 2);
				if(field) {
					int pictype = field->number.value;
					LOG("Found apic type %d\n", pictype);

					/* We'll prefer type 3 (cover) over type 0 (other) */
					if (pictype == 3) {
						coverart = id3_get_binary(tag, "APIC", &len, i);
					}
					if ((pictype == 0) && !coverart) {
						coverart = id3_get_binary(tag, "APIC", &len, i);
						break;
					}
				}
			}
			if(coverart) {
				GdkPixbuf *pic = NULL;
				gchar *tmpName = g_strdup_printf("%s/squeezeboxaa",
					g_get_tmp_dir());
				if(tmpName && g_file_set_contents(tmpName, coverart, len, NULL)) {
					g_string_assign(sd->player.albumArt, tmpName);
					 pic = gdk_pixbuf_new_from_file_at_size(tmpName,
						 64, 64, NULL);
					if(pic) {
						bFound = TRUE;
						g_object_unref(pic);
					}
					LOG("Image is %s", (bFound)?"valid":"invalid");
				}
				g_free(tmpName);
			}		
		}
		id3_file_close(fp);
	}
#endif
	if(!bFound) {
		gchar *strNext = g_path_get_dirname(path);
		GDir *dir = g_dir_open(strNext, 0, NULL);
		LOG("SB Enter Check #2:'%s/[.][folder|cover|front].jpg'", strNext);
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
	}
	LOG("SB: Leave Check");
}

static void
squeezebox_update_UI(gpointer thsPlayer, gboolean updateSong,
		     eSynoptics State, const gchar * playerMessage) {
	MKTHIS;

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
		if (sd->notify) {
			squeezebox_update_UI_show_toaster(thsPlayer);
		}
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

static void
squeezebox_style_set(XfcePanelPlugin * plugin, gpointer ignored,
		     SqueezeBoxData * sd) {
	squeezebox_set_size(plugin, xfce_panel_plugin_get_size(plugin), sd);
}

static GdkFilterReturn
filter_mmkeys(GdkXEvent * xevent, GdkEvent * event, gpointer data) {
	SqueezeBoxData * sd = (SqueezeBoxData *) data;
	XEvent *xev;
	XKeyEvent *key;

	xev = (XEvent *) xevent;
	if (xev->type == KeyPress) {
		key = (XKeyEvent *) xevent;

		if (0 != key->keycode && 0 != key->state) {
			return GDK_FILTER_REMOVE;
		} 
	
	}
	return GDK_FILTER_CONTINUE;
}

static void squeezebox_free_data(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {
	LOG("Enter squeezebox_free_data");
#ifndef HAVE_GTK_2_12
	if (sd->tooltips) {
		g_object_unref(sd->tooltips);
		sd->tooltips = NULL;
	}
#endif
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
	if (sd->channel) {
		g_object_unref(sd->channel);
	}
	if (sd->player.Detach) {
		sd->player.Detach(sd->player.db);
		g_free(sd->player.db);
	}
  	if (sd->grabmedia) {
		int idx;
		GdkDisplay *display;
		GdkScreen *screen;
		GdkWindow *root;
		for(idx = 0; idx < 5; idx++) {
			gchar *name = NULL, *path1 = NULL, *path2 = NULL;
			guint accelKey;
			guint accelKeyMask;
			path1 = g_strdup_printf("/MediaKeys/Key%dValue", idx);
			path2 = g_strdup_printf("/MediaKeys/Key%dMask", idx);
			accelKey = xfconf_channel_get_uint(sd->channel, path1, 0);		
			accelKeyMask = xfconf_channel_get_uint(sd->channel, path2, 0);
			if(0 != accelKey && 0 != accelKeyMask)		
				squeezebox_ungrab_key(accelKey, accelKeyMask, sd);
			g_free(path1);
			g_free(path2);
		}
		for (idx = 0; idx < gdk_display_get_n_screens(display); idx++) {
			screen = gdk_display_get_screen(display, idx);

			if (screen != NULL) {
				root = gdk_screen_get_root_window(screen);
				gdk_window_remove_filter(root, filter_mmkeys, sd);		
			}
		}
    }	
	xfconf_shutdown();
	g_free(sd);
	LOG("Leave squeezebox_free_data");
}

static void
squeezebox_read_rc_file(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {
	gint nBackend = 1;
	gboolean bShowNext = TRUE;
	gboolean bShowPrev = TRUE;
	gboolean bGrabMedia = TRUE;
	gboolean bAutoConnect = FALSE;
	gint toolTipStyle = 1;
	gboolean bNotify = TRUE;
	gdouble dNotifyTimeout = 5.0;
	LOG("Enter squeezebox_read_rc_file");

	// Read properties from channel
    sd->channel = xfconf_channel_new_with_property_base (
    	"xfce4-panel", "/plugins/squeezebox/settings");
    
	// Always init backend
	nBackend = xfconf_channel_get_uint(sd->channel, "/Backend", nBackend);
	bAutoConnect = xfconf_channel_get_bool(sd->channel, "/AutoConnect", bAutoConnect);
	sd->player.sd = sd;
	sd->autoAttach = bAutoConnect;
	squeezebox_init_backend(sd, nBackend);
	
	// Appearance
	bShowNext = xfconf_channel_get_bool(sd->channel, "/ShowNext", bShowNext);
	bShowPrev = xfconf_channel_get_bool(sd->channel, "/ShowPrev", bShowPrev);
	bNotify = xfconf_channel_get_bool(sd->channel, "/Notifications/Show", bNotify);
	dNotifyTimeout = xfconf_channel_get_double(sd->channel, 
		"/Notifications/TimeOut", dNotifyTimeout);
	sd->player.updateRateMS = xfconf_channel_get_uint(sd->channel, "/UpdateRateMS", 500);
	toolTipStyle = xfconf_channel_get_uint(sd->channel, "/Tooltips/Style", 1);
	if (toolTipStyle < 0)
		toolTipStyle = 0;
	if (toolTipStyle > 1)
		toolTipStyle = 1;
	sd->show[ebtnNext] = bShowNext;
	sd->show[ebtnPlay] = TRUE;	// well, maybe not later
	sd->show[ebtnPrev] = bShowPrev;
	sd->notify = bNotify;
	sd->notifyTimeout = dNotifyTimeout;
	
	
	LOG("Attach %d", sd->notify);
	if (toolTipStyle > ettSimple)
		toolTipStyle = ettSimple;

	if (toolTipStyle < ettNone)
		toolTipStyle = ettNone;
	sd->toolTipStyle = toolTipStyle;
#ifndef HAVE_GTK_2_12
	if (sd->toolTipStyle == ettSimple)
		gtk_tooltips_enable(sd->tooltips);
	else
		gtk_tooltips_disable(sd->tooltips);
#endif

	// Media buttons
	bGrabMedia = xfconf_channel_get_bool(sd->channel, "/MediaKeys/Grab", bGrabMedia);
	sd->grabmedia = bGrabMedia;
  	if (bGrabMedia) {
		int idx;
		GdkDisplay *display;
		GdkScreen *screen;
		GdkWindow *root;
		for(idx = 0; idx < 5; idx++) {
			gchar *name = NULL, *path1 = NULL, *path2 = NULL;
			guint accelKey;
			guint accelKeyMask;
			path1 = g_strdup_printf("/MediaKeys/Key%dValue", idx);
			path2 = g_strdup_printf("/MediaKeys/Key%dMask", idx);
			accelKey = xfconf_channel_get_uint(sd->channel, path1, 0);		
			accelKeyMask = xfconf_channel_get_uint(sd->channel, path2, 0);
			if(0 != accelKey && 0 != accelKeyMask)		
				squeezebox_grab_key(accelKey, accelKeyMask, sd);
			g_free(path1);
			g_free(path2);
		}
		for (idx = 0; idx < gdk_display_get_n_screens(display); idx++) {
			screen = gdk_display_get_screen(display, idx);

			if (screen != NULL) {
				root = gdk_screen_get_root_window(screen);
				gdk_window_add_filter(root, filter_mmkeys, sd);		
			}
		}
    }	
	LOG("Leave squeezebox_read_rc_file");
}

static void
squeezebox_write_rc_file(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {

	LOG("Enter squeezebox_write_rc_file");

	// all this could be written during dialog visibility, too
	xfconf_channel_set_uint(sd->channel, "/Backend", sd->backend);
	xfconf_channel_set_bool(sd->channel, "/AutoConnect", sd->autoAttach);
	xfconf_channel_set_bool(sd->channel, "/ShowNext", sd->show[ebtnNext]);
	xfconf_channel_set_bool(sd->channel, "/ShowPrev", sd->show[ebtnPrev]);
	xfconf_channel_set_bool(sd->channel, "/MediaKeys/Grab", sd->grabmedia);
	xfconf_channel_set_bool(sd->channel, "/Notifications/Show", sd->notify);
	xfconf_channel_set_double(sd->channel, "/Notifications/TimeOut", sd->notifyTimeout);
	xfconf_channel_set_uint(sd->channel, "/UpdateRateMS", sd->player.updateRateMS);
	xfconf_channel_set_uint(sd->channel, "/Tooltips/Style", sd->toolTipStyle);

    // notify properties are about to be written
    if(sd->player.Persist)
        sd->player.Persist(sd->player.db, TRUE);

	LOG("Leave squeezebox_write_rc_file");
}

EXPORT void on_dialogSettings_response(GtkWidget * dlg, int reponse, SqueezeBoxData * sd) {
    LOG("Enter DialogResponse");
	gtk_widget_destroy(dlg);
	xfce_panel_plugin_unblock_menu(sd->plugin);
	squeezebox_write_rc_file(sd->plugin, sd);
    LOG("Leave DialogResponse");
}

EXPORT void on_btnEdit_clicked(GtkButton * btn, SqueezeBoxData * sd) {
	LOG("Enter config_show_backend_properties");
	if (sd->player.Configure)
		sd->player.Configure(sd->player.db, GTK_WIDGET(sd->dlg));
	else {
		GtkWidget *dlg =
		    gtk_message_dialog_new(GTK_WINDOW
					   (g_object_get_data
					    (G_OBJECT(sd->plugin), "dialog")),
					   GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
					   GTK_BUTTONS_OK,
					   _
					   ("This backend has no configurable properties"));

		gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(sd->dlg));
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
	}
	LOG("Leave config_show_backend_properties");
}

typedef enum eColumns{
	PIXBUF_COLUMN,
	TEXT_COLUMN,
    INDEX_COLUMN,
    AVAIL_COLUMN,
	N_COLUMNS,
}eColumns;

const Backend* squeezebox_get_current_backend(SqueezeBoxData * sd)
{
	return &squeezebox_get_backends()[sd->backend -1];
}

EXPORT void on_tvPlayers_cursor_changed(GtkTreeView * tv, SqueezeBoxData * sd) {
	if(!sd->autoAttach) {
		GtkTreeIter iter = {0};
		GtkTreeSelection *selection = gtk_tree_view_get_selection(tv);
		GtkTreeModel *model = NULL;
		GtkWidget *button = GTK_WIDGET(g_object_get_data(G_OBJECT(sd->dlg), "btnEdit"));
		LOG("Backend change...");
		if( gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gint nBackend = -1;
			LOG("Have iter %d", iter.stamp);
			if(model) {
				gchar *backend = NULL;
				gtk_tree_model_get(model, &iter, 
					TEXT_COLUMN, &backend, 
					INDEX_COLUMN, &nBackend, -1);
				LOG("Have model %s %d", backend, nBackend);
				squeezebox_init_backend(sd, nBackend + 1);
				gtk_widget_set_sensitive(button,
					NULL != sd->player.Configure);
			}
		}
	}
}

EXPORT void on_cellrenderertoggle1_toggled(GtkCellRendererToggle * crt, 
		gchar                *path_string,
		SqueezeBoxData * sd) {
	GtkTreeIter iter = {0};
	GtkTreeView *view = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(sd->dlg), "tvPlayers"));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
	GtkTreeModel *model = NULL;
	gboolean active = !gtk_cell_renderer_toggle_get_active(crt);
	LOG("ClickCRT %d", active);
	if( gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
			AVAIL_COLUMN, active, -1);
	}
}
gboolean squeezebox_grab_key(guint accel_key, guint accel_mods, SqueezeBoxData *sd)
{
	int iErr = 0, i;
	GdkDisplay *display;
	GdkScreen *screen;
	GdkWindow *root;

	display = gdk_display_get_default();
	for (i = 0; i < gdk_display_get_n_screens(display); i++) {
		screen = gdk_display_get_screen(display, i);

		if (screen != NULL) {
			root = gdk_screen_get_root_window(screen);
			gdk_error_trap_push();

			XGrabKey(GDK_DISPLAY(), accel_key,
				 accel_mods, GDK_WINDOW_XID(root), True, GrabModeAsync, GrabModeAsync);
			gdk_flush();
			iErr = gdk_error_trap_pop();
			if (iErr) {
				LOG("Error grabbing key %d, %p", accel_key, root);
				break;
			} else
				LOG("Grabbed key %d", accel_key);
		}
	}
	return (iErr == 0);
}

void squeezebox_ungrab_key(guint accel_key, guint accel_mods, SqueezeBoxData *sd)
{
	int iErr = 0, i;
	GdkDisplay *display;
	GdkScreen *screen;
	GdkWindow *root;

	display = gdk_display_get_default();
	for (i = 0; i < gdk_display_get_n_screens(display); i++) {
		screen = gdk_display_get_screen(display, i);

		if (screen != NULL) {
			root = gdk_screen_get_root_window(screen);
			gdk_error_trap_push();

			XUngrabKey(GDK_DISPLAY(), accel_key, accel_mods, GDK_WINDOW_XID(root));
			gdk_flush();
			iErr = gdk_error_trap_pop();
			if (iErr) {
				LOG("Error ungrabbing key %d, %p", accel_key, root);
				break;
			} else
				LOG("Ungrabbed key %d", accel_key);
		}
	}
}

EXPORT void on_cellrenderShortCut_accel_cleared(GtkCellRendererAccel *accel,
		gchar                *path_string,
		SqueezeBoxData		 *sd) {
	GtkListStore *storeShortCuts = g_object_get_data(G_OBJECT(sd->dlg), "liststoreShortcuts");
	GtkTreeIter iter = {0};
	guint                 accel_key;
	GdkModifierType       accel_mods;
	gchar *path1 = NULL;
	gchar *path2 = NULL;
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(storeShortCuts), &iter, path_string)) {
		gtk_tree_model_get(GTK_TREE_MODEL(storeShortCuts), &iter,
			2, &accel_mods, 1, &accel_key, -1);	
		squeezebox_ungrab_key(accel_key, accel_mods, sd);
		gtk_list_store_set(storeShortCuts, &iter, 
			2, 0, 1, 0, -1);
		path1 = g_strdup_printf("/MediaKeys/Key%sValue", path_string);
		path2 = g_strdup_printf("/MediaKeys/Key%sMask", path_string);
		xfconf_channel_set_uint(sd->channel, path1, 0);		
		xfconf_channel_set_uint(sd->channel, path2, 0);		
		g_free(path1);
		g_free(path2);
	}
}

EXPORT void on_cellrenderShortCut_accel_edited(GtkCellRendererAccel *accel,
		gchar                *path_string,
		guint                 accel_key,
		GdkModifierType       accel_mods,
		guint                 hardware_keycode,
		SqueezeBoxData		 *sd){ 
	GtkListStore *storeShortCuts = g_object_get_data(G_OBJECT(sd->dlg), "liststoreShortcuts");
	GtkTreeIter iter = {0};
	gchar *path1 = NULL;
	gchar *path2 = NULL;
	LOG("Accel %s %d %d", path_string, accel_key, accel_mods);
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(storeShortCuts), &iter, path_string)) {
		if(squeezebox_grab_key(accel_key, accel_mods, sd)) {		
			gtk_list_store_set(storeShortCuts, &iter, 
				2, accel_mods, 1, accel_key, -1);
			path1 = g_strdup_printf("/MediaKeys/Key%sValue", path_string);
			path2 = g_strdup_printf("/MediaKeys/Key%sMask", path_string);
			xfconf_channel_set_uint(sd->channel, path1, accel_key);		
			xfconf_channel_set_uint(sd->channel, path2, accel_mods);		
			g_free(path1);
			g_free(path2);
		}
	}
}

static void
squeezebox_properties_dialog(XfcePanelPlugin * plugin, SqueezeBoxData * sd) {
	// backends
	GtkListStore *store;
	GtkTreeIter iter = { 0 };
	const Backend *ptr = squeezebox_get_backends();
    gint idx;
	GtkCellRenderer *renderer;
	GdkPixbuf *pix;
	GtkTreeView *view;
	gboolean valid;
	
	xfce_panel_plugin_block_menu(plugin);

    // new
    GtkBuilder* builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "settings.ui", NULL);
	sd->dlg = GTK_WIDGET(gtk_builder_get_object(builder, "dialogSettings"));
	store = GTK_LIST_STORE(gtk_builder_get_object(builder, "datastore"));
	view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tvPlayers"));
	// the button sensitivity update
	g_object_set_data(G_OBJECT(sd->dlg), 
		"btnAdd", gtk_builder_get_object(builder, "btnAdd"));
	g_object_set_data(G_OBJECT(sd->dlg), 
		"btnEdit", gtk_builder_get_object(builder, "btnEdit"));
	g_object_set_data(G_OBJECT(sd->dlg), 
		"btnRemove", gtk_builder_get_object(builder, "btnRemove"));
	//cellrenderertoggle1
	g_object_set_data(G_OBJECT(sd->dlg), 
		"tvcEnabled", gtk_builder_get_object(builder, "tvcEnabled"));
	g_object_set_data(G_OBJECT(sd->dlg), 
		"tvPlayers", gtk_builder_get_object(builder, "tvPlayers"));
	g_object_set_data(G_OBJECT(sd->dlg), 
		"liststoreShortcuts", gtk_builder_get_object(builder, "liststoreShortcuts"));
	g_object_set_data(G_OBJECT(sd->dlg), 
		"datastore", gtk_builder_get_object(builder, "datastore"));

	/* fill the backend store with data */
	idx = 0;
	for (;;) {
		LOG("Have %s", ptr->BACKEND_name());
		gtk_list_store_append(store, &iter);
		pix = exo_gdk_pixbuf_scale_down(ptr->BACKEND_icon(), TRUE, 24, 32);
		gtk_list_store_set(store, &iter, 
                   PIXBUF_COLUMN, pix,
				   TEXT_COLUMN, ptr->BACKEND_name(), 
                   INDEX_COLUMN, idx++, 
				   AVAIL_COLUMN, TRUE, -1);
		if( sd->backend == idx) {
			GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
			gtk_tree_selection_select_iter(selection, &iter);
			gtk_widget_set_sensitive(
				GTK_WIDGET((gtk_builder_get_object(builder, "btnEdit"))),
			 	NULL != sd->player.Configure);
		}
		
		ptr++;
		if (!ptr->BACKEND_name)
			break;
	}
	
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(store), TEXT_COLUMN, GTK_SORT_ASCENDING);

	// Appearance tab
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkAutoAttach")), sd->autoAttach);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkShowPrevious")), sd->show[ebtnPrev]);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkShowNext")), sd->show[ebtnNext]);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkShowToolTips")), sd->toolTipStyle);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkShowNotifications")), sd->notify);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinNotificationTimeout")), sd->notifyTimeout);
	
	// Shortcuts tab
	store = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststoreShortcuts"));	
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter);
	idx = 0;
  	while (valid)
    {
		gchar *name = NULL, *path1 = NULL, *path2 = NULL;
		guint accelKey;
		guint accelKeyMask;
		gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
						  0, &name, -1);
		path1 = g_strdup_printf("/MediaKeys/Key%dValue", idx);
		path2 = g_strdup_printf("/MediaKeys/Key%dMask", idx);
		accelKey = xfconf_channel_get_uint(sd->channel, path1, 0);		
		accelKeyMask = xfconf_channel_get_uint(sd->channel, path2, 0);		
		gtk_list_store_set(store, &iter, 
			2, accelKeyMask, 1, accelKey, -1);
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &iter);
		idx++;
		g_free(path1);
		g_free(path2);
    }	
	// liftoff
	gtk_builder_connect_signals(builder, sd);
	gtk_dialog_run(GTK_DIALOG(sd->dlg));
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
	gboolean bRet = FALSE;
	LOG("Enter squeezebox_play");
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
		if(NULL != sd->player.IsVisible)
			squeezebox_update_visibility(sd,
						     sd->player.IsVisible(sd->player.db));
		sd->noUI = FALSE;
	} else if (2 == event->button) {
        LOG("MiddleClick");
		if (NULL == sd->player.IsVisible && NULL != sd->player.Show) {
			sd->player.Show(sd->player.db, TRUE);
		}
		if (NULL != sd->player.IsVisible && NULL != sd->player.Show) {
			sd->player.Show(sd->player.db,
					!sd->player.IsVisible(sd->player.db));
			squeezebox_update_visibility(sd,
						     sd->player.IsVisible(sd->player.db));
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

EXPORT void on_chkAutoAttach_toggled(GtkToggleButton *button, SqueezeBoxData * sd) {
	sd->autoAttach = gtk_toggle_button_get_active(button);
	gtk_tree_view_column_set_visible(
		GTK_TREE_VIEW_COLUMN(g_object_get_data(G_OBJECT(sd->dlg), "tvcEnabled")), 
		sd->autoAttach);
}

EXPORT void on_chkShowPrevious_toggled(GtkToggleButton *button, SqueezeBoxData *sd) {
	gboolean *pBtn = &sd->show[ebtnPrev];
	gint size = xfce_panel_plugin_get_size(sd->plugin);

	*pBtn = gtk_toggle_button_get_active(button);

	if (*pBtn)
		gtk_widget_show(sd->button[ebtnPrev]);
	else
		gtk_widget_hide(sd->button[ebtnPrev]);

	squeezebox_set_size(sd->plugin, size, sd);
}

EXPORT void on_chkShowNext_toggled(GtkToggleButton *button, SqueezeBoxData *sd) {
	gboolean *pBtn = &sd->show[ebtnNext];
	gint size = xfce_panel_plugin_get_size(sd->plugin);

	*pBtn = gtk_toggle_button_get_active(button);

	if (*pBtn)
		gtk_widget_show(sd->button[ebtnNext]);
	else
		gtk_widget_hide(sd->button[ebtnNext]);

	squeezebox_set_size(sd->plugin, size, sd);
}

EXPORT void on_chkShowToolTips_toggled(GtkToggleButton *button, SqueezeBoxData *sd) {
	if (gtk_toggle_button_get_active(button)) {
		sd->toolTipStyle = ettSimple;
#ifndef HAVE_GTK_2_12
		gtk_tooltips_enable(sd->tooltips);
#endif
	} else {
		sd->toolTipStyle = ettNone;
#ifndef HAVE_GTK_2_12
		gtk_tooltips_disable(sd->tooltips);
#endif
	}
}

EXPORT void on_chkShowNotifications_toggled(GtkToggleButton *button, SqueezeBoxData *sd) {
	sd->notify = gtk_toggle_button_get_active(button);
}

EXPORT void on_spinNotificationTimeout_change_value(GtkSpinButton *button, SqueezeBoxData *sd) {
	sd->notifyTimeout = gtk_spin_button_get_value(button);
}

void squeezebox_next(SqueezeBoxData * sd) {
	LOG("Enter squeezebox_next");
	if (sd->player.Next)
		sd->player.Next(sd->player.db);
	LOG("Leave squeezebox_next");
}

void squeezebox_map_property(gpointer thsPlayer, const gchar *propName, gpointer address) {
	MKTHIS;
    LOG("Mapping property %s to %p", propName, address);
    g_hash_table_insert(sd->propertyAddresses, g_strdup(propName), address);
}

void on_btnNext_clicked(GtkButton * button, SqueezeBoxData * sd) {
	squeezebox_next(sd);
}
void on_keyNext_clicked(gpointer noIdea1, int noIdea2, SqueezeBoxData * sd) {
	squeezebox_next(sd);
}

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
		GdkPixbuf *pic = NULL;
		GString * albumArt = sd->player.albumArt;
		gtk_tooltip_set_text(tooltip, sd->toolTipText->str);
		if (albumArt->len && g_file_test(albumArt->str, G_FILE_TEST_EXISTS))
			pic = gdk_pixbuf_new_from_file_at_size(albumArt->str, 32, 32, NULL);
		else
			pic = squeezebox_get_backends(sd)[sd->backend - 1].BACKEND_icon();
		gtk_tooltip_set_icon(tooltip, pic);
		return TRUE;
	}
	return FALSE;
}
#endif

static GtkContainer *squeezebox_create(SqueezeBoxData * sd) {
	GtkContainer *window1 = GTK_CONTAINER(sd->plugin);
	GError *error = NULL;
	LOG("Enter squeezebox_create");
	xfconf_init(&error);
	
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

	if (sd->state != ebtnPlay)
		squeezebox_update_playbtn(sd);

	LOG("Leave squeezebox_create");
	return window1;
}

static void squeezebox_dbus_update(DBusGProxy * proxy, const gchar * Name,
				   const gchar * OldOwner,
				   const gchar * NewOwner,
				   SqueezeBoxData * sd) {
	if (sd->backend) {
		const Backend *ptr = squeezebox_get_backends();
		ptr += (sd->backend - 1);
		if (ptr->BACKEND_dbusName) {
			gboolean appeared = (NULL != NewOwner && 0 != NewOwner[0]);
			const gchar *dbusName = ptr->BACKEND_dbusName();
			if (!g_ascii_strcasecmp(Name, dbusName)) {
				LOG("DBUS name change %s: '%s'->'%s'", Name, OldOwner, NewOwner);
				if(sd->player.UpdateDBUS)	
					sd->player.UpdateDBUS(sd->player.db, appeared);
				sd->player.playerPID = 0;
				if(appeared 
				    && org_freedesktop_DBus_get_connection_unix_process_id(
					sd->player.dbService, NewOwner, &sd->player.playerPID, NULL)
					&& sd->player.playerPID > 0) {
					// now we got the process
				}
			}
		}
	}
}
static gboolean squeezebox_dbus_start_service(gpointer thsPlayer) {
	MKTHIS;
	GError *error = NULL;
	guint start_service_reply;
	gboolean bRet = TRUE;
	Backend const *ptr = &squeezebox_get_backends()[sd->backend -1];
	gchar *path = NULL;
	
	LOG("Starting new instance of '%s'", ptr->BACKEND_dbusName());
	if (!dbus_g_proxy_call(sd->player.dbService, "StartServiceByName", &error,
			 G_TYPE_STRING, ptr->BACKEND_dbusName(),
			 G_TYPE_UINT, 0, G_TYPE_INVALID,
			 G_TYPE_UINT, &start_service_reply,
			 G_TYPE_INVALID)) {
		bRet = FALSE;
		LOG("Could'n start service '%s'", error->message);
		g_error_free(error);
		path = g_find_program_in_path(ptr->BACKEND_commandLine());
		if(NULL != path) {
			const gchar *argv[] = {
				path,
				// here we could have arguments
				NULL
			};
			bRet = g_spawn_async(NULL, (gchar**)argv, NULL, 
				G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL,
				NULL, NULL, NULL, NULL);
			LOG("Spawn '%s' %s", path, (bRet)?"OK.":"failed.");
		}
	}
	return bRet;
}
static void squeezebox_construct(XfcePanelPlugin * plugin) {
	int i = 0;
	SqueezeBoxData *sd = g_new0(SqueezeBoxData, 1);

	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	LOG("Enter squeezebox_construct");

	sd->plugin = plugin;
    sd->properties = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    sd->propertyAddresses = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	sd->player.artist = g_string_new(_("(unknown)"));
	sd->player.album = g_string_new(_("(unknown)"));
	sd->player.title = g_string_new(_("(unknown)"));
	sd->player.albumArt = g_string_new("");
    sd->player.playLists = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	sd->player.Update = squeezebox_update_UI;
	sd->player.UpdatePlaylists = squeezebox_update_playlists;
	sd->player.UpdateShuffle = squeezebox_update_shuffle;
	sd->player.UpdateRepeat = squeezebox_update_repeat;
	sd->player.UpdateVisibility = squeezebox_update_visibility;
	sd->player.bus = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	sd->player.StartService = squeezebox_dbus_start_service;
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
					    G_CALLBACK(squeezebox_dbus_update),
					    sd, NULL);
					    
		// notifications
		sd->note = NULL;
		sd->note = dbus_g_proxy_new_for_name(sd->player.bus,
								 "org.freedesktop.Notifications",
								 "/org/freedesktop/Notifications",
								 "org.freedesktop.Notifications");
							
	}
	sd->player.FindAlbumArtByFilePath =
	    squeezebox_find_albumart_by_filepath;
    sd->player.MapProperty = squeezebox_map_property;
	sd->inEnter = FALSE;
	sd->notifyTimeout = 5;
	sd->notifyID = 0;
	sd->inCreate = TRUE;
	sd->toolTipText = g_string_new("");
	sd->wnckScreen =
	    wnck_screen_get(gdk_screen_get_number(gdk_screen_get_default()));

	/* wnck */
	g_signal_connect(sd->wnckScreen, "window_opened",
			 G_CALLBACK(on_window_opened), sd);

	g_signal_connect(sd->wnckScreen, "window_closed",
			 G_CALLBACK(on_window_closed), sd);
	

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

	sd->mnuPlayLists = gtk_menu_item_new_with_label(_("Playlists"));
	gtk_widget_show(sd->mnuPlayLists);
	gtk_widget_set_sensitive(sd->mnuPlayLists, FALSE);
	xfce_panel_plugin_menu_insert_item(sd->plugin,
					   GTK_MENU_ITEM(sd->mnuPlayLists));
	g_object_ref(sd->mnuPlayLists);

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
	sd->inCreate = FALSE;
	LOG("Leave squeezebox_construct");

}

//XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (squeezebox_construct);
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(squeezebox_construct);
