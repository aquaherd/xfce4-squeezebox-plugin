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
#include "gmpd.h"
#include <gio/gio.h>
G_DEFINE_TYPE (GMpd, g_mpd, G_TYPE_OBJECT);
#define G_MPD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_MPD_TYPE, GMpdPrivate))

struct _GMpdPrivate
{
	GSocketAddress *address;
	GSocketClient *client;
	GSocketConnection *connection;
};

static void
g_mpd_class_init (GMpdClass *klass)
{
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
	GSocketConnectable *connectable = G_SOCKET_CONNECTABLE(priv->address);
	priv->client = g_socket_client_new();
	priv->connection = g_socket_client_connect(priv->client, connectable, NULL, NULL);
}

void g_mpd_disconnect(GMpd *self) {
	g_return_if_fail (G_IS_MPD (self));
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	_clear_object((GObject**)&priv->address);
	_clear_object((GObject**)&priv->client);
	_clear_object((GObject**)&priv->connection);
	if(priv->address) {
		g_object_unref(priv->address);
		priv->address = NULL;
	}
}
gboolean g_mpd_connect(GMpd *self, const gchar* host, const int port) {
	g_return_if_fail (G_IS_MPD (self));
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	g_mpd_disconnect(self);
	
	priv->address = g_inet_socket_address_new(
		g_inet_address_new_from_string(host), port);
	
	_init_tcp_socket_client(priv);
	return TRUE;
}
