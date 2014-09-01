/***************************************************************************
 *            squeezebox-radiotray.c
 *
 *  Fri Aug 29 17:08:15 2014
 *  Copyright  2014  Hakan Erduman
 *  Email hakan@erduman.de
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
#ifdef HAVE_BACKEND_RADIOTRAY

// default
#include "squeezebox.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <glib/gstdio.h>
#include "radiotray-player-binding.h"

#ifndef DBUS_TYPE_G_STRING_STRING_HASHTABLE
#define DBUS_TYPE_G_STRING_STRING_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_STRING))
#endif

/* --- */
#define RT_MAP(a) parent->a = rt##a;
#define MKTHIS rtData *this = (rtData *)thsPtr

typedef struct rtData
{
   SPlayer *parent;
   gpointer *player;
   gchar *bin;
   DBusGProxy *rtPlayer;
   guint intervalID;
   gboolean isPlaying;
} rtData;

void *RT_attach(SPlayer * parent);
#define BASENAME "radiotray"
DEFINE_DBUS_BACKEND(RT, _("RadioTray"), "net.sourceforge.radiotray",
      "radiotray")

// implementation

static gint rtCallbackFake(gpointer thsPtr)
{
   MKTHIS;
   static gboolean inTimer = FALSE;

   gchar *val = NULL;
   gboolean bStateChanged = FALSE;
   gboolean bSongChanged = FALSE;
   eSynoptics eStat = estErr;

   if (!inTimer)
   {
      inTimer = TRUE;

      if (radiotray_get_current_radio(this->rtPlayer, &val, NULL))
      {
         //LOG("Radio: %s", val);
         if (val)
         {
            gboolean oldPlaying = this->isPlaying;
            this->isPlaying = !g_str_has_suffix(val, "(not playing)");
            bStateChanged = oldPlaying != this->isPlaying;
            eStat = this->isPlaying ? estPlay : estStop;
            g_string_assign(this->parent->artist, val);
         }
      }

      if (radiotray_get_current_meta_data(this->rtPlayer, &val, NULL))
      {
         //LOG("Meta: %s", val);
         if (val)
         {
            if (!g_str_equal(this->parent->title->str, val))
            {
               g_string_assign(this->parent->title, val);
               bSongChanged = TRUE;
            }
         }
      }

      if (bStateChanged || bSongChanged)
      {
         this->parent->Update(this->parent->sd, bSongChanged, eStat, val);
      }

      inTimer = FALSE;
   }
   return TRUE;
}

static void rtAsyncRetrurn(DBusGProxy *proxy, GError *error, gpointer userdata)
{

}

static gboolean rtIsPlaying(gpointer thsPtr)
{
   MKTHIS;
   return this->isPlaying;
}

static gboolean rtAssure(gpointer thsPtr, gboolean noCreate)
{
   MKTHIS;
   char **radioes = NULL;

   LOG("Enter rtAssure");

   if (this->parent->bus && !this->rtPlayer)
   {
      GError *error = NULL;
      this->rtPlayer = dbus_g_proxy_new_for_name_owner(this->parent->bus,
            "net.sourceforge.radiotray", "/net/sourceforge/radiotray",
            "net.sourceforge.radiotray", &error);
      if (error)
      {
         LOGWARN("\tCouldn't connect to shell proxy '%s' ",
               error->message);
         g_error_free(error);
         error = NULL;
         if (!noCreate)
            this->parent->StartService(this->parent->sd,
                  "net.sourceforge.radiotray");
      }
      if (this->rtPlayer)
      {
         // state & song changes faked since no event
         this->intervalID = g_timeout_add(1500, rtCallbackFake, this);

         // read radio stations only once and disguise them as playlists
         if (radiotray_list_radios(this->rtPlayer, &radioes, NULL))
         {
            while (*radioes)
            {
               LOG("Have radio: %s", *radioes);
               g_hash_table_insert(this->parent->playLists, g_strdup(*radioes),
                     g_strdup("radiotray"));
               radioes++;
            }
            this->parent->UpdatePlaylists(this->parent->sd);
         }
      }
   }
   LOG("Leave rtAssure");
   return (NULL != this->rtPlayer);
}

static gboolean rtPlayPause(gpointer thsPtr, gboolean newState)
{
   MKTHIS;
   gboolean bRet = FALSE;
   LOG("Enter rtPlayPause");
   if (rtAssure(this, FALSE))
   {
      radiotray_toggle_play_async(this->rtPlayer, rtAsyncRetrurn, this);
      bRet = TRUE;
   }
   LOG("LEAVE rtPlayPause");
   return bRet;
}

static gboolean rtToggle(gpointer thsPtr, gboolean * newState)
{
   MKTHIS;
   gboolean bRet = FALSE;
   LOG("Enter rtToggle");
   if (rtAssure(this, FALSE))
   {
      radiotray_toggle_play_async(this->rtPlayer, rtAsyncRetrurn, this);
      bRet = TRUE;
   }
   LOG("Leave rtToggle");
   return bRet;
}

static gboolean rtPlayPlaylist(gpointer thsPtr, gchar *playlistName)
{
   MKTHIS;
   gboolean bRet = FALSE;
   LOG("Enter rtPlayPlaylist");
   if (rtAssure(this, FALSE))
   {
      GValue val;
      g_value_init(&val, G_TYPE_STRING);
      g_value_set_string(&val, playlistName);
      bRet = radiotray_play_radio(this->rtPlayer, &val, NULL);
   }
   LOG("Leave rtPlayPlaylist");
   return bRet;
}

static gboolean rtDetach(gpointer thsPtr)
{
   MKTHIS;
   gboolean bRet = FALSE;
   LOG("Enter rtDetach");
   if (this->rtPlayer)
   {
      g_object_unref(G_OBJECT(this->rtPlayer));
      this->rtPlayer = NULL;
   }
   if (this->intervalID)
   {
      g_source_remove(this->intervalID);
      this->intervalID = 0;
   }
   LOG("Leave rtDetach");
   return bRet;
}

static gboolean rtUpdateDBUS(gpointer thsPtr, const gchar *name,
      gboolean appeared)
{
   MKTHIS;
   if (appeared)
   {
      LOG("radiotray has started");
      rtAssure(thsPtr, FALSE);
   }
   else
   {
      LOG("radiotray has died");
      if (this->rtPlayer)
      {
         g_object_unref(G_OBJECT(this->rtPlayer));
         this->rtPlayer = NULL;
      }
      g_string_truncate(this->parent->artist, 0);
      g_string_truncate(this->parent->album, 0);
      g_string_truncate(this->parent->title, 0);
      g_string_truncate(this->parent->albumArt, 0);
      this->parent->Update(this->parent->sd, TRUE, estStop, NULL);
   }
   return TRUE;
}

void *RT_attach(SPlayer * parent)
{
   rtData *this = g_new0(rtData, 1);
   LOG("Enter RT_attach");

   RT_MAP(Assure);
   NOMAP(Next);
   NOMAP(Previous);
   RT_MAP(PlayPause);
   RT_MAP(PlayPlaylist);
   RT_MAP(IsPlaying);
   RT_MAP(Toggle);
   RT_MAP(Detach);
   NOMAP(Persist);
   NOMAP(Configure);
   NOMAP(IsVisible);
   NOMAP(Show);
   RT_MAP(UpdateDBUS);
   NOMAP(GetRepeat);
   NOMAP(SetRepeat);
   NOMAP(GetShuffle);
   NOMAP(SetShuffle);
   NOMAP(UpdateWindow);

   // we init default values
   this->parent = parent;
   this->bin = g_find_program_in_path("radiotray");

   LOG("Leave RT_attach");
   return this;
}
#endif
