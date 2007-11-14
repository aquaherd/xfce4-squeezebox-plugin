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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

#include "squeezebox.h"
#include <libintl.h>

typedef enum tag_eButtons
{
	ebtnPrev = 0,
	ebtnPlay = 1,
	ebtnNext = 2
}eButtons;

typedef enum tag_eTTStyle
{
	ettNone = 0,
	ettSimple = 1,
#if HAVE_NOTIFY
	ettFull = 2
#endif
}eTTStyle;	

typedef struct
{
    XfcePanelPlugin *plugin;
    gulong style_id;

    GtkWidget *button[3];
    GtkWidget *image[3];
	gboolean  show[3];
	GtkWidget *table;

	#if HAVE_NOTIFY
	gboolean  notify;
    gboolean  inEnter;
	NotifyNotification *note;
	gint      notifytimeout;
    gint      timerCount;
    guint     timerHandle;
	#endif
    
    // menu items
    GtkWidget 
        *mnuShuffle,
        *mnuRepeat,
        *mnuPlayer;
    gboolean noUI;

	gint        toolTipStyle;
    GtkTooltips *tooltips;
    GString     *toolTipText;

	gint backend;
	SPlayer player;
	eSynoptics state;
}
SqueezeBoxData;

/* some small helpers */
static void lose (const char *fmt, ...) G_GNUC_NORETURN G_GNUC_PRINTF (1, 2);
static void lose_gerror (const char *prefix, GError *error) G_GNUC_NORETURN;

/* Panel Plugin Interface */

static void squeezebox_properties_dialog (XfcePanelPlugin *plugin, 
                                       SqueezeBoxData *sd);
static void squeezebox_construct (XfcePanelPlugin * plugin);

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (squeezebox_construct);

/* Backend mapping */ 
IMPORT_BACKEND(DBUS)
IMPORT_BACKEND(MPD)
IMPORT_BACKEND(QL)

BEGIN_BACKEND_MAP()
    BACKEND(DBUS)
    BACKEND(MPD)
	BACKEND(QL)
END_BACKEND_MAP()

/* internal functions */

static void 
squeezebox_init_backend(SqueezeBoxData *sd, gint nBackend)
{
    // call init of backend
    sd->backend = nBackend;
    const Backend *ptr = squeezebox_get_backends();
    sd->player.db = ptr[nBackend-1].BACKEND_attach(&sd->player);

    // have menu populated
    gtk_widget_set_sensitive(sd->mnuPlayer, (NULL != sd->player.Show));
    gtk_widget_set_sensitive(sd->mnuRepeat, 
        (NULL != sd->player.GetRepeat && NULL != sd->player.SetRepeat ));
    gtk_widget_set_sensitive(sd->mnuShuffle, 
        (NULL != sd->player.GetShuffle && NULL != sd->player.SetShuffle ));
    
}

static void 
squeezebox_update_playbtn(SqueezeBoxData *sd)
{
	LOG("Enter squeezebox_update_playbtn\n");
	/* // hmmm.
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	GError *err = NULL;
	gint size = xfce_panel_plugin_get_size(sd->plugin) - 2;
	*/
	gtk_widget_destroy(sd->image[ebtnPlay]);
	
	switch(sd->state)
	{
	case estPlay:
		sd->image[ebtnPlay] = gtk_image_new_from_stock(
			GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);
		break;
	case estPause:
		sd->image[ebtnPlay] = gtk_image_new_from_stock(
			GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_MENU);
		break;
	case estStop:
		sd->image[ebtnPlay] = gtk_image_new_from_stock(
			GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_MENU);
		break;
	default:
		sd->image[ebtnPlay] = gtk_image_new_from_stock(
			GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU);
		break;
	}		
	
	gtk_widget_show (sd->image[ebtnPlay]);
	gtk_container_add(
		GTK_CONTAINER(sd->button[ebtnPlay]), sd->image[ebtnPlay]);
	LOG("Leave squeezebox_update_playbtn\n");
}

#if HAVE_NOTIFY


static void 
toaster_closed(NotifyNotification *notification, SqueezeBoxData *sd)
{
    sd->note = NULL;
}


static void 
squeezebox_update_UI_hide_toaster(gpointer thsPlayer)
{
	LOG("hide_toaster\n");	
	SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
    if( sd->note )
    {
        notify_notification_close(sd->note, NULL);
        sd->note = NULL;
    }
}    


static gboolean
on_timer(gpointer thsPlayer)
{
    SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
    if( NULL == sd->note )
        return TRUE;
	printf("CountDown %ld %ld\n", sd->timerCount, sd->notifytimeout);
    if( sd->inEnter )
        sd->timerCount = sd->notifytimeout;
    else {
        sd->timerCount--;
        if( sd->timerCount < 1 )
            squeezebox_update_UI_hide_toaster(thsPlayer);
    }
            
    return TRUE;
}
static void
squeezebox_update_UI_show_toaster(gpointer thsPlayer)
{
	LOG("show_toaster ");	
	SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
	gboolean bAct = TRUE;
    gboolean bExisted = (sd->note != NULL);
	GdkPixbuf *pixbuf = NULL;
	if( !notify_is_initted() )
		if( !notify_init("xfce4-squeezebox-plugin"))
			bAct = FALSE;
        
	if( bAct )
	{
		GString *albumArt = g_string_new(sd->player.albumArt->str);
		gint icon_size;
		GtkIconTheme *theme;
		
		theme = gtk_icon_theme_get_default ();
		gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &icon_size, NULL);
		//happily, we easily can escape ampersands and other usual suspects.
        gchar *ntTitle = g_markup_printf_escaped(sd->player.title->str);
		gchar *ntDetails = g_markup_printf_escaped(
                "by <b>%s</b>\nfrom <i>%s</i>",
                sd->player.artist->str,
                sd->player.album->str);
		
		if( albumArt->len ) {
			pixbuf = gdk_pixbuf_new_from_file_at_size(
				albumArt->str, icon_size, icon_size, NULL);
		}
		else {
			pixbuf = gtk_icon_theme_load_icon(theme,
						 "gnome-media-player",
						 icon_size,
						 0,
						 NULL);
		}
        //squeezebox_update_UI_hide_toaster(thsPlayer);
        if( ! bExisted ) {
			LOG("new");
            sd->note = notify_notification_new(	
                    ntTitle, 
                    ntDetails, NULL, NULL);
            g_signal_connect(G_OBJECT(sd->note), "closed", 
                G_CALLBACK(toaster_closed), sd);

        }
        else if( sd->note ) {
			LOG("update");
            notify_notification_update(sd->note, ntTitle, ntDetails, NULL);
		}
		if( sd->note ) {
			gint x = 0, y = 0;
            gint nTime;
			GtkRequisition size;
			XfceScreenPosition pos = 
				xfce_panel_plugin_get_screen_position(sd->plugin);
			
			gdk_window_get_origin(GTK_WIDGET(sd->plugin)->window, &x, &y);
			gtk_widget_size_request(GTK_WIDGET(sd->plugin), &size);
			x += size.width / 2;
			if(! xfce_screen_position_is_bottom(pos) )
				y += size.height;
			
			notify_notification_set_hint_int32 (sd->note, "x", x);
			notify_notification_set_hint_int32 (sd->note, "y", y);

            //timeout? never - only on our control
            notify_notification_set_timeout(sd->note, 0);
			
			// did we get an icon?
			if(pixbuf)
			{
				#if (LIBNOTIFY_VERSION_MAJOR == 0 && LIBNOTIFY_VERSION_MINOR <=3 && LIBNOTIFY_VERSION_MICRO < 2)
					notify_notification_set_icon_data_from_pixbuf (sd->note, pixbuf);
				#else
					notify_notification_set_icon_from_pixbuf (sd->note, pixbuf);
				#endif
				g_object_unref (pixbuf);
			}

			// liftoff					
            if( ! bExisted )
                notify_notification_show(sd->note, NULL);
            
            sd->timerCount = sd->notifytimeout;
		}
		g_free(ntTitle);
		g_free(ntDetails);
		g_string_free(albumArt, TRUE);
	}
	LOG("\n");
}
#endif

static void
squeezebox_update_repeat(gpointer thsPlayer, gboolean newRepeat)
{
    LOG("Enter squeezebox_update_repeat\n");
    SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
    sd->noUI = TRUE;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuRepeat), newRepeat);
    sd->noUI = FALSE;
    LOG("Leave squeezebox_update_repeat\n");
}

static void
squeezebox_update_shuffle(gpointer thsPlayer, gboolean newShuffle)
{
    LOG("Enter squeezebox_update_shuffle\n");
    SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
    sd->noUI = TRUE;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sd->mnuShuffle), newShuffle);
    sd->noUI = FALSE;
    LOG("Enter squeezebox_update_shuffle\n");
}

static void
squeezebox_update_time_position(gpointer thsPlayer)
{
    SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
    /*
    if( sd->note )
        squeezebox_update_UI_show_toaster(thsPlayer, FALSE);
    
    */
}

static void
squeezebox_update_UI(gpointer thsPlayer, gboolean updateSong, 
    eSynoptics State, const gchar* playerMessage)
{
	SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
	
	if( sd->state != State )
	{
		sd->state = State;
		squeezebox_update_playbtn(sd);		
	}
	
	if( updateSong )
	{
		if( sd->toolTipStyle == ettSimple )
		{
			g_string_printf(
				sd->toolTipText, "%s: %s - %s",
				sd->player.artist->str,
				sd->player.album->str,
				sd->player.title->str);
			
			if( sd->show[ebtnPrev] )
				gtk_tooltips_set_tip(
					sd->tooltips, sd->button[ebtnPrev], 
					sd->toolTipText->str, NULL);
			
			if( sd->show[ebtnPlay] )
				gtk_tooltips_set_tip(
					sd->tooltips, sd->button[ebtnPlay], 
					sd->toolTipText->str, NULL);
			
			if( sd->show[ebtnNext] )
				gtk_tooltips_set_tip(
					sd->tooltips, sd->button[ebtnNext], 
					sd->toolTipText->str, NULL);
			
		}
		
		#if HAVE_NOTIFY
		if( sd->notify )
		{
			squeezebox_update_UI_show_toaster(thsPlayer);
		}
		#endif
	}
}

static gboolean
squeezebox_set_size (XfcePanelPlugin *plugin, int size, SqueezeBoxData *sd)
{
	int items = 1;
    
	if( sd->show[ebtnPrev] )
	{
		gtk_widget_set_size_request (GTK_WIDGET (sd->button[ebtnPrev]), size, size);
		items++;
	}
	
	if( sd->show[ebtnNext] )
	{
		gtk_widget_set_size_request (GTK_WIDGET (sd->button[ebtnNext]), size, size);
		items++;
	}
	gtk_widget_set_size_request (GTK_WIDGET (sd->button[ebtnPlay]), size, size);
    gtk_widget_set_size_request (GTK_WIDGET (sd->table), 3*size, size);
    gtk_widget_set_size_request (GTK_WIDGET (plugin), items*size, size);

    return TRUE;
}

static void
squeezebox_free_data (XfcePanelPlugin * plugin, SqueezeBoxData * sd)
{
    LOG("Enter squeezebox_free_data\n");
    if(sd->tooltips)
    {
        g_object_unref(sd->tooltips);
        sd->tooltips = NULL;
    }
	if( sd->player.Detach )
		sd->player.Detach(sd->player.db);
    if( sd->timerHandle )
        g_source_remove(sd->timerHandle);
    g_free (sd);
	LOG("Leave squeezebox_free_data\n");
}

static void
squeezebox_style_set (XfcePanelPlugin *plugin, gpointer ignored,
                       SqueezeBoxData *sd)
{
    squeezebox_set_size(plugin, xfce_panel_plugin_get_size (plugin), sd);
}

static void
squeezebox_read_rc_file (XfcePanelPlugin *plugin, SqueezeBoxData *sd)
{
    char *file;
    XfceRc *rc = NULL;
	gint nBackend = 1;
	gboolean bShowNext = TRUE;
	gboolean bShowPrev = TRUE;
	gint toolTipStyle = 1;
#if HAVE_NOTIFY
	gboolean bNotify = TRUE;
	gdouble  dNotifyTimeout = 5.0;
#endif
    LOG("Enter squeezebox_read_rc_file\n");
	
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
			nBackend = xfce_rc_read_int_entry(rc, "squeezebox_backend", 2);

			bShowNext = xfce_rc_read_int_entry(rc, "show_next", 1);
			bShowPrev = xfce_rc_read_int_entry(rc, "show_prev", 1);
			#if HAVE_NOTIFY
			bNotify   = xfce_rc_read_int_entry(rc, "notify", 1);
			dNotifyTimeout = xfce_rc_read_int_entry(rc, "notify_timeout", 5);
			#endif
			sd->player.updateRateMS = xfce_rc_read_int_entry(rc, "updateRateMS", 500);
			toolTipStyle = xfce_rc_read_int_entry(rc, "tooltips", 1);
			if( toolTipStyle < 0 )
				toolTipStyle = 0;
        }
    }
	
	// Always init backend
    sd->player.sd = sd;
    squeezebox_init_backend(sd, nBackend);	
	sd->show[ebtnNext] = bShowNext;
	sd->show[ebtnPlay] = TRUE; // well, maybe not later
	sd->show[ebtnPrev] = bShowPrev;
#if HAVE_NOTIFY
	sd->notify = bNotify;
	sd->notifytimeout = dNotifyTimeout;
	if( toolTipStyle > ettFull )
		toolTipStyle = ettFull;
    if( toolTipStyle == ettFull ) {
        sd->timerHandle = g_timeout_add(1000, on_timer, sd);
		//printf("Attach %ld\n", sd->timerHandle);
	}
#else
	if( toolTipStyle > ettSimple )
		toolTipStyle = ettSimple;
    
    if( toolTipStyle < ettNone )
        toolTipStyle = ettNone;
#endif
	sd->toolTipStyle = toolTipStyle;
    if( sd->toolTipStyle == ettSimple )
        gtk_tooltips_enable(sd->tooltips);
    else
        gtk_tooltips_disable(sd->tooltips);

	if( rc != NULL )
	{
		if( sd->player.Persist )
			sd->player.Persist(sd->player.db, rc, FALSE);
		xfce_rc_close (rc);
	}
    
	LOG("Leave squeezebox_read_rc_file\n");
}

static void
squeezebox_write_rc_file (XfcePanelPlugin *plugin, SqueezeBoxData *sd)
{
    
	char *file;
    XfceRc *rc;
	
	LOG("Enter squeezebox_write_rc_file\n");
    
    if ((file = xfce_panel_plugin_save_location (plugin, TRUE)))
	{

		rc = xfce_rc_simple_open (file, FALSE);
		g_free (file);
	
		if (rc != NULL) 
		{
			LOG("Writing to file '");
			LOG(file);
			xfce_rc_write_int_entry(rc, "squeezebox_backend", sd->backend);
			xfce_rc_write_int_entry(rc, "show_next", (sd->show[ebtnNext])? 1 : 0);
			xfce_rc_write_int_entry(rc, "show_prev", (sd->show[ebtnPrev])? 1 : 0);
			xfce_rc_write_int_entry(rc, "tooltips", sd->toolTipStyle);
			#if HAVE_NOTIFY
			xfce_rc_write_int_entry(rc, "notify", (sd->notify)? 1 : 0);
			xfce_rc_write_int_entry(rc, "notify_timeout", sd->notifytimeout);
			#endif

			if( sd->player.Persist )
				sd->player.Persist(sd->player.db, rc, TRUE);
			
			xfce_rc_close (rc);
			LOG("OK\n");
		}
	}
	LOG("Leave squeezebox_write_rc_file\n");
}


static void
squeezebox_dialog_response (GtkWidget *dlg, int reponse, 
                         SqueezeBoxData *sd)
{
    g_object_set_data (G_OBJECT (sd->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (sd->plugin);
    squeezebox_write_rc_file (sd->plugin, sd);
	
	// have tooltips?
	switch( sd->toolTipStyle )
	{
		case ettSimple:
			if( !sd->tooltips )
			{
			}
			break;
		default:
			if( sd->tooltips )
			{
			}
			break;
	}
    
    
	
}

static void
config_show_backend_properties(GtkButton *btn, SqueezeBoxData *sd)
{
	if( sd->player.Configure )
		sd->player.Configure(sd->player.db, GTK_WIDGET(sd->plugin));
	else
	{
		GtkWidget *dlg = gtk_message_dialog_new(
			GTK_WINDOW(g_object_get_data(G_OBJECT(sd->plugin), "dialog")), 
			GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
			_("This backend has no configurable properties"));
		gtk_dialog_run (GTK_DIALOG (dlg));
		gtk_widget_destroy (dlg);
	}
}

static void
config_change_backend(GtkOptionMenu *cb, SqueezeBoxData *sd)
{
	int nBackend = gtk_option_menu_get_history(cb) + 1;
    squeezebox_init_backend(sd, nBackend);    
}

#if HAVE_NOTIFY
static void
config_change_notify_timeout(GtkSpinButton *sb, SqueezeBoxData *sd)
{
	sd->notifytimeout = gtk_spin_button_get_value(sb);
}

static void
config_toggle_notify (GtkToggleButton *tb, SqueezeBoxData *sd)
{
    sd->notify = gtk_toggle_button_get_active(tb);
    #if HAVE_NOTIFY
    if( sd->toolTipStyle == ettFull )
        sd->timerHandle = g_timeout_add(1000, on_timer, sd);
    else if( sd->timerHandle != 0 )
        g_source_remove(sd->timerHandle);
    #endif
    
}
#endif

static void
config_toggle_tooltips_none (GtkToggleButton *opt, SqueezeBoxData *sd)
{
    if( gtk_toggle_button_get_active(opt) ) {
        sd->toolTipStyle = ettNone;
        gtk_tooltips_disable(sd->tooltips);
    }
}

static void
config_toggle_tooltips_simple (GtkToggleButton *opt, SqueezeBoxData *sd)
{
    if( gtk_toggle_button_get_active(opt) ) {
        sd->toolTipStyle = ettSimple;
        gtk_tooltips_enable(sd->tooltips);
    }
}

static void
config_toggle_tooltips_full (GtkToggleButton *opt, SqueezeBoxData *sd)
{
    if( gtk_toggle_button_get_active(opt) ) {
        sd->toolTipStyle = ettFull;
        gtk_tooltips_disable(sd->tooltips);
    }
}

static void
config_toggle_next (GtkToggleButton *tb, SqueezeBoxData *sd)
{
	gboolean *pBtn = &sd->show[ebtnNext];
	gint size = xfce_panel_plugin_get_size(sd->plugin);
					
	
	*pBtn = gtk_toggle_button_get_active(tb);
	
	if( *pBtn )
		gtk_widget_show(sd->button[ebtnNext]);
	else
		gtk_widget_hide(sd->button[ebtnNext]);

	squeezebox_set_size(sd->plugin, size, sd);	
}

static void
config_toggle_prev (GtkToggleButton *tb, SqueezeBoxData *sd)
{
	gboolean *pBtn = &sd->show[ebtnPrev];
	gint size = xfce_panel_plugin_get_size(sd->plugin);
	
	*pBtn = gtk_toggle_button_get_active(tb);
	
	if( *pBtn )
		gtk_widget_show(sd->button[ebtnPrev]);
	else
		gtk_widget_hide(sd->button[ebtnPrev]);		

	squeezebox_set_size(sd->plugin, size, sd);
}


static void
squeezebox_properties_dialog (XfcePanelPlugin *plugin, SqueezeBoxData *sd)
{
    GtkWidget *dlg, *header, *vbox, *hbox1, *hbox2, *label1, *label2, 
	*cb1, *cb2, *cb3, *btnDet;
    //GtkAdjustment *adjustment;
    GtkWidget *squeezebox_delay_spinner;
	GtkWidget *cbBackend, *cbItem, *cbMenu;
	GtkWidget *opt[3];
    const Backend *ptr = squeezebox_get_backends();
    int i;
	//GList *glist = NULL;
	

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    
    g_signal_connect (dlg, "response", G_CALLBACK (squeezebox_dialog_response),
                      sd);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    header = xfce_create_header (NULL, _("Squeezebox media player remote"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, 200, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), 6);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0);
    
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox,
                        TRUE, TRUE, 0);
						
	hbox1 = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, FALSE, 0);

    cb1 = gtk_check_button_new_with_mnemonic (_("Show _previous button"));
    gtk_widget_show (cb1);
    gtk_box_pack_start (GTK_BOX (hbox1), cb1, FALSE, FALSE, 0);
	
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1),
                                  sd->show[ebtnPrev]);
    g_signal_connect (cb1, "toggled", G_CALLBACK (config_toggle_prev),
                      sd);
	
    cb2 = gtk_check_button_new_with_mnemonic (_("Show _next button"));
    gtk_widget_show (cb2);
    gtk_box_pack_start (GTK_BOX (hbox1), cb2, FALSE, FALSE, 0);
	
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb2),
                                  sd->show[ebtnNext]);
    g_signal_connect (cb2, "toggled", G_CALLBACK (config_toggle_next),
                      sd);

	hbox1 = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, FALSE, 0);
	opt[0] = gtk_radio_button_new_with_mnemonic(NULL, _("No _tooltips"));
	gtk_widget_show(opt[0]);
	gtk_box_pack_start(GTK_BOX (hbox1), opt[0], FALSE, FALSE, 0);
	g_signal_connect (opt[0], "toggled",
                     G_CALLBACK(config_toggle_tooltips_none), sd);	

	opt[1] = gtk_radio_button_new_with_mnemonic_from_widget(
		GTK_RADIO_BUTTON(opt[0]), _("_Simple tooltips"));
	gtk_widget_show(opt[1]);
	gtk_box_pack_start(GTK_BOX (hbox1), opt[1], FALSE, FALSE, 0);
	g_signal_connect (opt[1], "toggled",
                     G_CALLBACK(config_toggle_tooltips_simple), sd);	

	
#if HAVE_NOTIFY    
	opt[2] = gtk_radio_button_new_with_mnemonic_from_widget(
		GTK_RADIO_BUTTON(opt[0]), _("N_otification tooltips"));
	gtk_widget_show(opt[2]);
	g_signal_connect (opt[2], "toggled",
                     G_CALLBACK(config_toggle_tooltips_full), sd);	
	
	// 0, 1 or 2
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(opt[sd->toolTipStyle]), TRUE);
	
	gtk_box_pack_start(GTK_BOX (hbox1), opt[2], FALSE, FALSE, 0);

	cb3 = gtk_check_button_new_with_mnemonic(
		_("Show noti_fications (notify daemon)"));
    gtk_widget_show (cb3);
    gtk_box_pack_start (GTK_BOX (vbox), cb3, FALSE, FALSE, 0);
	
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb3),
                                  sd->notify);
    g_signal_connect (cb3, "toggled", G_CALLBACK (config_toggle_notify),
                      sd);

    hbox2 = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox2);
    gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
    
    label2 = gtk_label_new_with_mnemonic(_("Notification _timeout (sec)"));
    gtk_widget_show(label2);
    gtk_box_pack_start (GTK_BOX (hbox2), label2, FALSE, FALSE, 0);

    squeezebox_delay_spinner = gtk_spin_button_new_with_range(0.0, 60.0, 1.0);
    gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(squeezebox_delay_spinner),sd->notifytimeout);
    gtk_widget_show(squeezebox_delay_spinner);
    gtk_box_pack_start (GTK_BOX (hbox2), squeezebox_delay_spinner, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label2), squeezebox_delay_spinner);

    
    g_signal_connect(squeezebox_delay_spinner, "value-changed",
                        G_CALLBACK(config_change_notify_timeout), sd);
#else
	// force 0 or 1
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(opt[(sd->toolTipStyle)? 1 : 0]), TRUE);
#endif
	hbox1 = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, FALSE, 0);
    
    label1 = gtk_label_new_with_mnemonic(_("_Backend:"));
    gtk_widget_show(label1);
    gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);

    cbBackend = gtk_option_menu_new();
	gtk_widget_show(cbBackend);
	cbMenu = gtk_menu_new();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (cbBackend), cbMenu);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), cbBackend);

    for(i=0;;i++)
    {
        cbItem = gtk_menu_item_new_with_label(ptr->BACKEND_name());    
        gtk_widget_show (cbItem);
        gtk_menu_shell_append (GTK_MENU_SHELL(cbMenu), cbItem);
        ptr++;
        if( !ptr->BACKEND_name )
            break;
    }
    
    /*
    
    cbItem = gtk_menu_item_new_with_label (_("Rhythmbox (via DBUS interface)"));
    gtk_widget_show (cbItem);
    gtk_menu_shell_append (GTK_MENU_SHELL(cbMenu), cbItem);
	
    cbItem = gtk_menu_item_new_with_label (_("Music Player Daemon (libmpd)"));
    gtk_widget_show (cbItem);
    gtk_menu_shell_append (GTK_MENU_SHELL(cbMenu), cbItem);
    
    */
	
	gtk_option_menu_set_history (GTK_OPTION_MENU (cbBackend),
                                 sd->backend-1);
	
    g_signal_connect(cbBackend, "changed",
                        G_CALLBACK(config_change_backend), sd);
	
    gtk_box_pack_start (GTK_BOX (hbox1), cbBackend, FALSE, FALSE, 0);
    
    btnDet = gtk_button_new_with_mnemonic(_("_Settings..."));
    gtk_widget_show(btnDet);
    gtk_box_pack_start (GTK_BOX (hbox1), btnDet, FALSE, FALSE, 0);
    g_signal_connect(btnDet, "clicked",
                        G_CALLBACK(config_show_backend_properties), sd);
	

	gtk_widget_show (dlg);
}

void on_btnPrev_clicked(GtkButton *button, SqueezeBoxData *sd)
{
	if( sd->player.Previous )
		sd->player.Previous(sd->player.db);
}

void on_btnPlay_clicked(GtkButton  *button, SqueezeBoxData *sd)
{
	gboolean bRet = FALSE;
	if( sd->player.Toggle && sd->player.Toggle(sd->player.db, &bRet) )
	{
		squeezebox_update_playbtn(sd);
	}		
}

void on_btnNext_clicked(GtkButton *button, SqueezeBoxData *sd)
{
	LOG("Enter on_btnNext_clicked\n");
	if( sd->player.Next )
		sd->player.Next(sd->player.db);
	LOG("Leave on_btnNext_clicked\n");
}

static gboolean 
on_btn_any_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer thsPlayer)
{
	SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
    if( sd->toolTipStyle == ettFull ) {
        sd->inEnter = TRUE;
        if( NULL == sd->note )
            squeezebox_update_UI_show_toaster(thsPlayer);
    }
    return FALSE;
}

static gboolean 
on_btn_any_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer thsPlayer)
{
	SqueezeBoxData *sd = (SqueezeBoxData *)thsPlayer;
    if( sd->toolTipStyle == ettFull ) {
        sd->inEnter = FALSE;
    }
    return FALSE;
}


void on_mnuPlayerToggled(GtkCheckMenuItem *checkmenuitem, SqueezeBoxData *sd)
{
    if( sd->noUI == FALSE && sd->player.Show )
        sd->player.Show(sd->player.db, checkmenuitem->active);        
}

void on_mnuShuffleToggled(GtkCheckMenuItem *checkmenuitem, SqueezeBoxData *sd)
{
    if( sd->noUI == FALSE && sd->player.SetShuffle )
        sd->player.SetShuffle(sd->player.db, checkmenuitem->active);
}

void on_mnuRepeatToggled(GtkCheckMenuItem *checkmenuitem, SqueezeBoxData *sd)
{
    if( sd->noUI == FALSE && sd->player.SetRepeat )
        sd->player.SetRepeat(sd->player.db, checkmenuitem->active);
}

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

static GtkContainer*
squeezebox_create (SqueezeBoxData *sd)
{
	LOG("Enter squeezebox_create\n");
  
    sd->tooltips = gtk_tooltips_new();
    g_object_ref(sd->tooltips);
    gtk_object_sink(GTK_OBJECT(sd->tooltips));
    
	GtkContainer *window1 = GTK_CONTAINER(sd->plugin);
	
	sd->table = gtk_table_new(1, 3, FALSE);
	gtk_widget_show (sd->table);
	gtk_container_add (GTK_CONTAINER (window1), sd->table);
	
	sd->button[ebtnPrev] = gtk_button_new ();
    gtk_button_set_relief(GTK_BUTTON(sd->button[ebtnPrev]), GTK_RELIEF_NONE);
	gtk_widget_show (sd->button[ebtnPrev]);
	gtk_table_attach (GTK_TABLE (sd->table), sd->button[ebtnPrev], 0, 1, 0, 1,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (sd->button[ebtnPrev]), FALSE);
	gtk_tooltips_set_tip(sd->tooltips, sd->button[ebtnPrev], "previous", NULL);
	
	sd->image[ebtnPrev] = gtk_image_new_from_stock ("gtk-media-rewind", GTK_ICON_SIZE_MENU);
	gtk_widget_show (sd->image[ebtnPrev]);
	gtk_container_add (GTK_CONTAINER (sd->button[ebtnPrev]), sd->image[ebtnPrev]);
	
	sd->button[ebtnPlay] = gtk_button_new ();
    gtk_button_set_relief(GTK_BUTTON(sd->button[ebtnPlay]), GTK_RELIEF_NONE);
	gtk_widget_show (sd->button[ebtnPlay]);
	gtk_table_attach (GTK_TABLE (sd->table), sd->button[ebtnPlay], 1, 2, 0, 1,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (sd->button[ebtnPlay]), FALSE);
	
	sd->image[ebtnPlay] = gtk_image_new_from_stock ("gtk-media-play", GTK_ICON_SIZE_MENU);
	gtk_widget_show (sd->image[ebtnPlay]);
	gtk_container_add (GTK_CONTAINER (sd->button[ebtnPlay]), sd->image[ebtnPlay]);
	
	sd->button[ebtnNext] = gtk_button_new ();
    gtk_button_set_relief(GTK_BUTTON(sd->button[ebtnNext]), GTK_RELIEF_NONE);
	gtk_widget_show (sd->button[ebtnNext]);
	gtk_table_attach (GTK_TABLE (sd->table), sd->button[ebtnNext], 2, 3, 0, 1,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (sd->button[ebtnNext]), FALSE);
	
	sd->image[ebtnNext] = gtk_image_new_from_stock ("gtk-media-forward", GTK_ICON_SIZE_MENU);
	gtk_widget_show (sd->image[ebtnNext]);
	gtk_container_add (GTK_CONTAINER (sd->button[ebtnNext]), sd->image[ebtnNext]);
	
	// connect signals
	g_signal_connect ((gpointer) sd->button[ebtnPrev], "clicked",
					G_CALLBACK(on_btnPrev_clicked), sd);
    g_signal_connect ((gpointer) sd->button[ebtnPlay], "clicked",
					G_CALLBACK (on_btnPlay_clicked), sd);
    g_signal_connect ((gpointer) sd->button[ebtnNext], "clicked",
					G_CALLBACK (on_btnNext_clicked), sd);

    // toaster handling
    #if HAVE_NOTIFY
    g_signal_connect( sd->button[ebtnPrev], "enter-notify-event",
                    G_CALLBACK(on_btn_any_enter), sd);
    g_signal_connect( sd->button[ebtnPrev], "leave-notify-event",
                    G_CALLBACK(on_btn_any_leave), sd);
	
    g_signal_connect( sd->button[ebtnPlay], "enter-notify-event",
                    G_CALLBACK(on_btn_any_enter), sd);
    g_signal_connect( sd->button[ebtnPlay], "leave-notify-event",
                    G_CALLBACK(on_btn_any_leave), sd);
	
    g_signal_connect( sd->button[ebtnNext], "enter-notify-event",
                    G_CALLBACK(on_btn_any_enter), sd);
    g_signal_connect( sd->button[ebtnNext], "leave-notify-event",
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
	
    if( sd->state != ebtnPlay )
        squeezebox_update_playbtn(sd);
	
    
    
	LOG("Leave squeezebox_create\n");
	return window1;
}

static void
squeezebox_construct (XfcePanelPlugin * plugin)
{
	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	LOG("Enter squeezebox_construct\n");
	
	SqueezeBoxData *sd = g_new0 (SqueezeBoxData, 1);
   
    sd->plugin = plugin;
	sd->player.artist = g_string_new(_("(unknown)"));
	sd->player.album = g_string_new(_("(unknown)"));
	sd->player.title = g_string_new(_("(unknown)"));
	sd->player.albumArt = g_string_new("");
    sd->toolTipText = g_string_new("");
    sd->player.Update = squeezebox_update_UI;
    sd->player.UpdateShuffle = squeezebox_update_shuffle;
    sd->player.UpdateRepeat = squeezebox_update_repeat;
    sd->player.UpdateTimePosition = squeezebox_update_time_position;
    sd->inEnter = FALSE;
#if HAVE_NOTIFY
	sd->notifytimeout = 5;
    sd->timerHandle = 0;
#endif
    
	
	squeezebox_create(sd);

    xfce_panel_plugin_add_action_widget (plugin, sd->button[ebtnPrev]);
    xfce_panel_plugin_add_action_widget (plugin, sd->button[ebtnPlay]);
    xfce_panel_plugin_add_action_widget (plugin, sd->button[ebtnNext]);
	xfce_panel_plugin_add_action_widget (plugin, sd->table);
    
    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (squeezebox_free_data), sd);

    g_signal_connect (plugin, "size-changed",
                      G_CALLBACK (squeezebox_set_size), sd);

    sd->style_id = 
        g_signal_connect(plugin, "style-set",
                          G_CALLBACK (squeezebox_style_set), sd);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (squeezebox_properties_dialog), sd);
					  
	// add menu items
	sd->mnuPlayer = gtk_check_menu_item_new_with_label(_("Show player"));
	gtk_widget_show(sd->mnuPlayer);
	xfce_panel_plugin_menu_insert_item(sd->plugin, GTK_MENU_ITEM(sd->mnuPlayer));
    g_signal_connect (G_OBJECT(sd->mnuPlayer), "toggled", 
                      G_CALLBACK (on_mnuPlayerToggled), sd);
    g_object_ref(sd->mnuPlayer);
		
	sd->mnuShuffle = gtk_check_menu_item_new_with_label(_("Shuffle"));
	gtk_widget_show(sd->mnuShuffle);
	xfce_panel_plugin_menu_insert_item(sd->plugin, GTK_MENU_ITEM(sd->mnuShuffle));
    g_signal_connect (G_OBJECT(sd->mnuShuffle), "toggled", 
                      G_CALLBACK (on_mnuShuffleToggled), sd);
    g_object_ref(sd->mnuShuffle);
		
	sd->mnuRepeat = gtk_check_menu_item_new_with_label(_("Repeat"));
	gtk_widget_show(sd->mnuRepeat);
	xfce_panel_plugin_menu_insert_item(sd->plugin, GTK_MENU_ITEM(sd->mnuRepeat));
    g_signal_connect (G_OBJECT(sd->mnuRepeat), "toggled", 
                      G_CALLBACK (on_mnuRepeatToggled), sd);
    g_object_ref(sd->mnuRepeat);
    
    squeezebox_read_rc_file (plugin, sd);
    
    // the above will init & create the actual player backend
    // and also init menu states
					  
	LOG("Leave squeezebox_construct\n");
		
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (squeezebox_construct);
