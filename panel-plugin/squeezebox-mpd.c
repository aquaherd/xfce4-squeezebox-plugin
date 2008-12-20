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
#ifdef HAVE_BACKEND_MPD

// default
#include "squeezebox.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if HAVE_GIO
#include <gio/gdesktopappinfo.h>
#endif

#include <string.h>

// libmpd for music player daemon remote
#include <libmpd/libmpd.h>

// pixmap
#include "squeezebox-mpd.png.h"

DEFINE_BACKEND(MPD, _("Music Player Daemon (libmpd)"))
#define MPD_MAP(a) parent->a = mpd##a
#define MPD_SQ_ALL (MPD_CST_SONGID|MPD_CST_STATE|MPD_CST_REPEAT|MPD_CST_RANDOM)
typedef struct {
	SPlayer *parent;
	gint port;
	GString *host;
	GString *pass;
	GString *path;
	GString *pmanager;
	MpdObj *player;
	gboolean bUseDefault;
	gboolean bUseMPDFolder;
	gboolean bUsePManager;
	gboolean bAutoConnect;
	gboolean bRequireReconnect;
	gint intervalID;
	gint songID;
	gint songLength;
	GtkWidget *wHost, *wPass, *wPort, *wDlg, *wPath, *wPMgr;
} mpdData;

// MFCish property map
BEGIN_PROP_MAP(MPD)
    PROP_ENTRY("mpd_usedefault", G_TYPE_BOOLEAN, "1")
    PROP_ENTRY("mpd_port", G_TYPE_INT, "6600")
    PROP_ENTRY("mpd_host", G_TYPE_STRING, "localhost")
    PROP_ENTRY("mpd_pass", G_TYPE_STRING, "")
    PROP_ENTRY("mpd_usemusicfolder", G_TYPE_BOOLEAN, "0")
    PROP_ENTRY("mpd_musicfolder", G_TYPE_STRING, "")
	PROP_ENTRY("mpd_usepmanager", G_TYPE_BOOLEAN, "0")
	PROP_ENTRY("mpd_pmanager", G_TYPE_STRING, "")
END_PROP_MAP()
#define MKTHIS mpdData *this = (mpdData *)thsPtr;
void *MPD_attach(SPlayer * player);
void mpdCallbackStateChanged(MpdObj * player, ChangedStatusType sType,
			     gpointer thsPtr);

gboolean mpdAssure(gpointer thsPtr, gboolean noCreate) {

	MKTHIS;
	gboolean gConnect = FALSE;
	LOG("Enter mpdAssure");
	if (!this->player) {
		if (this->bUseDefault) {
			LOG("    Connecting local...");
			this->player = mpd_new_default();
		} else {
			LOG("    Connecting remote...");
			this->player = mpd_new(this->host->str,
					       this->port, this->pass->str);
		}
		if (this->bAutoConnect && this->player) {
			if (MPD_OK == mpd_connect(this->player)) {
				LOG("Connect OK");
			}
		}
		gConnect = TRUE;
	}
	if (this->player != NULL && mpd_status_update(this->player) != MPD_OK) {
		LOG("reconnecting..");
		mpd_connect(this->player);
		//if(!this->bUseDefault)
		//mpd_send_password(this->player);
		if (mpd_check_error(this->player)
		    || mpd_status_update(this->player) != MPD_OK) {
			LOG(".Fail.");
			g_string_assign(this->parent->artist, "");
			g_string_assign(this->parent->album, "");
			g_string_assign(this->parent->title, "");
			g_string_assign(this->parent->albumArt, "");
			this->parent->Update(this->parent->sd, TRUE, estErr,
					     _("Can't connect to music player daemon."));
			mpd_disconnect(this->player);
			mpd_free(this->player);
			this->player = NULL;
		} else {
			LOG(".OK.");
		}
	}

	if (gConnect && this->player) {
		mpd_signal_connect_status_changed(this->player,
						  mpdCallbackStateChanged,
						  this);
		mpdCallbackStateChanged(this->player,
					MPD_CST_SONGID | MPD_CST_STATE |
					MPD_CST_RANDOM | MPD_CST_REPEAT, this);
	}

	LOG("Leave mpdAssure");
	return (this->player != NULL);
}

gint mpdCallback(gpointer thsPtr) {
	MKTHIS;
	if (this->player != NULL)
		mpd_status_update(this->player);
	return TRUE;
}

gboolean mpdNext(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter mpdNext");
	if (!mpdAssure(this, TRUE))
		return FALSE;
	else
		bRet = (MPD_OK == mpd_player_next(this->player));
	LOG("Leave mpdNext");
	return bRet;
}

gboolean mpdPrevious(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter mpdPrevious");
	if (!mpdAssure(this, TRUE))
		return FALSE;
	else
		bRet = (MPD_OK == mpd_player_prev(this->player));
	LOG("Leave mpdPrevious");
	return TRUE;
}

gboolean mpdPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter mpdPlayPause");
	int iRet = MPD_OK;
	if (!mpdAssure(this, FALSE))
		return FALSE;
	else if (newState)
		iRet = mpd_player_play(this->player);
	else
		iRet = mpd_player_pause(this->player);
	LOG("Leave mpdPlayPause");
	return (iRet == MPD_OK);
}

gboolean mpdIsPlaying(gpointer thsPtr) {
	MKTHIS;
	MpdState sStat = MPD_STATUS_STATE_STOP;

	LOG("Enter mpdIsPlaying");
	if (!mpdAssure(this, TRUE))
		return FALSE;
	if (this->player) {
		sStat = mpd_player_get_state(this->player);
	}
	LOG("Leave mpdIsPlaying");
	return (sStat == MPD_STATUS_STATE_PLAY);
}

gboolean mpdToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean oldState = FALSE;
	LOG("Enter mpdToggle");
	if (!mpdAssure(this, FALSE))
		return FALSE;
	oldState = mpdIsPlaying(this);
	if (!mpdPlayPause(this, !oldState))
		return FALSE;
	if (newState)
		*newState = !oldState;
	LOG("Leave mpdToggle");
	return TRUE;
}

gboolean mpdDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter mpdDetach");
	if (this->player) {
		mpd_disconnect(this->player);
		mpd_free(this->player);
		this->player = NULL;
	}
	if (this->intervalID) {
		g_source_remove(this->intervalID);
		this->intervalID = 0;
	}
	LOG("Leave mpdDetach");
	return TRUE;
}

gboolean mpdShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter mpdShow");
	const gchar *argv[] = {
		this->pmanager->str,
		// here we could have arguments
		NULL
	};
	g_spawn_async(NULL, (gchar**)argv, NULL, 
		G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL,
		NULL, NULL, NULL, NULL);
	LOG("Leave mpdShow");
	return this->bUsePManager;
}

gboolean mpdGetRepeat(gpointer thsPtr) {
	MKTHIS;
	if (this->player)
		return mpd_player_get_repeat(this->player);
	return FALSE;
}

gboolean mpdSetRepeat(gpointer thsPtr, gboolean newShuffle) {
	MKTHIS;
	if (mpdAssure(thsPtr, TRUE)) {
		return (MPD_OK ==
			mpd_player_set_repeat(this->player,
					      (newShuffle) ? 1 : 0));
	}
	return FALSE;
}

gboolean mpdGetShuffle(gpointer thsPtr) {
	MKTHIS;
	if (this->player)
		return mpd_player_get_random(this->player);
	return FALSE;
}

gboolean mpdSetShuffle(gpointer thsPtr, gboolean newRandom) {
	MKTHIS;
	if (mpdAssure(thsPtr, TRUE)) {
		return (MPD_OK ==
			mpd_player_set_random(this->player,
					      (newRandom) ? 1 : 0));
	}
	return FALSE;
}

void mpdCallbackStateChanged(MpdObj * player, ChangedStatusType sType,
			     gpointer thsPtr) {
	MKTHIS;
	if (sType & MPD_CST_SONGID) {
		LOG("Enter mpdCallback: SongChanged");
		mpd_Song *song = mpd_playlist_get_current_song(this->player);
		if (mpdAssure(thsPtr, TRUE) && song != NULL
		    && song->id != this->songID) {
			GString *artLocation = NULL;
			gboolean bFound = FALSE;

			g_string_assign(this->parent->artist, song->artist);
			g_string_assign(this->parent->album, song->album);
			g_string_assign(this->parent->title, song->title);

			//fetching gmpc covers
			artLocation = g_string_new(g_get_home_dir());
			g_string_append_printf(artLocation,
					       "/.covers/%s - %s.jpg",
					       song->artist, song->album);
			LOG("Check 1:'%s'", artLocation->str);
			g_string_truncate(this->parent->albumArt, 0);

			if (g_file_test(artLocation->str, G_FILE_TEST_EXISTS)) {
				bFound = TRUE;
			} else {
				if (this->bUseMPDFolder
				    && g_file_test(this->path->str,
						   G_FILE_TEST_EXISTS)) {
					g_string_printf(artLocation, "%s/%s",
							this->path->str,
							song->file);
					this->
					    parent->FindAlbumArtByFilePath
					    (this->parent->sd,
					     artLocation->str);
				}
			}
			if (bFound) {
				// just assign here, scaling is done in callee
				g_string_assign(this->parent->albumArt,
						artLocation->str);
				LOG("Found :'%s'", artLocation->str);
			}

			g_string_free(artLocation, TRUE);
			this->parent->Update(this->parent->sd, TRUE, estPlay,
					     NULL);
			this->songID = song->id;
		}
		LOG("Leave mpdCallback: SongChanged");
	}
	if (sType & MPD_CST_STATE) {
		LOG("Enter mpdCallback: StateChanged");
		eSynoptics eStat;
		MpdState state = mpd_player_get_state(this->player);
		switch (state) {
		    case MPD_PLAYER_PAUSE:
			    eStat = estPause;
			    break;
		    case MPD_PLAYER_PLAY:
			    eStat = estPlay;
			    break;
		    case MPD_PLAYER_STOP:
			    eStat = estStop;
			    break;
		    default:
			    eStat = estErr;
			    break;
		}
		this->parent->Update(this->parent->sd, FALSE, eStat, NULL);
		LOG("Leave mpdCallback: StateChanged");
	}
	if (sType & MPD_CST_REPEAT) {
		LOG("Enter mpdCallback: RepeatChanged");
		this->parent->UpdateRepeat(this->parent->sd,
					   mpd_player_get_repeat(this->player));
		LOG("Leave mpdCallback: RepeatChanged");
	}
	if (sType & MPD_CST_RANDOM) {
		LOG("Enter mpdCallback: ShuffleChanged");
		this->parent->UpdateShuffle(this->parent->sd,
					    mpd_player_get_random
					    (this->player));
		LOG("Leave mpdCallback: ShuffleChanged");
	}
	if (sType & (MPD_CST_ELAPSED_TIME | MPD_CST_TOTAL_TIME)) {
		/* -- display seconds in some distant future
		   if( sType & MPD_CST_ELAPSED_TIME )
		   this->parent->secPos = 
		   mpd_status_get_elapsed_song_time(this->player);
		   if( sType & MPD_CST_TOTAL_TIME )
		   this->parent->secTot = 
		   mpd_status_get_total_song_time(this->player);
		   this->parent->UpdateTimePosition(this->parent->sd);
		 */
	}
	//MPD_CST_TOTAL_TIME
}

void mpdPersist(gpointer thsPtr, gboolean bIsStoring) {
	LOG("Enter mpdPersist");
	MKTHIS;
	if(bIsStoring){
		// nothing to do
	} else {
		SPlayer * parent = this->parent;
		if(this->bUsePManager && (NULL != g_find_program_in_path(this->pmanager->str))) {
			MPD_MAP(Show);
		} else {
			NOMAP(Show);
		}
		if(mpdAssure(thsPtr, FALSE) && NULL != this->player) {
			mpdCallbackStateChanged(this->player, MPD_SQ_ALL, thsPtr);
		}
	}	
	LOG("Leave mpdPersist");
}

static void mpdSettingsDialogResponse(GtkWidget * dlg, int reponse,
				      gpointer thsPtr) {
	LOG("Enter mpdSettingsDialogResponse");
	MKTHIS;
	const gchar *tmpHost = gtk_entry_get_text(GTK_ENTRY(this->wHost));

	const gchar *tmpPass = gtk_entry_get_text(GTK_ENTRY(this->wPass));

	const gchar *tmpPath =
	    gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(this->wPath));
                          
    const gchar *tmpPManager = 
        gtk_combo_box_get_active_text(GTK_COMBO_BOX(this->wPMgr));
                          
    LOG("MPD PManager: %s", tmpPManager);

	if (g_strcasecmp(tmpHost, this->host->str)) {
		g_string_assign(this->host, tmpHost);
		this->bRequireReconnect = TRUE;
	}

	if (g_strcasecmp(tmpPass, this->pass->str)) {
		g_string_assign(this->pass, tmpPass);
		this->bRequireReconnect = TRUE;
	}

	if (!g_str_equal(this->path, tmpPath)) {
		g_string_assign(this->path, tmpPath);
	}

	if (!g_str_equal(this->pmanager, tmpPManager)) {
		g_string_assign(this->pmanager, tmpPManager);
	}
    // reconnect if changed
	if (0)	// this->bRequireReconnect )
	{
		LOG("    Reconnecting to %s/%s", tmpHost, this->host->str);
		SPlayer *p = this->parent;
		this->bRequireReconnect = FALSE;
		mpdDetach(thsPtr);
		MPD_attach(p);
		LOG("    Done reconnect.");
	}
    
    // don't allow bogus playlist manager
    if(this->bUsePManager && this->pmanager->len) {
        gchar *binPath = g_find_program_in_path(this->pmanager->str);
        if(NULL == binPath) {
             GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_CLOSE,
                                              _("Can't find binary '%s'"),
                                              this->pmanager->str);
            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
            gtk_widget_grab_focus(this->wPMgr);
            goto errExit;
        }
    }
	
	// force update
	mpdPersist(thsPtr, FALSE);    
	gtk_widget_destroy(dlg);
errExit:
	LOG("Leave mpdSettingsDialogResponse");
}

static void mpdConfigureTimeout(GtkSpinButton * sb, gpointer thsPtr) {
	MKTHIS;
	this->port = gtk_spin_button_get_value_as_int(sb);
	this->bRequireReconnect = TRUE;
}

static void mpdConfigureUseDefault(GtkToggleButton * tb, gpointer thsPtr) {
	MKTHIS;
	this->bUseDefault = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(this->wHost, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPort, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPass, !this->bUseDefault);
	this->bRequireReconnect = TRUE;
}

static void mpdConfigureUseMPDFolder(GtkToggleButton * tb, gpointer thsPtr) {
	LOG("Enter mpdConfigureUseMPDFolder");
	MKTHIS;
	this->bUseMPDFolder = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(this->wPath, this->bUseMPDFolder);
	LOG("Leave mpdConfigureUseMPDFolder");
}

static void mpdConfigureUsePManager(GtkToggleButton * tb, gpointer thsPtr) {
	LOG("Enter mpdConfigureUsePMgr");
	MKTHIS;
	this->bUsePManager = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(this->wPMgr, this->bUsePManager);
	LOG("Leave mpdConfigureUsePMgr");
}

static void mpdConfigure(gpointer thsPtr, GtkWidget * parent) {
	LOG("Enter mpdConfigure");
	MKTHIS;
	GtkWidget *dlg, *header, *vbox, *cb1, *label1, *label2, *label3, *cb2, *cb3;
	GtkTable *table;
    GtkListStore *store;
    GtkTreeIter iter = {0};

	this->bRequireReconnect = FALSE;

	dlg = gtk_dialog_new_with_buttons(_("Properties"),
					  GTK_WINDOW(parent),
					  GTK_DIALOG_MODAL |
					  GTK_DIALOG_NO_SEPARATOR,
					  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
					  NULL);

	this->wDlg = dlg;

	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(GTK_WINDOW(dlg), "xfce4-sound");

	g_signal_connect(dlg, "response", G_CALLBACK(mpdSettingsDialogResponse),
			 thsPtr);

	gtk_container_set_border_width(GTK_CONTAINER(dlg), 2);

	header = xfce_heading_new();
	xfce_heading_set_title(XFCE_HEADING(header), _("MPD"));
	xfce_heading_set_icon(XFCE_HEADING(header),
			      gdk_pixbuf_new_from_inline(sizeof(my_pixbuf),
							 my_pixbuf, FALSE,
							 NULL));
	xfce_heading_set_subtitle(XFCE_HEADING(header),
				  _("music player daemon"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), header,
			   FALSE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), vbox, TRUE, TRUE, 0);

	cb1 = gtk_check_button_new_with_mnemonic(_("Use _defaults"));
	gtk_box_pack_start(GTK_BOX(vbox), cb1, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb1), this->bUseDefault);
	g_signal_connect(cb1, "toggled", G_CALLBACK(mpdConfigureUseDefault),
			 thsPtr);

	table = GTK_TABLE(gtk_table_new(3, 2, FALSE));
	gtk_container_set_border_width(GTK_CONTAINER(table), 6);
	gtk_table_set_row_spacings(table, 6);
	gtk_table_set_col_spacings(table, 6);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(table), FALSE, FALSE, 0);

	label1 = gtk_label_new_with_mnemonic(_("_Host:"));
	gtk_table_attach_defaults(table, label1, 0, 1, 0, 1);

	this->wHost = gtk_entry_new();
	gtk_table_attach_defaults(table, this->wHost, 1, 2, 0, 1);
	gtk_entry_set_text(GTK_ENTRY(this->wHost), this->host->str);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), this->wHost);

	label2 = gtk_label_new_with_mnemonic(_("_Port:"));
	gtk_table_attach_defaults(table, label2, 0, 1, 1, 2);

	this->wPort = gtk_spin_button_new_with_range(1.0, 65536.0, 1.0);
	gtk_table_attach_defaults(table, this->wPort, 1, 2, 1, 2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(this->wPort), this->port);
	g_signal_connect(this->wPort, "value-changed",
			 G_CALLBACK(mpdConfigureTimeout), thsPtr);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label2), this->wPort);

	label3 = gtk_label_new_with_mnemonic(_("Pass_word:"));
	gtk_table_attach_defaults(table, label3, 0, 1, 2, 3);

	this->wPass = gtk_entry_new();
	gtk_table_attach_defaults(table, this->wPass, 1, 2, 2, 3);
	gtk_entry_set_visibility(GTK_ENTRY(this->wPass), FALSE);
	gtk_entry_set_text(GTK_ENTRY(this->wPass), this->pass->str);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label3), this->wPass);

	// album art lookup
	cb2 =
	    gtk_check_button_new_with_mnemonic(_
					       ("Use MPD Music _folder as cover source"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(cb2), FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb2),
				     this->bUseMPDFolder);
	g_signal_connect(cb2, "toggled", G_CALLBACK(mpdConfigureUseMPDFolder),
			 thsPtr);

	this->wPath = gtk_file_chooser_button_new(_("Select MPD music folder"),
						  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	LOG("Music folder: %s", this->path->str);
	if (this->path->len)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER
						    (this->wPath),
						    this->path->str);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(this->wPath), FALSE, FALSE,
			   0);
    
    // player selection
    cb3 = gtk_check_button_new_with_mnemonic(_("Use a playlist _manager:"));
	gtk_box_pack_start(GTK_BOX(vbox), cb3, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb3),
				     this->bUsePManager);
    store = gtk_list_store_new(1, G_TYPE_STRING);
    this->wPMgr = gtk_combo_box_entry_new_with_model(
         GTK_TREE_MODEL(store), 0);
    if(this->pmanager->len) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, this->pmanager->str, -1);
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0, GTK_SORT_ASCENDING);
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(this->wPMgr), &iter);
    }
	gtk_box_pack_start(GTK_BOX(vbox), this->wPMgr, FALSE, FALSE, 0);
	g_signal_connect(cb3, "toggled", G_CALLBACK(mpdConfigureUsePManager),
			 thsPtr);
    // cheapo tracker search, requires gio-unix for now
    #if HAVE_GIO
    {
        gchar *tracker = g_find_program_in_path("tracker-search");
        if(tracker) {
            GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
            if(this->pmanager->len)
                g_hash_table_insert(table, this->pmanager->str, g_strdup("!"));
	        gchar *outText = NULL;
	        const gchar *argv[] = {
		        tracker,
		        "--service",
                "Applications",
                "mpd",
		        NULL
	        };
	        gint exit_status = 0;
	        if (g_spawn_sync(NULL, (gchar **) argv, NULL,
			         G_SPAWN_STDERR_TO_DEV_NULL,
			         NULL, NULL, &outText, NULL, &exit_status, NULL)) {
                 gchar *ptr = strtok(outText, "\n");
                 while(ptr) {
                     GDesktopAppInfo *app = g_desktop_app_info_new_from_filename(ptr);
                     if(app) {
                         const gchar *binPath = g_app_info_get_executable(
                                                G_APP_INFO(app));   
                         if(!g_hash_table_lookup_extended(table, binPath, NULL, NULL)) {
                             LOG("Found: %s", binPath);
                             gtk_list_store_append(store, &iter);
                             gtk_list_store_set(store, &iter, 0, binPath, -1);
                             g_hash_table_insert(table, (gchar*)binPath, g_strdup("."));
                         }
                     }
                     ptr = strtok(NULL, "\n");
                 }
            }
        }
    }
	#endif
	// apply checkboxes
	gtk_widget_set_sensitive(this->wHost, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPort, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPass, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPath, this->bUseMPDFolder);
    gtk_widget_set_sensitive(this->wPMgr, this->bUsePManager);

	gtk_widget_show_all(dlg);
	LOG("Leave mpdConfigure");
}

void *MPD_attach(SPlayer * parent) {
	mpdData *this = g_new0(mpdData, 1);
	LOG("Enter MPD_attach");
	MPD_MAP(Assure);
	MPD_MAP(Next);
	MPD_MAP(Previous);
	MPD_MAP(PlayPause);
	MPD_MAP(IsPlaying);
	MPD_MAP(Toggle);
	MPD_MAP(Detach);
	MPD_MAP(Persist);
	MPD_MAP(Configure);
	NOMAP(IsVisible);
	NOMAP(Show);
	NOMAP(UpdateDBUS);
	MPD_MAP(GetRepeat);
	MPD_MAP(SetRepeat);
	MPD_MAP(GetShuffle);
	MPD_MAP(SetShuffle);

	this->host = g_string_new("");
	this->pass = g_string_new("");
	this->path = g_string_new("");
	this->pmanager = g_string_new("");
    
	// always assign
	this->parent = parent;

	// force our own update rate
	if (parent->updateRateMS < 500)
		parent->updateRateMS = 500;

	// establish the callback function
	this->intervalID =
	    g_timeout_add(parent->updateRateMS, mpdCallback, this);
    
    // initialize property addresses
    PROP_MAP("mpd_usedefault", &this->bUseDefault)
    PROP_MAP("mpd_port", &this->port)
    PROP_MAP("mpd_host", this->host)
    PROP_MAP("mpd_pass", this->pass)
    PROP_MAP("mpd_usemusicfolder", &this->bUseMPDFolder)
    PROP_MAP("mpd_musicfolder", this->path)
	PROP_MAP("mpd_usepmanager", &this->bUsePManager)
	PROP_MAP("mpd_pmanager", this->pmanager)


	LOG("Leave MPD_attach");
	return this;
}
#endif
