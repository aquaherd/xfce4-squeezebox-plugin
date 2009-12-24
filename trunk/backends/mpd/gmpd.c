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
#include "gmpd-private.h"


G_DEFINE_TYPE (GMpd, g_mpd, G_TYPE_OBJECT);
#define G_MPD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_MPD_TYPE, GMpdPrivate))
#ifndef LOG
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GMpd"
#define LOG g_message
#endif

/* region Private Realm */

struct _GMpdPrivate
{
	GSocketConnectable *address;
	GSocketClient *client;
	GSocketConnection *connection;
	GDataInputStream *istm;
	GCancellable *cancel;
	GError *error;
	gboolean inIdle;
	GHashTable *currentsong;
	GHashTable *playlists;
	GHashTable *stateinfo;
	GHashTable *changes;
	gint signal;
	GMpd *self;
};

static void _clear_object(GObject **socket) {
	if(*socket) {
		g_object_unref(*socket);
		socket = NULL;
	}
}

static void _error_clear(GMpdPrivate *priv) {
	if(NULL != priv->error) {
		g_error_free(priv->error);
		priv->error = NULL;
	}
}

static gboolean _error_extract(GMpdPrivate *priv, const gchar* line) {
	if(g_str_has_prefix(line, "ACK ")) {
		_error_clear(priv);
		priv->error = g_error_new(G_MPD_DOMAIN, G_MPD_ERROR, 
			"GMpd-Error:%s", line);
		return TRUE;
	}
	return FALSE;
}

static gchar * _read_response_line(GMpdPrivate *priv) {
	_error_clear(priv);
	gchar *line = g_data_input_stream_read_line(priv->istm, NULL, NULL, &priv->error);
	LOG("GMpd:got  %s", line);
	return line;
}

static gboolean _send_command_raw(GMpdPrivate *priv, const gchar* command) {
	GOutputStream *ostm = g_io_stream_get_output_stream(G_IO_STREAM(priv->connection));
	gssize sz1 = strlen(command);
	_error_clear(priv);
	gssize sz2 = g_output_stream_write(ostm, command, sz1, NULL, &priv->error);
	LOG("GMpd:sent %s", command);
	return (sz1 == sz2 || NULL == priv->error);
}

static void _update_dispatch(GMpdPrivate *priv, gchar *changeDetail) {
	void( *update_func)(GMpdPrivate *priv, gchar *changeDetail);
	update_func = g_hash_table_lookup(priv->changes, changeDetail);
	LOG("Dispatch %s->%p", changeDetail, update_func);
	if(update_func)
		update_func(priv, changeDetail);
}

static void _idle_cb(GObject *source_object, GAsyncResult *res,  gpointer user_data) {
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(user_data);
	gssize sz = 0;
	_error_clear(priv);
	gchar *line = g_data_input_stream_read_line_finish(priv->istm, res, &sz, &priv->error);
	priv->inIdle = FALSE;
	if(priv->error) {
		LOG("GMpd:IdleComplete:%s", priv->error->message);
		g_cancellable_reset(priv->cancel);
	} else {
		LOG("GMpd:IdleComplete:%d:%s", priv->signal, line);
		if(g_str_has_prefix(line, "changed: ")) {
			gchar *changeDetail = line + 9;
			_update_dispatch(priv, changeDetail);
			g_signal_emit(priv->self, priv->signal, 0, changeDetail);
		}
		g_free(line);
		// read OK
		line = _read_response_line(priv);
		g_free(line);
		_idle_enter(priv);
	}
}

static void _idle_enter(GMpdPrivate *priv) {
	if(!priv->inIdle) {
		LOG("GMpd:IdleEnter");
		priv->inIdle = TRUE;
		_send_command_raw(priv, "idle\n");
		g_data_input_stream_read_line_async(
			priv->istm, G_PRIORITY_DEFAULT, priv->cancel, _idle_cb, priv->self);
	}
}

static void _idle_cancel(GMpdPrivate *priv) {
	if(priv->inIdle) {
		priv->inIdle = FALSE;
		g_cancellable_cancel(priv->cancel);
		// flush the cancel events
		g_main_context_iteration(g_main_context_default(), TRUE);
		_send_command_raw(priv, "noidle\n");
		gchar *line = _read_response_line(priv);
		g_free(line);
	}
}

static gboolean _send_command_simple(GMpdPrivate *priv, const gchar* format, ...) {
	va_list args;
	va_start(args, format);
	gchar *command = g_strdup_vprintf(format, args);
	_idle_cancel(priv);
	_send_command_raw(priv, command);
	if(NULL == priv->error) {
		gchar *line = _read_response_line(priv);
		if (line) {
			_error_extract(priv, line);
			g_free(line);
		}
	}
	//va_end(args);
	g_free(command);
	_idle_enter(priv);
	return (NULL == priv->error);
}

static void _update_playlists(GMpdPrivate *priv, gchar *changeDetail) {
	//_idle_cancel(priv);
	if(_send_command_raw(priv, "listplaylists\n")) {
		gchar *line = _read_response_line(priv);
		if(_error_extract(priv, line)) {
			g_free(line);
			return;
		}
		while(line && g_strcmp0(line, "OK")) {
			if(!g_str_has_prefix(line, "playlist: ")) {
				g_hash_table_insert(priv->playlists, 
					g_strdup(line + 10), g_strdup("stock_playlist"));
			}
			g_free(line);
			line = _read_response_line(priv);
		}
	}
	//_idle_enter(priv);
}

static void _update_status(GMpdPrivate *priv, gchar *changeDetail) {
	//_idle_cancel(priv);
	if(_send_command_raw(priv, "status\n")) {
		gchar *line = _read_response_line(priv);
		if(_error_extract(priv, line)) {
			g_free(line);
			return;
		}
		while(line && g_strcmp0(line, "OK")) {
			gchar **tokens = g_strsplit(line, ":", 2);
			g_hash_table_insert(priv->stateinfo, g_strdup(tokens[0]), g_strdup(tokens[1]));
			g_free(line);
			g_strfreev(tokens);
			line = _read_response_line(priv);
		}
	}
	//_idle_enter(priv);
}

static void _update_currentsong(GMpdPrivate *priv, gchar *changeDetail) {
	//_idle_cancel(priv);
	if(_send_command_raw(priv, "currentsong\n")) {
		gchar *line = _read_response_line(priv);
		if(_error_extract(priv, line)) {
			g_free(line);
			return;
		}
		while(line && !g_str_has_prefix(line, "OK")) {
			gchar **tokens = g_strsplit(line, ":", 2);
			g_hash_table_insert(priv->currentsong, g_strdup(tokens[0]), g_strdup(tokens[1]));
			g_free(line);
			g_strfreev(tokens);
			line = _read_response_line(priv);
		}
	}
	//_idle_enter(priv);
}

/* endregion */

static void g_mpd_dispose (GObject *gobject) {
	/* Free all stateful data */
	g_mpd_disconnect(G_MPD(gobject));
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(gobject);
	_error_clear(priv);
	_clear_object((GObject**)&priv->cancel);
	g_hash_table_destroy(priv->currentsong);
	g_hash_table_destroy(priv->playlists);
	g_hash_table_destroy(priv->stateinfo);
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
	G_MPD_DOMAIN = g_quark_from_static_string("MPD");
	self->priv = priv = G_MPD_GET_PRIVATE (self);
	priv->cancel = g_cancellable_new();
	priv->currentsong = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	priv->playlists = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	priv->stateinfo = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	priv->changes = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		
	priv->signal = g_signal_new ("mpd-changed",
		G_TYPE_FROM_INSTANCE(self),
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		0 /* offset */,
		NULL /* accumulator */,
		NULL /* accumulator data */,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE /* return_type */,
		1     /* n_params */,
		G_TYPE_STRING /* what has changed */
		);
	priv->self = self;
	
	g_hash_table_insert(priv->changes, "database", NULL);
	g_hash_table_insert(priv->changes, "update", NULL);
	g_hash_table_insert(priv->changes, "stored_playlist", _update_playlists);
	g_hash_table_insert(priv->changes, "playlist", _update_playlists);
	g_hash_table_insert(priv->changes, "player", _update_currentsong);
	g_hash_table_insert(priv->changes, "mixer", _update_status);
	g_hash_table_insert(priv->changes, "output", _update_status);
	g_hash_table_insert(priv->changes, "options", _update_status);
}

GMpd *g_mpd_new(void) {
	return G_MPD(g_object_new(G_MPD_TYPE, NULL));
}

void g_mpd_disconnect(GMpd *self) {
	g_return_if_fail (G_IS_MPD (self));
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	if(priv->connection) {
		_send_command_simple(priv, "close\n");
	}
	_clear_object((GObject**)&priv->istm);
	_clear_object((GObject**)&priv->connection);
	_clear_object((GObject**)&priv->client);
	_clear_object((GObject**)&priv->address);
}
gboolean g_mpd_connect(GMpd *self, const gchar* host, const int port) {
	g_return_if_fail (G_IS_MPD (self));
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	g_mpd_disconnect(self);
	
	priv->address = g_network_address_new(host, port);
	
	GSocketConnectable *connectable = priv->address;
	priv->client = g_socket_client_new();
	_error_clear(priv);
	priv->connection = g_socket_client_connect(priv->client, connectable, NULL, &priv->error);
	LOG("GMpd:%p/%p", priv->client, priv->connection);
	if(NULL == priv->connection && priv->error) {
		LOG("GMpd:%s", priv->error->message);
	} else {
		priv->istm = g_data_input_stream_new(
			g_io_stream_get_input_stream(G_IO_STREAM(priv->connection)));

		gchar *line = _read_response_line(priv);
		if (line) {
			LOG("GMpd: %s", line);
			g_free(line);
		}
		if(g_mpd_get_current_track(self) &&
			g_mpd_get_playlists(self) &&
			g_mpd_get_state_info(self) ) {
			_idle_enter(priv);
			return TRUE;
		}
	}
	return FALSE;
}

gboolean g_mpd_next(GMpd *self){
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "next\n");
}
gboolean g_mpd_prev(GMpd *self){
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "prev\n");
}
gboolean g_mpd_play(GMpd *self){
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "play\n");
}
gboolean g_mpd_pause(GMpd *self){
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "pause\n");
}

GHashTable *g_mpd_get_current_track(GMpd *self) {
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);
	return priv->currentsong;
}

GHashTable *g_mpd_get_state_info(GMpd *self) {
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);
	return priv->stateinfo;
}

GHashTable *g_mpd_get_playlists(GMpd *self) {
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);
	return priv->playlists;
}

gboolean g_mpd_switch_playlist(GMpd *self, const gchar *playlist) {
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);
	if(_send_command_simple(priv, "clear\n")) {
		if(_send_command_simple(priv, "load %s\n", playlist))
			return TRUE;
	}
	return FALSE;
}

gboolean g_mpd_is_playing(GMpd *self) {
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);
	return !g_strcmp0(g_hash_table_lookup(priv->stateinfo, "state"), "playing");
}
