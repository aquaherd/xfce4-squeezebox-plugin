/*		
 * 		gmpd.c
 * 
 * 		Copyright 2009 Hakan Erduman <hakan@erduman.de>
 * 
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */
#include <gio/gio.h>
#include <string.h>
#include "gmpd.h"
G_DEFINE_TYPE (GMpd, g_mpd, G_TYPE_OBJECT);
#define G_MPD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_MPD_TYPE, GMpdPrivate))
#ifndef LOG
#define LOG g_message
#endif
struct _GMpdPrivate
{
	GSocketConnectable *address;
	GSocketClient *client;
	GSocketConnection *connection;
	GError *error;
	gboolean inIdle;
};

void _clear_error(GMpdPrivate *priv) {
	if(NULL != priv->error) {
		g_error_free(priv->error);
		priv->error = NULL;
	}
}

static void g_mpd_dispose (GObject *gobject) {
  /* Free all stateful data */
  g_mpd_disconnect(G_MPD(gobject));
  _clear_error(G_MPD_GET_PRIVATE(gobject));
  
  /* Chain up to the parent class */
  G_OBJECT_CLASS (g_mpd_parent_class)->dispose (gobject);
}

static void g_mpd_finalize (GObject *gobject) {
  
  /* Chain up to the parent class */
  G_OBJECT_CLASS (g_mpd_parent_class)->finalize (gobject);
}

static void g_mpd_class_init (GMpdClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = g_mpd_dispose;
  gobject_class->finalize = g_mpd_finalize;  
  g_type_class_add_private (klass, sizeof (GMpdPrivate));
}

static void
g_mpd_init (GMpd *self)
{
  GMpdPrivate *priv;

  self->priv = priv = G_MPD_GET_PRIVATE (self);
}

GMpd *g_mpd_new(void) {
	return G_MPD(g_object_new(G_MPD_TYPE, NULL));
}

void _clear_object(GObject **socket) {
	if(*socket) {
		g_object_unref(*socket);
		socket = NULL;
	}
}

void _init_tcp_socket_client(GMpdPrivate *priv) {
	GSocketConnectable *connectable = priv->address;
	priv->client = g_socket_client_new();
	_clear_error(priv);
	priv->connection = g_socket_client_connect(priv->client, connectable, NULL, &priv->error);
	LOG("GMpd:%p/%p", priv->client, priv->connection);
	if(NULL == priv->connection && priv->error) {
		LOG("GMpd:%s", priv->error->message);
	}
}

void _async_ready_cb(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
}


gchar * _read_response(GMpdPrivate *priv) {
	GDataInputStream *istm = g_data_input_stream_new(
		g_io_stream_get_input_stream(G_IO_STREAM(priv->connection)));
	return g_data_input_stream_read_line(istm, NULL, NULL, &priv->error);
}

gboolean _send_command(GMpdPrivate *priv, const gchar* command) {
	GOutputStream *ostm = g_io_stream_get_output_stream(G_IO_STREAM(priv->connection));
	gssize sz1 = strlen(command);
	_clear_error(priv);
	gssize sz2 = g_output_stream_write(ostm, command, sz1, NULL, &priv->error);
	if(NULL == priv->error) {
		gchar *line = _read_response(priv);
		if (line) {
			LOG("GMpd: %s", line);
			g_free(line);
		}
	}
	return (sz1 == sz2 || NULL == priv->error);
}

void g_mpd_disconnect(GMpd *self) {
	g_return_if_fail (G_IS_MPD (self));
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	if(priv->connection) {
		_send_command(priv, "close\n");
	}
	_clear_object((GObject**)&priv->connection);
	_clear_object((GObject**)&priv->client);
	_clear_object((GObject**)&priv->address);
}
gboolean g_mpd_connect(GMpd *self, const gchar* host, const int port) {
	g_return_if_fail (G_IS_MPD (self));
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	g_mpd_disconnect(self);
	
	priv->address = g_network_address_new(host, port);
	
	_init_tcp_socket_client(priv);
	gchar *line = _read_response(priv);
	if (line) {
		LOG("GMpd: %s", line);
		g_free(line);
	}
	return TRUE;
}

gboolean g_mpd_next(GMpd *self){
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	return _send_command(priv, "next\n");
}
gboolean g_mpd_prev(GMpd *self){
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	return _send_command(priv, "prev\n");
}

