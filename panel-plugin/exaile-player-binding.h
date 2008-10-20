/* Generated by dbus-binding-tool; do not edit! */

#include <glib/gtypes.h>
#include <glib/gerror.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_org_exaile_DBusInterface
#define DBUS_GLIB_CLIENT_WRAPPERS_org_exaile_DBusInterface

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_cover_path (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_cover_path", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_cover_path_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_cover_path_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_cover_path_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_cover_path_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_cover_path_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_cover_path", org_exaile_DBusInterface_get_cover_path_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_decrease_volume (DBusGProxy *proxy, const guchar IN_vol, GError **error)

{
  return dbus_g_proxy_call (proxy, "decrease_volume", error, G_TYPE_UCHAR, IN_vol, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_decrease_volume_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_decrease_volume_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_decrease_volume_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_decrease_volume_async (DBusGProxy *proxy, const guchar IN_vol, org_exaile_DBusInterface_decrease_volume_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "decrease_volume", org_exaile_DBusInterface_decrease_volume_async_callback, stuff, g_free, G_TYPE_UCHAR, IN_vol, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_volume (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_volume", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_volume_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_volume_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_volume_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_volume_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_volume_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_volume", org_exaile_DBusInterface_get_volume_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_prev_track (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "prev_track", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_prev_track_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_prev_track_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_prev_track_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_prev_track_async (DBusGProxy *proxy, org_exaile_DBusInterface_prev_track_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "prev_track", org_exaile_DBusInterface_prev_track_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_title (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_title", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_title_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_title_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_title_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_title_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_title_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_title", org_exaile_DBusInterface_get_title_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_popup (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "popup", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_popup_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_popup_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_popup_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_popup_async (DBusGProxy *proxy, org_exaile_DBusInterface_popup_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "popup", org_exaile_DBusInterface_popup_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_query (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "query", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_query_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_query_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_query_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_query_async (DBusGProxy *proxy, org_exaile_DBusInterface_query_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "query", org_exaile_DBusInterface_query_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_play_file (DBusGProxy *proxy, const char * IN_filename, GError **error)

{
  return dbus_g_proxy_call (proxy, "play_file", error, G_TYPE_STRING, IN_filename, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_play_file_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_play_file_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_play_file_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_play_file_async (DBusGProxy *proxy, const char * IN_filename, org_exaile_DBusInterface_play_file_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "play_file", org_exaile_DBusInterface_play_file_async_callback, stuff, g_free, G_TYPE_STRING, IN_filename, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_set_rating (DBusGProxy *proxy, const guchar IN_rating, GError **error)

{
  return dbus_g_proxy_call (proxy, "set_rating", error, G_TYPE_UCHAR, IN_rating, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_set_rating_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_set_rating_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_set_rating_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_set_rating_async (DBusGProxy *proxy, const guchar IN_rating, org_exaile_DBusInterface_set_rating_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "set_rating", org_exaile_DBusInterface_set_rating_async_callback, stuff, g_free, G_TYPE_UCHAR, IN_rating, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_version (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_version", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_version_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_version_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_version_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_version_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_version_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_version", org_exaile_DBusInterface_get_version_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_album (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_album", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_album_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_album_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_album_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_album_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_album_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_album", org_exaile_DBusInterface_get_album_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_rating (DBusGProxy *proxy, gint* OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_rating", error, G_TYPE_INVALID, G_TYPE_INT, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_rating_reply) (DBusGProxy *proxy, gint OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_rating_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  gint OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INT, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_rating_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_rating_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_rating_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_rating", org_exaile_DBusInterface_get_rating_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_status (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "status", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_status_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_status_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_status_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_status_async (DBusGProxy *proxy, org_exaile_DBusInterface_status_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "status", org_exaile_DBusInterface_status_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_length (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_length", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_length_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_length_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_length_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_length_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_length_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_length", org_exaile_DBusInterface_get_length_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_play (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "play", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_play_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_play_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_play_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_play_async (DBusGProxy *proxy, org_exaile_DBusInterface_play_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "play", org_exaile_DBusInterface_play_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_current_position (DBusGProxy *proxy, guchar* OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "current_position", error, G_TYPE_INVALID, G_TYPE_UCHAR, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_current_position_reply) (DBusGProxy *proxy, guchar OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_current_position_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  guchar OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_UCHAR, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_current_position_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_current_position_async (DBusGProxy *proxy, org_exaile_DBusInterface_current_position_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "current_position", org_exaile_DBusInterface_current_position_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_play_pause (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "play_pause", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_play_pause_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_play_pause_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_play_pause_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_play_pause_async (DBusGProxy *proxy, org_exaile_DBusInterface_play_pause_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "play_pause", org_exaile_DBusInterface_play_pause_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_next_track (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "next_track", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_next_track_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_next_track_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_next_track_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_next_track_async (DBusGProxy *proxy, org_exaile_DBusInterface_next_track_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "next_track", org_exaile_DBusInterface_next_track_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_stop (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "stop", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_stop_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_stop_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_stop_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_stop_async (DBusGProxy *proxy, org_exaile_DBusInterface_stop_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "stop", org_exaile_DBusInterface_stop_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_increase_volume (DBusGProxy *proxy, const guchar IN_vol, GError **error)

{
  return dbus_g_proxy_call (proxy, "increase_volume", error, G_TYPE_UCHAR, IN_vol, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_increase_volume_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_increase_volume_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_increase_volume_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_increase_volume_async (DBusGProxy *proxy, const guchar IN_vol, org_exaile_DBusInterface_increase_volume_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "increase_volume", org_exaile_DBusInterface_increase_volume_async_callback, stuff, g_free, G_TYPE_UCHAR, IN_vol, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_track_attr (DBusGProxy *proxy, const char * IN_attr, char ** OUT_arg1, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_track_attr", error, G_TYPE_STRING, IN_attr, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg1, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_track_attr_reply) (DBusGProxy *proxy, char * OUT_arg1, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_track_attr_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg1;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg1, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_track_attr_reply)data->cb) (proxy, OUT_arg1, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_track_attr_async (DBusGProxy *proxy, const char * IN_attr, org_exaile_DBusInterface_get_track_attr_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_track_attr", org_exaile_DBusInterface_get_track_attr_async_callback, stuff, g_free, G_TYPE_STRING, IN_attr, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_get_artist (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "get_artist", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_get_artist_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_get_artist_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_get_artist_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_get_artist_async (DBusGProxy *proxy, org_exaile_DBusInterface_get_artist_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "get_artist", org_exaile_DBusInterface_get_artist_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_play_cd (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "play_cd", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_play_cd_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_play_cd_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_play_cd_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_play_cd_async (DBusGProxy *proxy, org_exaile_DBusInterface_play_cd_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "play_cd", org_exaile_DBusInterface_play_cd_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_toggle_visibility (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "toggle_visibility", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_toggle_visibility_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_toggle_visibility_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_toggle_visibility_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_toggle_visibility_async (DBusGProxy *proxy, org_exaile_DBusInterface_toggle_visibility_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "toggle_visibility", org_exaile_DBusInterface_toggle_visibility_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_popup_text (DBusGProxy *proxy, const GValue* IN_text, GError **error)

{
  return dbus_g_proxy_call (proxy, "popup_text", error, G_TYPE_VALUE, IN_text, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_popup_text_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_popup_text_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_popup_text_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_popup_text_async (DBusGProxy *proxy, const GValue* IN_text, org_exaile_DBusInterface_popup_text_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "popup_text", org_exaile_DBusInterface_popup_text_async_callback, stuff, g_free, G_TYPE_VALUE, IN_text, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_exaile_DBusInterface_test_service (DBusGProxy *proxy, const char * IN_arg, GError **error)

{
  return dbus_g_proxy_call (proxy, "test_service", error, G_TYPE_STRING, IN_arg, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_exaile_DBusInterface_test_service_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_exaile_DBusInterface_test_service_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_exaile_DBusInterface_test_service_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_exaile_DBusInterface_test_service_async (DBusGProxy *proxy, const char * IN_arg, org_exaile_DBusInterface_test_service_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "test_service", org_exaile_DBusInterface_test_service_async_callback, stuff, g_free, G_TYPE_STRING, IN_arg, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_org_exaile_DBusInterface */

G_END_DECLS
