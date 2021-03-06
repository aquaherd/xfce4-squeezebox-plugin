/* Generated by dbus-binding-tool; do not edit! */

#include <glib/gtypes.h>
#include <glib/gerror.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_org_gnome_Listen
#define DBUS_GLIB_CLIENT_WRAPPERS_org_gnome_Listen

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_quit (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "quit", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_quit_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_quit_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_quit_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_quit_async (DBusGProxy *proxy, org_gnome_Listen_quit_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "quit", org_gnome_Listen_quit_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_play (DBusGProxy *proxy, const GValue* IN_uris, GError **error)

{
  return dbus_g_proxy_call (proxy, "play", error, G_TYPE_VALUE, IN_uris, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_play_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_play_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_play_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_play_async (DBusGProxy *proxy, const GValue* IN_uris, org_gnome_Listen_play_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "play", org_gnome_Listen_play_async_callback, stuff, g_free, G_TYPE_VALUE, IN_uris, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_current_playing (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "current_playing", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_current_playing_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_current_playing_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_current_playing_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_current_playing_async (DBusGProxy *proxy, org_gnome_Listen_current_playing_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "current_playing", org_gnome_Listen_current_playing_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_play_pause (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "play_pause", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_play_pause_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_play_pause_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_play_pause_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_play_pause_async (DBusGProxy *proxy, org_gnome_Listen_play_pause_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "play_pause", org_gnome_Listen_play_pause_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_next (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "next", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_next_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_next_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_next_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_next_async (DBusGProxy *proxy, org_gnome_Listen_next_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "next", org_gnome_Listen_next_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_enqueue (DBusGProxy *proxy, const GValue* IN_uris, GError **error)

{
  return dbus_g_proxy_call (proxy, "enqueue", error, G_TYPE_VALUE, IN_uris, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_enqueue_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_enqueue_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_enqueue_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_enqueue_async (DBusGProxy *proxy, const GValue* IN_uris, org_gnome_Listen_enqueue_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "enqueue", org_gnome_Listen_enqueue_async_callback, stuff, g_free, G_TYPE_VALUE, IN_uris, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_hello (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "hello", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_hello_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_hello_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_hello_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_hello_async (DBusGProxy *proxy, org_gnome_Listen_hello_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "hello", org_gnome_Listen_hello_async_callback, stuff, g_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_gnome_Listen_previous (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "previous", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_gnome_Listen_previous_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_gnome_Listen_previous_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_gnome_Listen_previous_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_gnome_Listen_previous_async (DBusGProxy *proxy, org_gnome_Listen_previous_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_new (DBusGAsyncData, 1);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "previous", org_gnome_Listen_previous_async_callback, stuff, g_free, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_org_gnome_Listen */

G_END_DECLS
