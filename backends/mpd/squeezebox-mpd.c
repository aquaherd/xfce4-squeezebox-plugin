/***************************************************************************
 *            squeezebox-mpd.c
 *
 *  Fri Aug 25 17:20:09 2006
 *  Copyright  2006  Hakan Erduman
 *  Email hakan@erduman.de
 ****************************************************************************/

/*
 *  thys program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  thys program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with thys program; if not, write to the Free Software
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
#include <libxfcegui4/libxfcegui4.h>

// standalone glib mpd
#include "gmpd.h"

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
	GMpd *gmpd;
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

#define MKTHIS mpdData *thys = (mpdData *)thsPtr;

gpointer MPD_attach(SPlayer * player);
void mpdCallbackStateChanged(MpdObj * player, ChangedStatusType sType,
			     gpointer thsPtr);
gint mpdCallback(gpointer thsPtr);

#define BASENAME "mpd"
DEFINE_BACKEND(MPD, _("Music Player Daemon"))

// implementation

gboolean mpdAssure(gpointer thsPtr, gboolean noCreate) {

	MKTHIS;
	gboolean gConnect = FALSE;
	LOG("Enter mpdAssure");
	if (!thys->player) {
		if (thys->bUseDefault) {
			LOG("    Connecting local...");
			thys->player = mpd_new_default();
		} else {
			LOG("    Connecting to remote host %s:%d...", thys->host->str, thys->port);
			thys->player = mpd_new(thys->host->str,
					       thys->port, thys->pass->str);
		}
		if (thys->player) {
			if (MPD_OK == mpd_connect(thys->player)) {
				LOG("Connect OK");
				gConnect = TRUE;
			}
		}
	}
	if (thys->player != NULL && mpd_status_update(thys->player) != MPD_OK) {
		LOG("reconnecting..");
		mpd_connect(thys->player);
		//if(!thys->bUseDefault)
		//mpd_send_password(thys->player);
		if (mpd_check_error(thys->player)
		    || mpd_status_update(thys->player) != MPD_OK) {
			LOG(".Fail.");
			g_string_assign(thys->parent->artist, "");
			g_string_assign(thys->parent->album, "");
			g_string_assign(thys->parent->title, "");
			g_string_assign(thys->parent->albumArt, "");
			thys->parent->Update(thys->parent->sd, TRUE, estErr,
					     _("Can't connect to music player daemon."));
			mpd_disconnect(thys->player);
			mpd_free(thys->player);
			thys->player = NULL;
		} else {
			LOG(".OK.");
			gConnect = TRUE;
		}
	}

	if (gConnect && thys->player) {
		mpd_signal_connect_status_changed(thys->player,
						  mpdCallbackStateChanged,
						  thys);
		mpdCallbackStateChanged(thys->player, MPD_SQ_ALL, thys);
	}
	
	if (!thys->gmpd) {
		thys->gmpd = g_mpd_new();
	}

	LOG("Leave mpdAssure");
	return (thys->player != NULL);
}

gint mpdCallback(gpointer thsPtr) {
	MKTHIS;
	if (thys->player != NULL) {
		gint stat = mpd_status_update(thys->player);
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
	if (!mpdAssure(thys, TRUE))
		return FALSE;
	else
		bRet = (MPD_OK == mpd_player_next(thys->player));
	LOG("Leave mpdNext");
	return bRet;
}

gboolean mpdPrevious(gpointer thsPtr) {
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter mpdPrevious");
	if (!mpdAssure(thys, TRUE))
		return FALSE;
	else
		bRet = (MPD_OK == mpd_player_prev(thys->player));
	LOG("Leave mpdPrevious");
	return TRUE;
}

gboolean mpdPlayPause(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter mpdPlayPause");
	int iRet = MPD_OK;
	if (!mpdAssure(thys, FALSE))
		return FALSE;
	else if (newState)
		iRet = mpd_player_play(thys->player);
	else
		iRet = mpd_player_pause(thys->player);
	LOG("Leave mpdPlayPause");
	return (iRet == MPD_OK);
}

gboolean mpdPlayPlaylist(gpointer thsPtr, gchar *playlistName) {
	MKTHIS;
	MpdState state = mpd_player_get_state(thys->player);
	LOG("Enter mpdPlayPlaylist");
	mpd_playlist_clear(thys->player);
	mpd_playlist_queue_load(thys->player, playlistName);
	mpd_playlist_queue_commit(thys->player);
	switch(state) {
		case MPD_STATUS_STATE_PLAY:
			mpd_player_play(thys->player);
			break;
		case MPD_STATUS_STATE_PAUSE:
			mpd_player_play(thys->player);
			mpd_player_pause(thys->player);
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
	if (!mpdAssure(thys, TRUE))
		return FALSE;
	if (thys->player) {
		sStat = mpd_player_get_state(thys->player);
	}
	LOG("Leave mpdIsPlaying");
	return (sStat == MPD_STATUS_STATE_PLAY);
}

gboolean mpdToggle(gpointer thsPtr, gboolean * newState) {
	MKTHIS;
	gboolean oldState = FALSE;
	LOG("Enter mpdToggle");
	if (!mpdAssure(thys, FALSE))
		return FALSE;
	oldState = mpdIsPlaying(thys);
	if (!mpdPlayPause(thys, !oldState))
		return FALSE;
	if (newState)
		*newState = !oldState;
	LOG("Leave mpdToggle");
	return TRUE;
}

gboolean mpdDetach(gpointer thsPtr) {
	MKTHIS;
	LOG("Enter mpdDetach");
	if (thys->player) {
		mpd_disconnect(thys->player);
		mpd_free(thys->player);
		thys->player = NULL;
	}
	if (thys->intervalID) {
		g_source_remove(thys->intervalID);
		thys->intervalID = 0;
	}
	if (thys->xfconfChannel) {
		g_object_unref(thys->xfconfChannel);
	}
	if (!thys->gmpd) {
		g_object_unref(thys->gmpd);
	}
	LOG("Leave mpdDetach");
	return TRUE;
}

gboolean mpdShow(gpointer thsPtr, gboolean newState) {
	MKTHIS;
	LOG("Enter mpdShow");
	const gchar *argv[] = {
		thys->pmanager->str,
		// here we could have arguments
		NULL
	};
	g_spawn_async(NULL, (gchar**)argv, NULL, 
		G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL,
		NULL, NULL, NULL, NULL);
	LOG("Leave mpdShow");
	return thys->bUsePManager;
}

gboolean mpdGetRepeat(gpointer thsPtr) {
	MKTHIS;
	if (thys->player)
		return mpd_player_get_repeat(thys->player);
	return FALSE;
}

gboolean mpdSetRepeat(gpointer thsPtr, gboolean newShuffle) {
	MKTHIS;
	if (mpdAssure(thsPtr, TRUE)) {
		return (MPD_OK ==
			mpd_player_set_repeat(thys->player,
					      (newShuffle) ? 1 : 0));
	}
	return FALSE;
}

gboolean mpdGetShuffle(gpointer thsPtr) {
	MKTHIS;
	if (thys->player)
		return mpd_player_get_random(thys->player);
	return FALSE;
}

gboolean mpdSetShuffle(gpointer thsPtr, gboolean newRandom) {
	MKTHIS;
	if (mpdAssure(thsPtr, TRUE)) {
		return (MPD_OK ==
			mpd_player_set_random(thys->player,
					      (newRandom) ? 1 : 0));
	}
	return FALSE;
}

void mpdCallbackStateChanged(MpdObj * player, ChangedStatusType sType,
			     gpointer thsPtr) {
	MKTHIS;
	if (sType & MPD_CST_SONGID) {
		mpd_Song *song = mpd_playlist_get_current_song(thys->player);
		LOG("Enter mpdCallback: SongChanged");
		if (song != NULL) {
			GString *artLocation = NULL;
			gboolean bFound = FALSE;

			g_string_assign(thys->parent->artist, song->artist);
			g_string_assign(thys->parent->album, song->album);
			g_string_assign(thys->parent->title, song->title);

			//fetching gmpc covers
			artLocation = g_string_new(g_get_home_dir());
			g_string_append_printf(artLocation,
					       "/.covers/%s - %s.jpg",
					       song->artist, song->album);
			LOG("Check 1:'%s'", artLocation->str);
			g_string_truncate(thys->parent->albumArt, 0);

			if (g_file_test(artLocation->str, G_FILE_TEST_EXISTS)) {
				bFound = TRUE;
			} else {
				if (thys->bUseMPDFolder
				    && g_file_test(thys->path->str,
						   G_FILE_TEST_EXISTS)) {
					g_string_printf(artLocation, "%s/%s",
							thys->path->str,
							song->file);
					thys->
					    parent->FindAlbumArtByFilePath
					    (thys->parent->sd,
					     artLocation->str);
				}
			}
			if (bFound) {
				// just assign here, scaling is done in callee
				g_string_assign(thys->parent->albumArt,
						artLocation->str);
				LOG("Found :'%s'", artLocation->str);
			}

			g_string_free(artLocation, TRUE);
			thys->parent->Update(thys->parent->sd, TRUE, estPlay,
					     NULL);
			thys->songID = song->id;
		}
		LOG("Leave mpdCallback: SongChanged");
	}
	if (sType & MPD_CST_STATE) {
		LOG("Enter mpdCallback: StateChanged");
		eSynoptics eStat;
		MpdState state = mpd_player_get_state(thys->player);
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
		thys->parent->Update(thys->parent->sd, FALSE, eStat, NULL);
		LOG("Leave mpdCallback: StateChanged");
	}
	if (sType & MPD_CST_REPEAT) {
		LOG("Enter mpdCallback: RepeatChanged");
		thys->parent->UpdateRepeat(thys->parent->sd,
					   mpd_player_get_repeat(thys->player));
		LOG("Leave mpdCallback: RepeatChanged");
	}
	if (sType & MPD_CST_RANDOM) {
		LOG("Enter mpdCallback: ShuffleChanged");
		thys->parent->UpdateShuffle(thys->parent->sd,
					    mpd_player_get_random
					    (thys->player));
		LOG("Leave mpdCallback: ShuffleChanged");
	}
	// display elapsed/total ?
#if 0
	if (sType & (MPD_CST_ELAPSED_TIME | MPD_CST_TOTAL_TIME)) {
		   if( sType & MPD_CST_ELAPSED_TIME )
			   thys->parent->secPos = 
				   mpd_status_get_elapsed_song_time(thys->player);
		   if( sType & MPD_CST_TOTAL_TIME )
			   thys->parent->secTot = 
			   mpd_status_get_total_song_time(thys->player);
		   thys->parent->UpdateTimePosition(thys->parent->sd);
	}
#endif
	if(sType & (MPD_CST_STORED_PLAYLIST)) {
		mpd_Connection * conn = NULL;
		mpd_InfoEntity * entity = NULL;
		char * ls = "";
		LOG("Enter mpdCallback: PlaylistChanged");
		if(thys->bUseDefault)
			conn = mpd_newConnection("localhost", 6600, 150);
		else
			conn = mpd_newConnection(thys->host->str, thys->port, 1500);
		if(conn) {
			g_hash_table_remove_all(thys->parent->playLists);
			mpd_sendLsInfoCommand(conn,ls);

			while((entity = mpd_getNextInfoEntity(conn))) {
				if(entity->type==
						MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
					mpd_PlaylistFile * pl = entity->info.playlistFile;
					g_hash_table_insert(thys->parent->playLists, 
						g_strdup(pl->path), g_strdup("stock_playlist"));
				}
				mpd_freeInfoEntity(entity);
			}
			thys->parent->UpdatePlaylists(thys->parent->sd);
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
		SPlayer * parent = thys->parent;
		if(thys->bUsePManager && (NULL != g_find_program_in_path(thys->pmanager->str))) {
			MPD_MAP(Show);
		} else {
			NOMAP(Show);
		}
		
		if(mpdAssure(thsPtr, FALSE) && NULL != thys->player) {
			mpdCallbackStateChanged(thys->player, MPD_SQ_ALL, thsPtr);
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
	if (thys->bRequireReconnect )
	{
		LOG("    Reconnecting to %s", thys->host->str);
		thys->bRequireReconnect = FALSE;
		if (thys->player) {
			mpd_disconnect(thys->player);
			mpd_free(thys->player);
			thys->player = NULL;
		}
		mpdAssure(thsPtr, FALSE);
		LOG("    Done reconnect.");
	}
    
    // don't allow bogus playlist manager
    manager = gtk_combo_box_get_active_text(GTK_COMBO_BOX(thys->wPMgr));
    g_string_assign(thys->pmanager, manager);
    g_free(manager);
    if(thys->bUsePManager && thys->pmanager->len) {
        gchar *binPath = g_find_program_in_path(thys->pmanager->str);
        if(NULL == binPath) {
             GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_CLOSE,
                                              _("Can't find binary '%s'"),
                                              thys->pmanager->str);
            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
            gtk_widget_grab_focus(thys->wPMgr);
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
	thys->bUseDefault = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(thys->wHost, !thys->bUseDefault);
	gtk_widget_set_sensitive(thys->wPort, !thys->bUseDefault);
	gtk_widget_set_sensitive(thys->wPass, !thys->bUseDefault);
	thys->bRequireReconnect = TRUE;
	LOG("Leave on_chkUseDefault_toggled");
}

EXPORT void on_entryHost_changed(GtkEntry *tbd, gpointer thsPtr) {
	MKTHIS;
	g_string_assign(thys->host, gtk_entry_get_text(tbd));
	thys->bRequireReconnect = TRUE;
}

EXPORT void on_entryPassword_changed(GtkEntry *tbd, gpointer thsPtr) {
	MKTHIS;
	g_string_assign(thys->pass, gtk_entry_get_text(tbd));
	thys->bRequireReconnect = TRUE;
}

EXPORT void on_chkUseFolder_toggled(GtkToggleButton * tb, gpointer thsPtr) {
	MKTHIS;
	LOG("Enter on_chkUseFolder_toggled");
	thys->bUseMPDFolder = gtk_toggle_button_get_active(tb);
	thys->bRequireReconnect = TRUE;
	gtk_widget_set_sensitive(thys->wPath, thys->bUseMPDFolder);
	LOG("Leave on_chkUseFolder_toggled");
}

EXPORT void on_spinPort_value_changed(GtkSpinButton *tbd, gpointer thsPtr) {
	MKTHIS;
	thys->port = (guint)gtk_spin_button_get_value_as_int(tbd);
	thys->bRequireReconnect = TRUE;
}

EXPORT void on_chooserDirectory_current_folder_changed(GtkFileChooserButton *chb, gpointer thsPtr) {
	MKTHIS;
	gchar *text = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chb));
	LOG("Enter on_chooserDirectory_current_folder_changed: %s", text);
	xfconf_channel_set_string(thys->xfconfChannel, "/MusicFolder", text);
	thys->bRequireReconnect = TRUE;
	LOG("Leave on_chooserDirectory_current_folder_changed");
}

EXPORT void on_chkUseListManager_toggled(GtkToggleButton * tb, gpointer thsPtr) {
	LOG("Enter on_chkUseListManager_toggled");
	MKTHIS;
	thys->bUsePManager = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(thys->wPMgr, thys->bUsePManager);
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
	GError *error = NULL;
	XfceHeading *heading = NULL;
	if(!gtk_builder_add_from_file(builder, BACKENDDIR "/" BASENAME "/settings-mpd.ui", &error)){
		LOG("mpdConfigure unexpected:\n %s", error->message);
		return;
	}
		
	
	thys->wDlg = GTK_WIDGET(gtk_builder_get_object(builder, "mpdSettings"));
	thys->wHost = GTK_WIDGET(gtk_builder_get_object(builder, "entryHost"));
	thys->wPort = GTK_WIDGET(gtk_builder_get_object(builder, "spinPort"));
	thys->wPass = GTK_WIDGET(gtk_builder_get_object(builder, "entryPassword"));
	thys->wPMgr = GTK_WIDGET(gtk_builder_get_object(builder, "comboListManager"));
	thys->wPath = GTK_WIDGET(gtk_builder_get_object(builder, "chooserDirectory"));

	header = GTK_WIDGET(gtk_builder_get_object(builder, "xfce-heading1"));
	xfce_heading_set_icon(XFCE_HEADING(header), MPD_icon());

	gtk_builder_connect_signals(builder,thsPtr);

  	xfconf_g_property_bind (thys->xfconfChannel, "/UseDefault", G_TYPE_BOOLEAN,
                          gtk_builder_get_object(builder, "chkUseDefault"), "active");
  	xfconf_g_property_bind (thys->xfconfChannel, "/Host", G_TYPE_STRING,
                          gtk_builder_get_object(builder, "entryHost"), "text");
  	xfconf_g_property_bind (thys->xfconfChannel, "/Port", G_TYPE_UINT,
                          gtk_builder_get_object(builder, "spinPort"), "value");
  	xfconf_g_property_bind (thys->xfconfChannel, "/Password", G_TYPE_STRING,
                          gtk_builder_get_object(builder, "entryPassword"), "text");
  	
  	xfconf_g_property_bind (thys->xfconfChannel, "/UseFolder", G_TYPE_BOOLEAN,
                          gtk_builder_get_object(builder, "chkUseFolder"), "active");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(thys->wPath), 
		xfconf_channel_get_string(thys->xfconfChannel, "/MusicFolder", "~"));
  	
  	xfconf_g_property_bind (thys->xfconfChannel, "/UseListManager", G_TYPE_BOOLEAN,
                          gtk_builder_get_object(builder, "chkUseListManager"), "active");
  	xfconf_g_property_bind (thys->xfconfChannel, "/ListManager", G_TYPE_STRING,
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
            if(thys->pmanager->len)
                g_hash_table_insert(table, thys->pmanager->str, g_strdup("!"));
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
	thys->bRequireReconnect = FALSE;
	//gtk_dialog_run
	gtk_window_set_transient_for(GTK_WINDOW(thys->wDlg), GTK_WINDOW(parent));
	result = gtk_dialog_run (GTK_DIALOG (thys->wDlg));
	xfconf_g_property_unbind_all(thys->xfconfChannel);

	LOG("Leave mpdConfigure");
}

gpointer MPD_attach(SPlayer * parent) {
	mpdData *thys = g_new0(mpdData, 1);
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

	thys->host = g_string_new("");
	thys->pass = g_string_new("");
	thys->path = g_string_new("");
	thys->pmanager = g_string_new("");
    
	// always assign
	thys->parent = parent;
	thys->wDlg = NULL;

	// force our own update rate
	if (parent->updateRateMS < 1000)
		parent->updateRateMS = 1000;

	// establish the callback function
	thys->intervalID =
		g_timeout_add(thys->parent->updateRateMS, mpdCallback, thys);
		
	// initialize properties

    thys->xfconfChannel = xfconf_channel_new_with_property_base (
    	"xfce4-panel", "/plugins/squeezebox/backends/mpd");
	thys->bUseDefault = xfconf_channel_get_bool(thys->xfconfChannel, "/UseDefault", TRUE);
	thys->host->str = xfconf_channel_get_string(thys->xfconfChannel, "/Host", "localhost");
	thys->port = xfconf_channel_get_uint(thys->xfconfChannel, "/Port", 6600);
	thys->pass->str = xfconf_channel_get_string(thys->xfconfChannel, "/Password", "");
	thys->bUseMPDFolder = xfconf_channel_get_bool(thys->xfconfChannel, "/UseFolder", TRUE);
	thys->path->str = xfconf_channel_get_string(thys->xfconfChannel, "/MusicFolder", "~");
	thys->bUsePManager = xfconf_channel_get_bool(thys->xfconfChannel, "/UseListManager", TRUE);
	thys->pmanager->str = xfconf_channel_get_string(thys->xfconfChannel, "/ListManager", "");
	

	LOG("Leave MPD_attach:\n"
		" UseDefault: %d\n"
		" Host:       %s\n"
		" Port:       %d\n"
		" UseFolder:  %d\n"
		" Path:       %s\n"
		" UseManager: %d\n"
		" Manager:    %s",
		thys->bUseDefault,
		thys->host->str,
		thys->port,
		thys->bUseMPDFolder,
		thys->path->str,
		thys->bUsePManager,
		thys->pmanager->str);
	return thys;
}
#endif
