/* Generated by dbus-binding-tool; do not edit! */

#include <glib.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#ifndef _DBUS_GLIB_ASYNC_DATA_FREE
#define _DBUS_GLIB_ASYNC_DATA_FREE
static
#ifdef G_HAVE_INLINE
inline
#endif
void
_dbus_glib_async_data_free (gpointer stuff)
{
	g_slice_free (DBusGAsyncData, stuff);
}
#endif

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_radiotray
#define DBUS_GLIB_CLIENT_WRAPPERS_radiotray

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_list_radios (DBusGProxy *proxy, char *** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "listRadios", error, G_TYPE_INVALID, G_TYPE_STRV, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*radiotray_list_radios_reply) (DBusGProxy *proxy, char * *OUT_arg0, GError *error, gpointer userdata);

static void
radiotray_list_radios_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char ** OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRV, &OUT_arg0, G_TYPE_INVALID);
  (*(radiotray_list_radios_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_list_radios_async (DBusGProxy *proxy, radiotray_list_radios_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "listRadios", radiotray_list_radios_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_turn_off (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "turnOff", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*radiotray_turn_off_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
radiotray_turn_off_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(radiotray_turn_off_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_turn_off_async (DBusGProxy *proxy, radiotray_turn_off_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "turnOff", radiotray_turn_off_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_volume_down (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "volumeDown", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*radiotray_volume_down_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
radiotray_volume_down_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(radiotray_volume_down_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_volume_down_async (DBusGProxy *proxy, radiotray_volume_down_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "volumeDown", radiotray_volume_down_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_play_radio (DBusGProxy *proxy, const GValue* IN_radioName, GError **error)

{
  return dbus_g_proxy_call (proxy, "playRadio", error, G_TYPE_VALUE, IN_radioName, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*radiotray_play_radio_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
radiotray_play_radio_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(radiotray_play_radio_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_play_radio_async (DBusGProxy *proxy, const GValue* IN_radioName, radiotray_play_radio_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "playRadio", radiotray_play_radio_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_VALUE, IN_radioName, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_get_current_meta_data (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "getCurrentMetaData", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*radiotray_get_current_meta_data_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
radiotray_get_current_meta_data_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(radiotray_get_current_meta_data_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_get_current_meta_data_async (DBusGProxy *proxy, radiotray_get_current_meta_data_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "getCurrentMetaData", radiotray_get_current_meta_data_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_toggle_play (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "togglePlay", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*radiotray_toggle_play_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
radiotray_toggle_play_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(radiotray_toggle_play_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_toggle_play_async (DBusGProxy *proxy, radiotray_toggle_play_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "togglePlay", radiotray_toggle_play_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_volume_up (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "volumeUp", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*radiotray_volume_up_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
radiotray_volume_up_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(radiotray_volume_up_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_volume_up_async (DBusGProxy *proxy, radiotray_volume_up_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "volumeUp", radiotray_volume_up_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_play_url (DBusGProxy *proxy, const GValue* IN_url, GError **error)

{
  return dbus_g_proxy_call (proxy, "playUrl", error, G_TYPE_VALUE, IN_url, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*radiotray_play_url_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
radiotray_play_url_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(radiotray_play_url_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_play_url_async (DBusGProxy *proxy, const GValue* IN_url, radiotray_play_url_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "playUrl", radiotray_play_url_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_VALUE, IN_url, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
radiotray_get_current_radio (DBusGProxy *proxy, char ** OUT_arg0, GError **error)

{
  return dbus_g_proxy_call (proxy, "getCurrentRadio", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_arg0, G_TYPE_INVALID);
}

typedef void (*radiotray_get_current_radio_reply) (DBusGProxy *proxy, char * OUT_arg0, GError *error, gpointer userdata);

static void
radiotray_get_current_radio_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_arg0;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_arg0, G_TYPE_INVALID);
  (*(radiotray_get_current_radio_reply)data->cb) (proxy, OUT_arg0, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
radiotray_get_current_radio_async (DBusGProxy *proxy, radiotray_get_current_radio_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "getCurrentRadio", radiotray_get_current_radio_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_radiotray */

G_END_DECLS
