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

#include <libxfce4ui/libxfce4ui.h>

// standalone glib mpd
#include "gmpd.h"

#define MPD_MAP(a) parent->a = mpd##a
typedef struct mpdData
{
   SPlayer *parent;
   guint port;
   GString *host;
   GString *pass;
   GString *path;
   GString *pmanager;
   GMpd *gmpd;
   gboolean bUseDefault;
   gboolean bUseMPDFolder;
   gboolean bUsePManager;
   gboolean bAutoConnect;
   gboolean bRequireReconnect;
   gint intervalID;
   gint songID;
   gint songLength;
   eSynoptics state;
   GtkWidget *wHost, *wPass, *wPort, *wDlg, *wPath, *wPMgr;
   XfconfChannel *xfconfChannel;
} mpdData;

#define MKTHIS mpdData *thys = (mpdData *)thsPtr

gpointer MPD_attach(SPlayer * player);
void on_mpdSettings_response(GtkDialog * dlg, int reponse, gpointer thsPtr);
void on_chkUseDefault_toggled(GtkToggleButton * tb, gpointer thsPtr);
void on_entryHost_changed(GtkEntry *tbd, gpointer thsPtr);
void on_entryPassword_changed(GtkEntry *tbd, gpointer thsPtr);
void on_chkUseFolder_toggled(GtkToggleButton * tb, gpointer thsPtr);
void on_spinPort_value_changed(GtkSpinButton *tbd, gpointer thsPtr);
void on_chooserDirectory_current_folder_changed(GtkFileChooserButton *chb,
      gpointer thsPtr);
void on_chkUseListManager_toggled(GtkToggleButton * tb, gpointer thsPtr);

#define BASENAME "mpd"
DEFINE_NETWORK_BACKEND(MPD, _("Music Player Daemon"))

// implementation
static void _convert_state(GMpd *gmpd, mpdData *thys)
{
   GHashTable *state_info = g_mpd_get_state_info(thys->gmpd);
   gchar *state = g_hash_table_lookup(state_info, "state");
   if (!g_mpd_is_online(thys->gmpd))
      thys->state = estErr;
   else if (!g_strcmp0(state, "play"))
      thys->state = estPlay;
   else if (!g_strcmp0(state, "pause"))
      thys->state = estPause;
   else if (!g_strcmp0(state, "stop"))
      thys->state = estStop;
   else
      thys->state = estErr;

}

static void on_gmpd_song_changed(GMpd *gmpd, gpointer thsPtr)
{
   MKTHIS;
   GHashTable *song_info = g_mpd_get_current_track(thys->gmpd);

   g_string_assign(thys->parent->artist,
         g_hash_table_lookup(song_info, "Artist"));
   g_string_assign(thys->parent->album,
         g_hash_table_lookup(song_info, "Album"));
   g_string_assign(thys->parent->title,
         g_hash_table_lookup(song_info, "Title"));

   if (thys->bUseMPDFolder && g_file_test(thys->path->str, G_FILE_TEST_EXISTS))
   {
      GString *artLocation = g_string_new("");
      g_string_printf(artLocation, "%s/%s", thys->path->str,
            (gchar*) g_hash_table_lookup(song_info, "file"));
      thys->parent->FindAlbumArtByFilePath(thys->parent->sd, artLocation->str);
      g_string_free(artLocation, TRUE);
   }
   _convert_state(gmpd, thys);
   thys->parent->Update(thys->parent->sd, TRUE, thys->state, NULL);
}

static void on_gmpd_status_changed(GMpd *gmpd, gpointer thsPtr)
{
   MKTHIS;
   _convert_state(gmpd, thys);
   thys->parent->Update(thys->parent->sd, FALSE, thys->state, NULL);
}

static void _clone_playlists(gpointer key, gpointer value, gpointer thsPtr)
{
   LOG("Playlist %s", (gchar*)key);
   g_hash_table_insert(thsPtr, g_strdup(key), g_strdup(value));
}

static void on_gmpd_playlist_changed(GMpd *gmpd, gpointer thsPtr)
{
   MKTHIS;

   g_hash_table_remove_all(thys->parent->playLists);
   g_hash_table_foreach(g_mpd_get_playlists(thys->gmpd), _clone_playlists,
         thys->parent->playLists);

   thys->parent->UpdatePlaylists(thys->parent->sd);
}

static gboolean mpdAssure(gpointer thsPtr, gboolean noCreate)
{

   MKTHIS;
   gboolean gConnect = TRUE;
   LOG("Enter mpdAssure");
   if (!thys->gmpd)
   {
      thys->gmpd = g_mpd_new();
      g_signal_connect(thys->gmpd, "song-changed",
            G_CALLBACK(on_gmpd_song_changed), thsPtr);
      g_signal_connect(thys->gmpd, "status-changed",
            G_CALLBACK(on_gmpd_status_changed), thsPtr);
      g_signal_connect(thys->gmpd, "playlist-changed",
            G_CALLBACK(on_gmpd_playlist_changed), thsPtr);
   }
   if (g_mpd_is_online(thys->gmpd))
   {
      gConnect = TRUE;
   }
   else
   {
      if (thys->bUseDefault)
         gConnect = g_mpd_connect(thys->gmpd, "localhost", 6600);
      else
         gConnect = g_mpd_connect(thys->gmpd, thys->host->str, thys->port);
   }

   LOG("Leave mpdAssure %d", gConnect);
   return gConnect;
}

static gboolean mpdNext(gpointer thsPtr)
{
   MKTHIS;
   gboolean bRet = FALSE;
   LOG("Enter mpdNext");
   if (!mpdAssure(thys, TRUE))
      return FALSE;
   else
      bRet = g_mpd_next(thys->gmpd);
   LOG("Leave mpdNext");
   return bRet;
}

static gboolean mpdPrevious(gpointer thsPtr)
{
   MKTHIS;
   gboolean bRet;
   LOG("Enter mpdPrevious");
   if (!mpdAssure(thys, TRUE))
      bRet = FALSE;
   else
      bRet = g_mpd_previous(thys->gmpd);
   LOG("Leave mpdPrevious");
   return bRet;
}

static gboolean mpdStop(gpointer thsPtr)
{
   MKTHIS;
   gboolean bRet;
   LOG("Enter mpdStop");
   if (!mpdAssure(thys, TRUE))
      bRet = FALSE;
   else
      bRet = g_mpd_stop(thys->gmpd);
   LOG("Leave mpdStop");
   return bRet;
}

static gboolean mpdPlayPause(gpointer thsPtr, gboolean newState)
{
   MKTHIS;
   gboolean bRet = FALSE;
   LOG("Enter mpdPlayPause %d", newState);
   if (!mpdAssure(thys, FALSE))
      bRet = FALSE;
   else if (newState)
      bRet = g_mpd_play(thys->gmpd);
   else
      bRet = g_mpd_pause(thys->gmpd);
   LOG("Leave mpdPlayPause");
   return bRet;
}

static gboolean mpdPlayPlaylist(gpointer thsPtr, gchar *playlistName)
{
   MKTHIS;
   gboolean bRet = FALSE;
   gboolean bPlaying = g_mpd_is_playing(thys->gmpd);
   LOG("Enter mpdPlayPlaylist");
   bRet = g_mpd_switch_playlist(thys->gmpd, playlistName);
   if (bPlaying)
      bRet &= g_mpd_play(thys->gmpd);
   LOG("Leave mpdPlayPlaylist");
   return bRet;
}

static gboolean mpdIsPlaying(gpointer thsPtr)
{
   MKTHIS;
   gboolean bRet = FALSE;
   LOG("Enter mpdIsPlaying");
   if (!mpdAssure(thys, TRUE))
      bRet = FALSE;
   else
      bRet = g_mpd_is_playing(thys->gmpd);
   LOG("Leave mpdIsPlaying");
   return bRet;
}

static gboolean mpdToggle(gpointer thsPtr, gboolean * newState)
{
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
   LOG("Leave mpdToggle %d", oldState);
   return TRUE;
}

static gboolean mpdDetach(gpointer thsPtr)
{
   MKTHIS;
   LOG("Enter mpdDetach");
   if (thys->xfconfChannel)
   {
      g_object_unref(thys->xfconfChannel);
   }
   if (thys->gmpd)
   {
      g_object_unref(thys->gmpd);
      thys->gmpd = NULL;
   }
   LOG("Leave mpdDetach");
   return TRUE;
}

static gboolean mpdShow(gpointer thsPtr, gboolean newState)
{
   MKTHIS;
   const gchar *argv[] =
   { thys->pmanager->str,
   // here we could have arguments
         NULL };
   LOG("Enter mpdShow");
   g_spawn_async(NULL, (gchar**) argv, NULL,
         G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL
               | G_SPAWN_STDERR_TO_DEV_NULL,
         NULL, NULL, NULL, NULL);
   LOG("Leave mpdShow");
   return thys->bUsePManager;
}

static gboolean mpdGetRepeat(gpointer thsPtr)
{
   MKTHIS;
   return g_mpd_get_repeat(thys->gmpd);
}

static gboolean mpdSetRepeat(gpointer thsPtr, gboolean newShuffle)
{
   MKTHIS;
   if (mpdAssure(thsPtr, TRUE))
   {
      return g_mpd_set_repeat(thys->gmpd, newShuffle);
   }
   return FALSE;
}

static gboolean mpdGetShuffle(gpointer thsPtr)
{
   MKTHIS;
   return g_mpd_get_random(thys->gmpd);
}

static gboolean mpdSetShuffle(gpointer thsPtr, gboolean newRandom)
{
   MKTHIS;
   if (mpdAssure(thsPtr, TRUE))
   {
      return g_mpd_set_random(thys->gmpd, newRandom);
   }
   return FALSE;
}

static void mpdPersist(gpointer thsPtr, gboolean bIsStoring)
{
   MKTHIS;
   LOG("Enter mpdPersist");
   if (bIsStoring)
   {
      // nothing to do
   }
   else
   {
      SPlayer * parent = thys->parent;
      if (thys->bUsePManager
            && (NULL != g_find_program_in_path(thys->pmanager->str)))
      {
         MPD_MAP(Show);
      }
      else
      {
         NOMAP(Show);
      }
   }
   mpdAssure(thsPtr, FALSE);
   LOG("Leave mpdPersist");
}

EXPORT void on_mpdSettings_response(GtkDialog * dlg, int reponse,
      gpointer thsPtr)
{
   MKTHIS;
   gchar *manager = NULL;
   LOG("Enter on_mpdSettings_response");

   // reconnect if changed
   if (thys->bRequireReconnect)
   {
      LOG("    Reconnecting to %s", thys->host->str);
      thys->bRequireReconnect = FALSE;
      if (thys->gmpd)
      {
         g_object_unref(thys->gmpd);
         thys->gmpd = NULL;
      }
      mpdAssure(thsPtr, FALSE);
      LOG("    Done reconnect.");
   }

   // don't allow bogus playlist manager
   manager = gtk_combo_box_get_active_text(GTK_COMBO_BOX(thys->wPMgr));
   g_string_assign(thys->pmanager, manager);
   g_free(manager);
   if (thys->bUsePManager && thys->pmanager->len)
   {
      gchar *binPath = g_find_program_in_path(thys->pmanager->str);
      if (NULL == binPath)
      {
         GtkWidget *dialog = gtk_message_dialog_new(NULL,
               GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
               GTK_BUTTONS_CLOSE, _("Can't find binary '%s'"),
               thys->pmanager->str);
         gtk_dialog_run(GTK_DIALOG(dialog));
         gtk_widget_destroy(dialog);
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

EXPORT void on_chkUseDefault_toggled(GtkToggleButton * tb, gpointer thsPtr)
{
   MKTHIS;
   LOG("Enter on_chkUseDefault_toggled");
   thys->bUseDefault = gtk_toggle_button_get_active(tb);
   gtk_widget_set_sensitive(thys->wHost, !thys->bUseDefault);
   gtk_widget_set_sensitive(thys->wPort, !thys->bUseDefault);
   gtk_widget_set_sensitive(thys->wPass, !thys->bUseDefault);
   thys->bRequireReconnect = TRUE;
   LOG("Leave on_chkUseDefault_toggled");
}

EXPORT void on_entryHost_changed(GtkEntry *tbd, gpointer thsPtr)
{
   MKTHIS;
   g_string_assign(thys->host, gtk_entry_get_text(tbd));
   thys->bRequireReconnect = TRUE;
}

EXPORT void on_entryPassword_changed(GtkEntry *tbd, gpointer thsPtr)
{
   MKTHIS;
   g_string_assign(thys->pass, gtk_entry_get_text(tbd));
   thys->bRequireReconnect = TRUE;
}

EXPORT void on_chkUseFolder_toggled(GtkToggleButton * tb, gpointer thsPtr)
{
   MKTHIS;
   LOG("Enter on_chkUseFolder_toggled");
   thys->bUseMPDFolder = gtk_toggle_button_get_active(tb);
   thys->bRequireReconnect = TRUE;
   gtk_widget_set_sensitive(thys->wPath, thys->bUseMPDFolder);
   LOG("Leave on_chkUseFolder_toggled");
}

EXPORT void on_spinPort_value_changed(GtkSpinButton *tbd, gpointer thsPtr)
{
   MKTHIS;
   thys->port = (guint) gtk_spin_button_get_value_as_int(tbd);
   thys->bRequireReconnect = TRUE;
}

EXPORT void on_chooserDirectory_current_folder_changed(
      GtkFileChooserButton *chb, gpointer thsPtr)
{
   MKTHIS;
   gchar *text = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chb));
   LOG("Enter on_chooserDirectory_current_folder_changed: %s", text);
   xfconf_channel_set_string(thys->xfconfChannel, "/MusicFolder", text);
   thys->bRequireReconnect = TRUE;
   LOG("Leave on_chooserDirectory_current_folder_changed");
}

EXPORT void on_chkUseListManager_toggled(GtkToggleButton * tb, gpointer thsPtr)
{
   MKTHIS;
   LOG("Enter on_chkUseListManager_toggled");
   thys->bUsePManager = gtk_toggle_button_get_active(tb);
   gtk_widget_set_sensitive(thys->wPMgr, thys->bUsePManager);
   LOG("Leave on_chkUseListManager_toggled");
}

static void mpdConfigure(gpointer thsPtr, GtkWidget * parent)
{
   MKTHIS;
   GtkListStore *store;
   GtkTreeIter iter =
   { 0 };
   GtkBuilder* builder = gtk_builder_new();
   GError *error = NULL;
   LOG("Enter mpdConfigure");
   if (!gtk_builder_add_from_file(builder,
   BACKENDDIR "/" BASENAME "/settings-mpd.ui", &error))
   {
      LOG("mpdConfigure unexpected:\n %s", error->message);
      return;
   }

   thys->wDlg = GTK_WIDGET(gtk_builder_get_object(builder, "mpdSettings"));
   thys->wHost = GTK_WIDGET(gtk_builder_get_object(builder, "entryHost"));
   thys->wPort = GTK_WIDGET(gtk_builder_get_object(builder, "spinPort"));
   thys->wPass = GTK_WIDGET(gtk_builder_get_object(builder, "entryPassword"));
   thys->wPMgr = GTK_WIDGET(
         gtk_builder_get_object(builder, "comboListManager"));
   thys->wPath = GTK_WIDGET(
         gtk_builder_get_object(builder, "chooserDirectory"));

   gtk_builder_connect_signals(builder, thsPtr);

   xfconf_g_property_bind(thys->xfconfChannel, "/UseDefault", G_TYPE_BOOLEAN,
         gtk_builder_get_object(builder, "chkUseDefault"), "active");
   xfconf_g_property_bind(thys->xfconfChannel, "/Host", G_TYPE_STRING,
         gtk_builder_get_object(builder, "entryHost"), "text");
   xfconf_g_property_bind(thys->xfconfChannel, "/Port", G_TYPE_UINT,
         gtk_builder_get_object(builder, "spinPort"), "value");
   xfconf_g_property_bind(thys->xfconfChannel, "/Password", G_TYPE_STRING,
         gtk_builder_get_object(builder, "entryPassword"), "text");

   xfconf_g_property_bind(thys->xfconfChannel, "/UseFolder", G_TYPE_BOOLEAN,
         gtk_builder_get_object(builder, "chkUseFolder"), "active");
   gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(thys->wPath),
         xfconf_channel_get_string(thys->xfconfChannel, "/MusicFolder", "~"));

   xfconf_g_property_bind(thys->xfconfChannel, "/UseListManager",
   G_TYPE_BOOLEAN, gtk_builder_get_object(builder, "chkUseListManager"),
         "active");
   xfconf_g_property_bind(thys->xfconfChannel, "/ListManager", G_TYPE_STRING,
         gtk_bin_get_child(
               GTK_BIN(
                     GTK_COMBO_BOX_ENTRY( gtk_builder_get_object(builder, "comboListManager")))),
         "text");
   store = GTK_LIST_STORE(gtk_builder_get_object(builder, "listManagers"));

   {
      // cheapo per-name search
      /* TODO: make it work with GAppInfo */
      gchar *suspects[] =
      { "ario", "glurp", "gmpc", "mpdcon.app", "sonata", "qmpdclient", NULL };
      gchar **suspect;
      GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
            g_free);

      if (thys->pmanager->len)
         g_hash_table_insert(table, thys->pmanager->str, g_strdup("!"));

      suspect = suspects;
      do
      {
         gchar *prg = g_find_program_in_path(*suspect);
         if (prg)
         {
            if (!g_hash_table_lookup_extended(table, *suspect, NULL, NULL))
            {
               LOG("\tFound: %s", *suspect);
               gtk_list_store_append(store, &iter);
               gtk_list_store_set(store, &iter, 0, *suspect, -1);
               g_hash_table_insert(table, (gchar*) *suspect, g_strdup("."));
            }
         }
      } while (*suspect++);
   }

   thys->bRequireReconnect = FALSE;
   //gtk_dialog_run
   gtk_window_set_transient_for(GTK_WINDOW(thys->wDlg), GTK_WINDOW(parent));
   gtk_dialog_run(GTK_DIALOG(thys->wDlg));
   xfconf_g_property_unbind_all(thys->xfconfChannel);

   LOG("Leave mpdConfigure");
}

gpointer MPD_attach(SPlayer * parent)
{
   mpdData *thys = g_new0(mpdData, 1);
   LOG("Enter MPD_attach");
   MPD_MAP(Assure);
   MPD_MAP(Next);
   MPD_MAP(Previous);
   MPD_MAP(PlayPause);
   MPD_MAP(PlayPlaylist);
   MPD_MAP(IsPlaying);
   MPD_MAP(Toggle);
   MPD_MAP(Stop);
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

// initialize properties

   thys->xfconfChannel = xfconf_channel_new_with_property_base("xfce4-panel",
         "/plugins/squeezebox/backends/mpd");
   thys->bUseDefault = xfconf_channel_get_bool(thys->xfconfChannel,
         "/UseDefault",
         TRUE);
   thys->host->str = xfconf_channel_get_string(thys->xfconfChannel, "/Host",
         "localhost");
   thys->port = xfconf_channel_get_uint(thys->xfconfChannel, "/Port", 6600);
   thys->pass->str = xfconf_channel_get_string(thys->xfconfChannel, "/Password",
         "");
   thys->bUseMPDFolder = xfconf_channel_get_bool(thys->xfconfChannel,
         "/UseFolder",
         TRUE);
   thys->path->str = xfconf_channel_get_string(thys->xfconfChannel,
         "/MusicFolder", "~");
   thys->bUsePManager = xfconf_channel_get_bool(thys->xfconfChannel,
         "/UseListManager", TRUE);
   thys->pmanager->str = xfconf_channel_get_string(thys->xfconfChannel,
         "/ListManager", "");

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
