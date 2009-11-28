/***************************************************************************
 *            squeezebox-mpd.c
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

// settings dialog
#include "settings-mpd-ui.h"

DEFINE_BACKEND(MPD, _("Music Player Daemon"))
#define MPD_MAP(a) parent->a = mpd##a
#ifndef MPD_CST_STORED_PLAYLIST
#define MPD_CST_STORED_PLAYLIST 0x20000
#endif
#define MPD_SQ_ALL (MPD_CST_SONGID|MPD_CST_STATE|MPD_CST_REPEAT|MPD_CST_RANDOM|MPD_CST_STORED_PLAYLIST|MPD_CST_PLAYLIST)
typedef struct mpdData{
	SPlayer *parent;
	guint port;
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
	XfconfChannel *xfconfChannel;
} mpdData;

#define MKTHIS mpdData *this = (mpdData *)thsPtr;

void *MPD_attach(SPlayer * player);
void mpdCallbackStateChanged(MpdObj * player, ChangedStatusType sType,
			     gpointer thsPtr);
gint mpdCallback(gpointer thsPtr);
gboolean mpdAssure(gpointer thsPtr, gboolean noCreate) {

	MKTHIS;
	gboolean gConnect = FALSE;
	LOG("Enter mpdAssure");
	if (!this->player) {
		if (this->bUseDefault) {
			LOG("    Connecting local...");
			this->player = mpd_new_default();
		} else {
			LOG("    Connecting to remote host %s:%d...", this->host->str, this->port);
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
		mpdCallbackStateChanged(this->player, MPD_SQ_ALL, this);
	}

	LOG("Leave mpdAssure");
	return (this->player != NULL);
}

gint mpdCallback(gpointer thsPtr) {
	MKTHIS;
	if (this->player != NULL) {
		gint stat = mpd_status_update(this->player);
		if(MPD_OK != stat) {
			LOG("Unexpected mpd status %d", stat);
		}
	}
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

gboolean mpdPlayPlaylist(gpointer thsPtr, gchar *playlistName) {
	MKTHIS;
	MpdState state = mpd_player_get_state(this->player);
	LOG("Enter mpdPlayPlaylist");
	mpd_playlist_clear(this->player);
	mpd_playlist_queue_load(this->player, playlistName);
	mpd_playlist_queue_commit(this->player);
	switch(state) {
		case MPD_STATUS_STATE_PLAY:
			mpd_player_play(this->player);
			break;
		case MPD_STATUS_STATE_PAUSE:
			mpd_player_play(this->player);
			mpd_player_pause(this->player);
			break;
		case MPD_PLAYER_STOP:
		case MPD_PLAYER_UNKNOWN:
			break;
	}
	LOG("Leave mpdPlayPlaylist");
	return TRUE;
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
	if (this->xfconfChannel) {
		g_object_unref(this->xfconfChannel);
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
	// display elapsed/total ?
#if 0
	if (sType & (MPD_CST_ELAPSED_TIME | MPD_CST_TOTAL_TIME)) {
		   if( sType & MPD_CST_ELAPSED_TIME )
			   this->parent->secPos = 
				   mpd_status_get_elapsed_song_time(this->player);
		   if( sType & MPD_CST_TOTAL_TIME )
			   this->parent->secTot = 
			   mpd_status_get_total_song_time(this->player);
		   this->parent->UpdateTimePosition(this->parent->sd);
	}
#endif
	if(sType & (MPD_CST_STORED_PLAYLIST)) {
		mpd_Connection * conn = NULL;
		mpd_InfoEntity * entity = NULL;
		char * ls = "";
		LOG("Enter mpdCallback: PlaylistChanged");
		if(this->bUseDefault)
			conn = mpd_newConnection("localhost", 6600, 150);
		else
			conn = mpd_newConnection(this->host->str, this->port, 1500);
		if(conn) {
			g_hash_table_remove_all(this->parent->playLists);
			mpd_sendLsInfoCommand(conn,ls);

			while((entity = mpd_getNextInfoEntity(conn))) {
				if(entity->type==
						MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
					mpd_PlaylistFile * pl = entity->info.playlistFile;
					g_hash_table_insert(this->parent->playLists, 
						g_strdup(pl->path), g_strdup("stock_playlist"));
				}
				mpd_freeInfoEntity(entity);
			}
			this->parent->UpdatePlaylists(this->parent->sd);
		}
		LOG("Leave mpdCallback: PlaylistChanged");
	}
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

EXPORT void on_mpdSettings_response(GtkDialog * dlg, int reponse,
				      gpointer thsPtr) {
	MKTHIS;
	gchar *manager = NULL;
	LOG("Enter on_mpdSettings_response");

    // reconnect if changed
	if (this->bRequireReconnect )
	{
		LOG("    Reconnecting to %s", this->host->str);
		this->bRequireReconnect = FALSE;
		if (this->player) {
			mpd_disconnect(this->player);
			mpd_free(this->player);
			this->player = NULL;
		}
		mpdAssure(thsPtr, FALSE);
		LOG("    Done reconnect.");
	}
    
    // don't allow bogus playlist manager
    manager = gtk_combo_box_get_active_text(GTK_COMBO_BOX(this->wPMgr));
    g_string_assign(this->pmanager, manager);
    g_free(manager);
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
	gtk_widget_hide(GTK_WIDGET(dlg));
errExit:
	LOG("Leave on_mpdSettings_response");
}

EXPORT void on_chkUseDefault_toggled(GtkToggleButton * tb, gpointer thsPtr) {
	MKTHIS;
	LOG("Enter on_chkUseDefault_toggled");
	this->bUseDefault = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(this->wHost, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPort, !this->bUseDefault);
	gtk_widget_set_sensitive(this->wPass, !this->bUseDefault);
	this->bRequireReconnect = TRUE;
	LOG("Leave on_chkUseDefault_toggled");
}

EXPORT void on_entryHost_changed(GtkEntry *tbd, gpointer thsPtr) {
	MKTHIS;
	g_string_assign(this->host, gtk_entry_get_text(tbd));
	this->bRequireReconnect = TRUE;
}

EXPORT void on_entryPassword_changed(GtkEntry *tbd, gpointer thsPtr) {
	MKTHIS;
	g_string_assign(this->pass, gtk_entry_get_text(tbd));
	this->bRequireReconnect = TRUE;
}

EXPORT void on_chkUseFolder_toggled(GtkToggleButton * tb, gpointer thsPtr) {
	MKTHIS;
	LOG("Enter on_chkUseFolder_toggled");
	this->bUseMPDFolder = gtk_toggle_button_get_active(tb);
	this->bRequireReconnect = TRUE;
	gtk_widget_set_sensitive(this->wPath, this->bUseMPDFolder);
	LOG("Leave on_chkUseFolder_toggled");
}

EXPORT void on_spinPort_value_changed(GtkSpinButton *tbd, gpointer thsPtr) {
	MKTHIS;
	this->port = (guint)gtk_spin_button_get_value_as_int(tbd);
	this->bRequireReconnect = TRUE;
}

EXPORT void on_chooserDirectory_current_folder_changed(GtkFileChooserButton *chb, gpointer thsPtr) {
	MKTHIS;
	gchar *text = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chb));
	LOG("Enter on_chooserDirectory_current_folder_changed: %s", text);
	xfconf_channel_set_string(this->xfconfChannel, "/MusicFolder", text);
	LOG("Leave on_chooserDirectory_current_folder_changed");
}

EXPORT void on_chkUseListManager_toggled(GtkToggleButton * tb, gpointer thsPtr) {
	LOG("Enter on_chkUseListManager_toggled");
	MKTHIS;
	this->bUsePManager = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(this->wPMgr, this->bUsePManager);
	LOG("Leave on_chkUseListManager_toggled");
}

gchar** mpdGetIndexerArgs(){
	gchar *tracker = g_find_program_in_path("tracker-search");
	gchar **argv = NULL;
	if(tracker) {
		argv = g_new(gchar*, 5);
		argv[0] = tracker;
		argv[1] = "--service";
		argv[2] = "Applications";
		argv[3] = "mpd";
		argv[4] = NULL;
	} else {
		gchar *beagle = g_find_program_in_path("beagle-query");
		if(beagle) {
			argv = g_new(gchar*, 4);
			argv[0] = beagle;
			argv[1] = "source:applications";
			argv[2] = "mpd";
			argv[3] = NULL;
		}
	}
	if(argv)
		LOG("\tIndexer is:%s", argv[0]);
	return argv;
}

static void mpdConfigure(gpointer thsPtr, GtkWidget * parent) {
	LOG("Enter mpdConfigure");
	MKTHIS;
	GtkWidget *header, *vbox, *cb1, *label1, *label2, *label3, *cb2, *cb3;
	GtkTable *table;
    GtkListStore *store;
    GtkTreeIter iter = {0};
    gint result;
    GtkBuilder* builder = gtk_builder_new();

	gtk_builder_add_from_string(builder, settings_mpd_ui, 
		settings_mpd_ui_length, NULL);
		
	
	this->wDlg = GTK_WIDGET(gtk_builder_get_object(builder, "mpdSettings"));
	this->wHost = GTK_WIDGET(gtk_builder_get_object(builder, "entryHost"));
	this->wPort = GTK_WIDGET(gtk_builder_get_object(builder, "spinPort"));
	this->wPass = GTK_WIDGET(gtk_builder_get_object(builder, "entryPassword"));
	this->wPMgr = GTK_WIDGET(gtk_builder_get_object(builder, "comboListManager"));
	this->wPath = GTK_WIDGET(gtk_builder_get_object(builder, "chooserDirectory"));

	header = GTK_WIDGET(gtk_builder_get_object(builder, "xfce-heading1"));
	xfce_heading_set_icon(XFCE_HEADING(header),
				  gdk_pixbuf_new_from_inline(sizeof(my_pixbuf),
							 my_pixbuf, FALSE,
							 NULL));

	gtk_builder_connect_signals(builder,thsPtr);

  	xfconf_g_property_bind (this->xfconfChannel, "/UseDefault", G_TYPE_BOOLEAN,
                          gtk_builder_get_object(builder, "chkUseDefault"), "active");
  	xfconf_g_property_bind (this->xfconfChannel, "/Host", G_TYPE_STRING,
                          gtk_builder_get_object(builder, "entryHost"), "text");
  	xfconf_g_property_bind (this->xfconfChannel, "/Port", G_TYPE_UINT,
                          gtk_builder_get_object(builder, "spinPort"), "value");
  	xfconf_g_property_bind (this->xfconfChannel, "/Password", G_TYPE_STRING,
                          gtk_builder_get_object(builder, "entryPassword"), "text");
  	
  	xfconf_g_property_bind (this->xfconfChannel, "/UseFolder", G_TYPE_BOOLEAN,
                          gtk_builder_get_object(builder, "chkUseFolder"), "active");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(this->wPath), 
		xfconf_channel_get_string(this->xfconfChannel, "/MusicFolder", "~"));
  	
  	xfconf_g_property_bind (this->xfconfChannel, "/UseListManager", G_TYPE_BOOLEAN,
                          gtk_builder_get_object(builder, "chkUseListManager"), "active");
  	xfconf_g_property_bind (this->xfconfChannel, "/ListManager", G_TYPE_STRING,
                  gtk_bin_get_child(GTK_BIN(GTK_COMBO_BOX_ENTRY(
                  gtk_builder_get_object(builder, "comboListManager")))), "text");
	store = GTK_LIST_STORE(gtk_builder_get_object(builder, "listManagers"));

    // cheapo tracker/beagle search, requires gio-unix for now
    /* TODO: make it work with translations */
    #if HAVE_GIO
    {
		gchar **argv = mpdGetIndexerArgs();
		gchar *ptr = NULL;
        if(argv) {
            GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
            if(this->pmanager->len)
                g_hash_table_insert(table, this->pmanager->str, g_strdup("!"));
	        gchar *outText = NULL;
	        gint exit_status = 0;
	        if (g_spawn_sync(NULL, argv, NULL, G_SPAWN_STDERR_TO_DEV_NULL,
			         NULL, NULL, &outText, NULL, &exit_status, NULL)) {
                 LOG("Indexer says:%s", outText);
				 /* TODO: replace strtok with g_strsplit */
                 ptr = strtok(outText, "\n");
                 while(ptr) {
                     gchar *returnPath = ptr;
                     while(*returnPath && *returnPath == ' ')
                        returnPath++;
                     if(g_str_has_prefix(returnPath, "file://"))
						returnPath = g_filename_from_uri(returnPath, NULL, NULL);
					 else
						returnPath = g_strdup(returnPath);
                     if(g_file_test(returnPath, G_FILE_TEST_EXISTS))
                     {
                         GDesktopAppInfo *app = 
                             g_desktop_app_info_new_from_filename(returnPath);
                         if(app) {
                             const gchar *binPath = g_app_info_get_executable(
                                                    G_APP_INFO(app));   
                             if(!g_hash_table_lookup_extended(table, binPath, NULL, NULL)) {
								 GIcon *icon = g_app_info_get_icon(
                                                    G_APP_INFO(app));   
                                 LOG("\tFound: %s", binPath);
                                 gtk_list_store_append(store, &iter);
                                 gtk_list_store_set(store, &iter, 0, binPath, -1);
                                 g_hash_table_insert(table, (gchar*)binPath, g_strdup("."));
                             }
                         }
                     }
                     g_free(returnPath);
                     ptr = strtok(NULL, "\n");
                 }
            }
            g_free(argv);
        }
    }
	#endif

	// fin
jumpTarget:
	this->bRequireReconnect = FALSE;
	//gtk_dialog_run
	gtk_window_set_transient_for(GTK_WINDOW(this->wDlg), GTK_WINDOW(parent));
	result = gtk_dialog_run (GTK_DIALOG (this->wDlg));
	xfconf_g_property_unbind_all(this->xfconfChannel);

	LOG("Leave mpdConfigure");
}

void *MPD_attach(SPlayer * parent) {
	mpdData *this = g_new0(mpdData, 1);
	LOG("Enter MPD_attach");
	MPD_MAP(Assure);
	MPD_MAP(Next);
	MPD_MAP(Previous);
	MPD_MAP(PlayPause);
	MPD_MAP(PlayPlaylist);
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
	NOMAP(UpdateWindow);

	this->host = g_string_new("");
	this->pass = g_string_new("");
	this->path = g_string_new("");
	this->pmanager = g_string_new("");
    
	// always assign
	this->parent = parent;
	this->wDlg = NULL;

	// force our own update rate
	if (parent->updateRateMS < 1000)
		parent->updateRateMS = 1000;

	// establish the callback function
	this->intervalID =
		g_timeout_add(this->parent->updateRateMS, mpdCallback, this);
		
	// initialize properties

    this->xfconfChannel = xfconf_channel_new_with_property_base (
    	"xfce4-panel", "/plugins/squeezebox/backends/mpd");
	this->bUseDefault = xfconf_channel_get_bool(this->xfconfChannel, "/UseDefault", TRUE);
	this->host->str = xfconf_channel_get_string(this->xfconfChannel, "/Host", "localhost");
	this->port = xfconf_channel_get_uint(this->xfconfChannel, "/Port", 6600);
	this->pass->str = xfconf_channel_get_string(this->xfconfChannel, "/Password", "");
	this->bUseMPDFolder = xfconf_channel_get_bool(this->xfconfChannel, "/UseFolder", TRUE);
	this->path->str = xfconf_channel_get_string(this->xfconfChannel, "/MusicFolder", "~");
	this->bUsePManager = xfconf_channel_get_bool(this->xfconfChannel, "/UseListManager", TRUE);
	this->pmanager->str = xfconf_channel_get_string(this->xfconfChannel, "/ListManager", "");
	

	LOG("Leave MPD_attach");
	return this;
}
#endif
