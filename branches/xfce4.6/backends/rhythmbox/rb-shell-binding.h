/* Generated by dbus-binding-tool; do not edit! */

#include <glib/gtypes.h>
#include <glib/gerror.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_org_gnome_Rhythmbox_Shell
#define DBUS_GLIB_CLIENT_WRAPPERS_org_gnome_Rhythmbox_Shell

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_load_ur_i (DBusGProxy *proxy, const char * IN_arg0, const gboolean IN_arg1, GError **error)

{
  return dbus_g_proxy_call (proxy, "loadURI", error, G_TYPE_STRING, IN_arg0, G_TYPE_BOOLEAN, IN_arg1, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_load_ur_i_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_load_ur_i_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_load_ur_i_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_load_ur_i_async (DBusGProxy *proxy, const char * IN_arg0, const gboolean IN_arg1, org_gnome_Rhythmbox_Shell_load_ur_i_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "loadURI", org_gnome_Rhythmbox_Shell_load_ur_i_async_callback, stuff, g_free, G_TYPE_STRING, IN_arg0, G_TYPE_BOOLEAN, IN_arg1, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_get_player (DBusGProxy *proxy, char** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "getPlayer", error, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_get_player_reply) (DBusGProxy *proxy, char *OUT_arg0, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_get_player_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  char* OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, DBUS_TYPE_G_OBJECT_PATH, &OUT_arg0, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_get_player_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_get_player_async (DBusGProxy *proxy, org_gnome_Rhythmbox_Shell_get_player_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "getPlayer", org_gnome_Rhythmbox_Shell_get_player_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_get_playlist_manager (DBusGProxy *proxy, char** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "getPlaylistManager", error, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_get_playlist_manager_reply) (DBusGProxy *proxy, char *OUT_arg0, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_get_playlist_manager_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  char* OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, DBUS_TYPE_G_OBJECT_PATH, &OUT_arg0, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_get_playlist_manager_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_get_playlist_manager_async (DBusGProxy *proxy, org_gnome_Rhythmbox_Shell_get_playlist_manager_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "getPlaylistManager", org_gnome_Rhythmbox_Shell_get_playlist_manager_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_present (DBusGProxy *proxy, const guint IN_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "present", error, G_TYPE_UINT, IN_arg0, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_present_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_present_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_present_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_present_async (DBusGProxy *proxy, const guint IN_arg0, org_gnome_Rhythmbox_Shell_present_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "present", org_gnome_Rhythmbox_Shell_present_async_callback, stuff, g_free, G_TYPE_UINT, IN_arg0, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_get_song_properties (DBusGProxy *proxy, const char * IN_uri, GHashTable** OUT_arg1, GError **error)

{
  return dbus_g_proxy_call (proxy, "getSongProperties", error, G_TYPE_STRING, IN_uri, G_TYPE_INVALID, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), OUT_arg1, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_get_song_properties_reply) (DBusGProxy *proxy, GHashTable *OUT_arg1, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_get_song_properties_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  GHashTable* OUT_arg1;
  dbus_g_proxy_end_call (proxy, call, &error, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), &OUT_arg1, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_get_song_properties_reply)data->cb) (proxy, OUT_arg1, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_get_song_properties_async (DBusGProxy *proxy, const char * IN_uri, org_gnome_Rhythmbox_Shell_get_song_properties_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "getSongProperties", org_gnome_Rhythmbox_Shell_get_song_properties_async_callback, stuff, g_free, G_TYPE_STRING, IN_uri, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_set_song_property (DBusGProxy *proxy, const char * IN_uri, const char * IN_propname, const GValue* IN_value, GError **error)

{
  return dbus_g_proxy_call (proxy, "setSongProperty", error, G_TYPE_STRING, IN_uri, G_TYPE_STRING, IN_propname, G_TYPE_VALUE, IN_value, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_set_song_property_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_set_song_property_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_set_song_property_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_set_song_property_async (DBusGProxy *proxy, const char * IN_uri, const char * IN_propname, const GValue* IN_value, org_gnome_Rhythmbox_Shell_set_song_property_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "setSongProperty", org_gnome_Rhythmbox_Shell_set_song_property_async_callback, stuff, g_free, G_TYPE_STRING, IN_uri, G_TYPE_STRING, IN_propname, G_TYPE_VALUE, IN_value, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_add_to_queue (DBusGProxy *proxy, const char * IN_uri, GError **error)

{
  return dbus_g_proxy_call (proxy, "addToQueue", error, G_TYPE_STRING, IN_uri, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_add_to_queue_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_add_to_queue_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_add_to_queue_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_add_to_queue_async (DBusGProxy *proxy, const char * IN_uri, org_gnome_Rhythmbox_Shell_add_to_queue_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "addToQueue", org_gnome_Rhythmbox_Shell_add_to_queue_async_callback, stuff, g_free, G_TYPE_STRING, IN_uri, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_quit (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "quit", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_quit_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_quit_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_quit_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_quit_async (DBusGProxy *proxy, org_gnome_Rhythmbox_Shell_quit_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "quit", org_gnome_Rhythmbox_Shell_quit_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_remove_from_queue (DBusGProxy *proxy, const char * IN_uri, GError **error)

{
  return dbus_g_proxy_call (proxy, "removeFromQueue", error, G_TYPE_STRING, IN_uri, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_remove_from_queue_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_remove_from_queue_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_remove_from_queue_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_remove_from_queue_async (DBusGProxy *proxy, const char * IN_uri, org_gnome_Rhythmbox_Shell_remove_from_queue_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "removeFromQueue", org_gnome_Rhythmbox_Shell_remove_from_queue_async_callback, stuff, g_free, G_TYPE_STRING, IN_uri, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Rhythmbox_Shell_clear_queue (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "clearQueue", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Rhythmbox_Shell_clear_queue_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Rhythmbox_Shell_clear_queue_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Rhythmbox_Shell_clear_queue_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Rhythmbox_Shell_clear_queue_async (DBusGProxy *proxy, org_gnome_Rhythmbox_Shell_clear_queue_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "clearQueue", org_gnome_Rhythmbox_Shell_clear_queue_async_callback, stuff, g_free, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_org_gnome_Rhythmbox_Shell */

G_END_DECLS