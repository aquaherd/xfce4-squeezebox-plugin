/***************************************************************************
 *            rythmbox-mpd.c
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

#include <string.h>

// libmpd for music player daemon remote
#include <libmpd/libmpd.h>

#include "squeezebox.h"

DEFINE_BACKEND(MPD, _("Music Player Daemon (libmpd)"))

#define MPD_MAP(a) player->a = mpd##a;

typedef struct {
	SPlayer		*parent;
	gint 		port;
	GString 	*host;
	GString 	*pass;
	MpdObj 		*player;
	gboolean 	bUseDefault;
	gboolean	bAutoConnect;
	gboolean	bRequireReconnect;
	gint		intervalID;
	gint 		songID;
    gint        songLength;
	GtkWidget   *wHost, *wPass, *wPort, *wDlg;
}mpdData;

#define MKTHIS mpdData *this = (mpdData *)thsPtr;
void *MPD_attach(SPlayer *player);
void mpdCallbackStateChanged(MpdObj *player, ChangedStatusType sType, gpointer thsPtr);

gboolean mpdAssure(gpointer thsPtr)
{
	MKTHIS;
    gboolean gConnect = FALSE;
	LOG("Enter mpdAssure\n");
	if( !this->player )
	{
		if( this->bUseDefault ) 
		{
			LOG("    Connecting local...");
			this->player = mpd_new_default();
		}
		else
		{
			LOG("    Connecting remote...");
			this->player = mpd_new(
				this->host->str, 
				this->port, 
				this->pass->str);
		}
		if( this->bAutoConnect && this->player )
		{
			if( MPD_OK == mpd_connect(this->player) )
			{
				LOG("Connect OK\n");
			}
		}
		gConnect = TRUE;
	}
	if(
		this->player != NULL && 
		mpd_status_update(this->player) != MPD_OK)
	{
		LOG("reconnecting..");
		mpd_connect(this->player);
		//if(!this->bUseDefault)
			//mpd_send_password(this->player);
		if (mpd_check_error(this->player) || mpd_status_update(this->player) != MPD_OK)
		{
			LOG(".Fail.\n");
			g_string_assign(this->parent->artist, "");
			g_string_assign(this->parent->album, "");
			g_string_assign(this->parent->title, "");
			g_string_assign(this->parent->albumArt, "");
			this->parent->Update(this->parent->sd, TRUE, estErr, _("Can't connect to music player daemon."));
			mpd_disconnect(this->player);
			mpd_free(this->player);
			this->player = NULL;
		}
		else
		{
			LOG(".OK.\n");
		}
	}
	
	if( gConnect && this->player )
	{
		mpd_signal_connect_status_changed(
			this->player, mpdCallbackStateChanged, this);
		mpdCallbackStateChanged(
		    this->player, 
		    MPD_CST_SONGID | MPD_CST_STATE | MPD_CST_RANDOM | MPD_CST_REPEAT, 
		    this);
	}
	
	LOG("Leave mpdAssure\n");
	return (this->player != NULL);
}

gint mpdCallback(gpointer thsPtr)
{
	MKTHIS;
	if( this->player != NULL )
		mpd_status_update(this->player);
	return TRUE;	
}

gboolean mpdNext(gpointer thsPtr)
{
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter mpdNext\n");
	if( !mpdAssure(this) )
		return FALSE;
	else
		bRet = (MPD_OK == mpd_player_next(this->player));
	LOG("Leave mpdNext\n");
	return bRet;
}

gboolean mpdPrevious(gpointer thsPtr)
{
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter mpdPrevious\n");
	if( !mpdAssure(this) )
		return FALSE;
	else
		bRet = (MPD_OK == mpd_player_prev(this->player));
	LOG("Leave mpdPrevious\n");
	return TRUE;
}

gboolean mpdPlayPause(gpointer thsPtr, gboolean newState)
{
	MKTHIS;
	LOG("Enter mpdPlayPause\n");
	int iRet = MPD_OK;
	if( !mpdAssure(this) )
		return FALSE;
	else if(newState)
		iRet = mpd_player_play(this->player);
	else
		iRet = mpd_player_pause(this->player);
	LOG("Leave mpdPlayPause\n");
	return (iRet == MPD_OK);
}

gboolean mpdIsPlaying(gpointer thsPtr)
{
	MKTHIS;
	MpdState sStat = MPD_STATUS_STATE_STOP;
	
	LOG("Enter mpdIsPlaying\n");
	if( !mpdAssure(this) )
		return FALSE;
	if( this->player )
	{
		sStat = mpd_player_get_state(this->player);
	}
	LOG("Leave mpdIsPlaying\n");
	return (sStat == MPD_STATUS_STATE_PLAY);
}

gboolean mpdToggle(gpointer thsPtr, gboolean *newState)
{
	MKTHIS;
	gboolean oldState = FALSE;
	LOG("Enter mpdToggle\n");
	if( !mpdAssure(this) )
		return FALSE;
	oldState = mpdIsPlaying(this);
	if( !mpdPlayPause(this, !oldState) )
		return FALSE;
	if( newState )
		*newState = !oldState;
	LOG("Leave mpdToggle\n");
	return TRUE;
}

gboolean mpdDetach(gpointer thsPtr)
{
	MKTHIS;
	LOG("Enter mpdDetach\n");
	if( this->player )
	{
		mpd_disconnect(this->player);
		mpd_free(this->player);
		g_free(this);
	}
	if( this->intervalID )
	{
		g_source_remove(this->intervalID);
		this->intervalID = 0;
	}
	LOG("Leave mpdDetach\n");
	return TRUE;
}

void mpdPersist(gpointer thsPtr, XfceRc *rc, gboolean bIsStoring)
{
	MKTHIS;
	LOG("Enter mpdPersist\n");
	if( bIsStoring )
	{
		xfce_rc_write_int_entry(rc, "mpd_usedefault", 
			(this->bUseDefault)?TRUE:FALSE);
		xfce_rc_write_int_entry(rc, "mpd_port", this->port);
		xfce_rc_write_entry(rc, "mpd_host", this->host->str);
		xfce_rc_write_entry(rc, "mpd_pass", this->pass->str);
	}
	else
	{
		this->bUseDefault = 
			(xfce_rc_read_int_entry(rc, "mpd_usedefault", 1)?TRUE:FALSE);
		this->port = xfce_rc_read_int_entry(rc, "mpd_port", 6600);
		g_string_assign(
			this->host,
			xfce_rc_read_entry(rc, "mpd_host", "localhost")
		);
		g_string_assign(
			this->pass,
			xfce_rc_read_entry(rc, "mpd_pass", "")
		);
	}
	LOG("Leave mpdPersist\n");
}

gboolean mpdGetRepeat(gpointer thsPtr, gboolean *oldRepeat)
{
    MKTHIS;
    if( mpdAssure(thsPtr) )
    {
        *oldRepeat = mpd_player_get_repeat(this->player);
        return TRUE;
    }
    return FALSE;
}

gboolean mpdSetRepeat(gpointer thsPtr, gboolean newShuffle)
{
    MKTHIS;
    if( mpdAssure(thsPtr))
    {
        return (MPD_OK == mpd_player_set_repeat(this->player, (newShuffle)?1:0));
    }
    return FALSE;
}

gboolean mpdGetShuffle(gpointer thsPtr, gboolean *oldShuffle)
{
    MKTHIS;
    if( mpdAssure(thsPtr) )
    {
        *oldShuffle = mpd_player_get_random(this->player);
        return TRUE;
    }
    return FALSE;
}

gboolean mpdSetShuffle(gpointer thsPtr, gboolean newRandom)
{
    MKTHIS;
    if( mpdAssure(thsPtr))
    {
        return (MPD_OK == mpd_player_set_random(this->player, (newRandom)?1:0));
    }
    return FALSE;
}



void mpdCallbackStateChanged(MpdObj *player, ChangedStatusType sType, gpointer thsPtr)
{
	MKTHIS;
	if( sType & MPD_CST_SONGID )
	{
		LOG("Enter mpdCallback: SongChanged\n");
		mpd_Song *song = mpd_playlist_get_current_song(this->player);
		if( mpdAssure(thsPtr) && song != NULL && song->id != this->songID )
		{
			GString *artLocation = g_string_new("");
			
			g_string_assign(this->parent->artist, song->artist);
			g_string_assign(this->parent->album, song->album);
			g_string_assign(this->parent->title, song->title);
			
			gchar *strTmp = g_markup_printf_escaped("%s/.covers/%s-%s.jpg", 
				g_get_home_dir(),
				song->artist, song->album);
			g_string_assign(artLocation, strTmp);
			g_free(strTmp);
			LOG(artLocation->str);LOG("\n");
			g_string_truncate(this->parent->albumArt, 0);
			
			if( g_file_test(artLocation->str, G_FILE_TEST_EXISTS) )
			{
				// just assign here, scaling is done in callee
				g_string_assign(this->parent->albumArt, artLocation->str);
			}
			g_string_free(artLocation, TRUE);
			this->parent->Update(this->parent->sd, TRUE, estPlay, NULL);
			this->songID = song->id;		
		}
		LOG("Leave mpdCallback: SongChanged\n");
	}
	if( sType & MPD_CST_STATE )
	{
		LOG("Enter mpdCallback: StateChanged\n");
		eSynoptics eStat;
		MpdState state = mpd_player_get_state(this->player);
		switch(state)
		{
			case MPD_PLAYER_PAUSE: eStat = estPause; break;
			case MPD_PLAYER_PLAY: eStat = estPlay; break;
			case MPD_PLAYER_STOP: eStat = estStop; break;
			default: eStat = estErr; break;
		}
		this->parent->Update(this->parent->sd, FALSE, eStat, NULL);
		LOG("Leave mpdCallback: StateChanged\n");
	}
    if( sType & MPD_CST_REPEAT )
    {
		LOG("Enter mpdCallback: RepeatChanged\n");
        gboolean bNew = FALSE;
        this->parent->UpdateRepeat(
            this->parent->sd, 
            mpd_player_get_repeat(this->player));
		LOG("Leave mpdCallback: RepeatChanged\n");
    }
    if( sType & MPD_CST_RANDOM )
    {
		LOG("Enter mpdCallback: ShuffleChanged\n");
        gboolean bNew = FALSE;
        this->parent->UpdateShuffle(
            this->parent->sd, 
            mpd_player_get_random(this->player));
		LOG("Leave mpdCallback: ShuffleChanged\n");
    }
    if( sType & (MPD_CST_ELAPSED_TIME | MPD_CST_TOTAL_TIME) )
    {
        if( sType & MPD_CST_ELAPSED_TIME )
            this->parent->secPos = 
                mpd_status_get_elapsed_song_time(this->player);
        if( sType & MPD_CST_TOTAL_TIME )
            this->parent->secTot = 
                mpd_status_get_total_song_time(this->player);
        this->parent->UpdateTimePosition(this->parent->sd);
    }
    
    //MPD_CST_TOTAL_TIME
}

static void
mpdSettingsDialogResponse(GtkWidget *dlg, int reponse, 
                         gpointer thsPtr)
{
    LOG("Enter mpdSettingsDialogResponse\n");
	MKTHIS;
	const gchar *tmpHost = gtk_entry_get_text(GTK_ENTRY(this->wHost));
	
	const gchar *tmpPass = gtk_entry_get_text(GTK_ENTRY(this->wPass));
	
	if( g_strcasecmp(tmpHost, this->host->str) )
	{
		g_string_assign(this->host, tmpHost);
		this->bRequireReconnect = TRUE;
	}
	
	if( g_strcasecmp(tmpPass, this->pass->str) )
	{
		g_string_assign(this->pass, tmpPass);
		this->bRequireReconnect = TRUE;
	}
	// reconnect if changed
	if( 0 ) // this->bRequireReconnect )
	{
		LOGF("    Reconnecting to %s/%s\n", tmpHost, this->host->str);
		SPlayer *p = this->parent;
		this->bRequireReconnect = FALSE;
		mpdDetach(thsPtr);
		MPD_attach(p);
		LOG("    Done reconnect.\n");
	}
	
    gtk_widget_destroy (dlg);
	
	LOG("Leave mpdSettingsDialogResponse\n");
}

static void
mpdConfigureTimeout(GtkSpinButton *sb, gpointer thsPtr)
{
	MKTHIS;
	this->port = gtk_spin_button_get_value_as_int(sb);	
	this->bRequireReconnect = TRUE;
}

static void
mpdConfigureUseDefault(GtkToggleButton *tb, gpointer thsPtr)
{
	MKTHIS;
	this->bUseDefault = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(this->wHost, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPort, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPass, !this->bUseDefault);
	this->bRequireReconnect = TRUE;
}

static void
mpdConfigure(gpointer thsPtr, GtkWidget *parent)
{
	MKTHIS;
    GtkWidget *dlg, *header, *vbox, *cb1,
		*label1, *label2, *label3;
	GtkTable *table;
	
	this->bRequireReconnect = FALSE;

    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW(parent),
                GTK_DIALOG_MODAL |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    this->wDlg = dlg;

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
    
    g_signal_connect (dlg, "response", G_CALLBACK (mpdSettingsDialogResponse),
                      thsPtr);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    header = xfce_create_header (NULL, _("Music Player Daemon"));
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
						
	cb1 = gtk_check_button_new_with_mnemonic(_("Use _defaults"));
	gtk_widget_show(cb1);
    gtk_box_pack_start (GTK_BOX (vbox), cb1, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (cb1),
                                  this->bUseDefault);
    g_signal_connect (cb1, "toggled", G_CALLBACK (mpdConfigureUseDefault),
                      thsPtr);
						
	table = GTK_TABLE(gtk_table_new(3, 2, FALSE));
    gtk_container_set_border_width(GTK_CONTAINER (table), 6);
	gtk_table_set_row_spacings(table, 6);
	gtk_table_set_col_spacings(table, 6);
    gtk_widget_show(GTK_WIDGET(table));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(table), FALSE, FALSE, 0);
	
	label1 = gtk_label_new_with_mnemonic(_("_Host:"));
    gtk_widget_show(label1);
    gtk_table_attach_defaults(table, label1, 0, 1, 0, 1);
	
	this->wHost = gtk_entry_new();
    gtk_widget_show(this->wHost);
    gtk_table_attach_defaults(table, this->wHost, 1, 2, 0, 1);
	gtk_entry_set_text(GTK_ENTRY(this->wHost), this->host->str);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), this->wHost);
    
	label2 = gtk_label_new_with_mnemonic(_("_Port:"));
    gtk_widget_show(label2);
    gtk_table_attach_defaults(table, label2, 0, 1, 1, 2);
	
	this->wPort = gtk_spin_button_new_with_range(1.0, 65536.0, 1.0);
    gtk_widget_show(this->wPort);
    gtk_table_attach_defaults(table, this->wPort, 1, 2, 1, 2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(this->wPort), this->port);
    g_signal_connect(this->wPort, "value-changed",
                        G_CALLBACK(mpdConfigureTimeout), thsPtr);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label2), this->wPort);
	
    
	label3 = gtk_label_new_with_mnemonic(_("Pass_word:"));
    gtk_widget_show(label3);
    gtk_table_attach_defaults(table, label3, 0, 1, 2, 3);
	
	this->wPass = gtk_entry_new();
    gtk_widget_show(this->wPass);
    gtk_table_attach_defaults(table, this->wPass, 1, 2, 2, 3);
	gtk_entry_set_visibility(GTK_ENTRY(this->wPass), FALSE);
	gtk_entry_set_text(GTK_ENTRY(this->wPass), this->pass->str);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label3), this->wPass);
	
	
	gtk_widget_set_sensitive(this->wHost, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPort, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPass, !this->bUseDefault);
    
	gtk_widget_show (dlg);
}


void *MPD_attach(SPlayer *player)
{
	mpdData *this = g_new0(mpdData, 1);
	LOG("Enter MPD_attach\n");
	
	if( player->Detach )
		player->Detach(player->db);
	MPD_MAP(Assure);
	MPD_MAP(Next);
	MPD_MAP(Previous);
	MPD_MAP(PlayPause);
	MPD_MAP(IsPlaying);
	MPD_MAP(Toggle);
	MPD_MAP(Detach);
	MPD_MAP(Persist);
	MPD_MAP(Configure);
    //MPD_MAP(IsVisible);
    //MPD_MAP(Show);
    MPD_MAP(GetRepeat);
    MPD_MAP(SetRepeat);
    MPD_MAP(GetShuffle);
    MPD_MAP(SetShuffle);
	
	// we init default values 
	this->bAutoConnect = TRUE;
	this->bUseDefault = TRUE;
	this->port = 6600;
	this->host = g_string_new("localhost");
	this->pass = g_string_new("");
	this->intervalID = 0;
	this->parent = player;
	
	// force our own update rate
	if( player->updateRateMS < 500 )
		player->updateRateMS = 500;
	
	// establish the callback function
	this->intervalID = 
		g_timeout_add(player->updateRateMS, mpdCallback, this);
		
	/* check secondary callbacks
	void mpd_ob_signal_set_song_changed(MpdObj *mi, 
		void *(* song_changed)(MpdObj *mi, int old_song_id, 
            int new_song_id, void *pointer), 
		void *pointer);	
	*/
	
	LOG("Leave MPD_attach\n");
	return this;
}