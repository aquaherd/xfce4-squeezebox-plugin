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
#include <gdk/gdk.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "gmpd.h"
#include "gmpd-private.h"


G_DEFINE_TYPE (GMpd, g_mpd, G_TYPE_OBJECT);
#define G_MPD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_MPD_TYPE, GMpdPrivate))
/* region Private Realm */

typedef enum GMpdPropNames
{
  PROP_0,

  PROP_HOST,
  PROP_PORT,
  PROP_PASS
}GMpdPropNames;

struct _GMpdPrivate
{
	GSocketConnectable *address;
	GSocketClient *client;
	GSocketConnection *connection;
	gchar *pass;
	GDataInputStream *istm;
	GError *error;
	gboolean inIdle;
	GHashTable *currentsong;
	GHashTable *playlists;
	GHashTable *stateinfo;
	GHashTable *changes;
	GThread *thread;
	GMpd *self;
};

static void _clear_object(gpointer *socket) {
	if(*socket) {
		g_object_unref(*socket);
		*socket = NULL;
	}
}

static void _error_clear(GMpdPrivate *priv) {
	if(NULL != priv->error) {
		g_error_free(priv->error);
		priv->error = NULL;
	}
}

static void _free(void *ptr) {
	// can't possibly free function pointers, so this is a dummy.
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
	gchar *line;
	_error_clear(priv);
	line = g_data_input_stream_read_line(priv->istm, NULL, NULL, &priv->error);
	LOG("got  '%s'", line);
	return line;
}

static gboolean _send_command_raw(GMpdPrivate *priv, const gchar* command) {
	GOutputStream *ostm = g_io_stream_get_output_stream(G_IO_STREAM(priv->connection));
	gssize sz1 = strlen(command), sz2;
	if (NULL == priv->connection)
		return FALSE;
	_error_clear(priv);
	sz2 = g_output_stream_write(ostm, command, sz1, NULL, &priv->error);
	LOG("sent '%s'", command);
	return (sz1 == sz2 || NULL == priv->error);
}

static void _update_dispatch(GMpdPrivate *priv, gchar *changeDetail) {
	GMpdClass *klass = G_MPD_GET_CLASS(priv->self);
	
	LOG("_update_dispatch '%s'", changeDetail);
	if(changeDetail && *changeDetail) {
		gchar **changes = g_strsplit(changeDetail, ",", 0);
		void( *update_func)(GMpdPrivate *priv, gchar *changeDetail);
		gchar **change = changes;
		gdk_threads_enter();
		g_signal_emit(priv->self, klass->signal[SIGNAL_IDLE], 0, changeDetail);
		
		while(*change) {
			update_func = g_hash_table_lookup(priv->changes, *change);
			LOG("Dispatch %s->%p", *change, update_func);
			if(update_func)
				update_func(priv, changeDetail);
			change++;
		}
		g_strfreev(changes);
		gdk_threads_leave();
	}
}

static void _disconnect(GMpdPrivate *priv, gboolean threaded) {
	GMpdClass *klass = G_MPD_GET_CLASS(priv->self);
	LOG("Disconnect");

	if(threaded && priv->connection) {
		if(priv->thread) {
			gdk_threads_leave();
			_idle_cancel(priv);
			_idle_enter(priv);
			_send_command_raw(priv, "close\n");
			g_thread_join(priv->thread);
			priv->thread = NULL;
			gdk_threads_enter();
		}
	}
	_clear_object((gpointer*)&priv->istm);
	_clear_object((gpointer*)&priv->connection);
	_clear_object((gpointer*)&priv->client);
	_clear_object((gpointer*)&priv->address);
	g_signal_emit(priv->self, klass->signal[SIGNAL_STATUS], 0);
}

static void _idle_enter(GMpdPrivate *priv) {
	if(!priv->inIdle) {
		LOG("IdleEnter");
		priv->inIdle = TRUE;
		_send_command_raw(priv, "idle\n");
	}
}

static void _idle_cancel(GMpdPrivate *priv) {
	if(priv->inIdle) {
		gchar *line;
		gboolean isOK;
		priv->inIdle = FALSE;
		_send_command_raw(priv, "noidle\n");
		do {
			line = _read_response_line(priv);
			isOK = NULL != line && (!g_strcmp0(line, "OK") || g_str_has_prefix(line, "ACK "));
			g_free(line);
		} while(!isOK);
	}
}

static gpointer _idle_thread(gpointer user_data) {
   GMpdPrivate *priv = G_MPD_GET_PRIVATE(user_data);
	GSocket *socket = g_socket_connection_get_socket(priv->connection);
	LOG("EnterThread");
	while(g_socket_condition_wait(socket, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP, NULL, NULL)) {
		if(priv->inIdle) {
			GString *changes = g_string_new("");
			gchar *line = _read_response_line(priv);
			LOG("IdleComplete:%s", line);
			priv->inIdle  = FALSE;
			if(NULL == line) {
				break;
			} else {
				while(g_str_has_prefix(line, "changed: ")) {
					gchar *changeDetail = line + 9;
					g_string_append(changes, changeDetail);
					g_free(line);
					line = _read_response_line(priv);
					if(g_strcmp0(line, "OK"))
						g_string_append(changes, ",");
				}
				g_free(line);
				_update_dispatch(priv, changes->str);
				g_string_free(changes, TRUE);
			}
			_idle_enter(priv);
		}
	}
	_update_dispatch(priv, "socket");
	LOG("LeaveThread");
	return NULL;
}

static gboolean _send_command_simple(GMpdPrivate *priv, const gchar* format, ...)
{
	va_list args;
	gchar *command;
	if (NULL == priv->connection)
		return FALSE;
	va_start(args, format);
	command = g_strdup_vprintf(format, args);
	_idle_cancel(priv);
	_send_command_raw(priv, command);
	g_usleep(25000);
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

static void _update_database(GMpdPrivate *priv, gchar *changeDetail) {
	/* nothing to do for now */
}

static void _update_playlists(GMpdPrivate *priv, gchar *changeDetail) {
	if(_send_command_raw(priv, "listplaylists\n")) {
		GMpdClass *klass = G_MPD_GET_CLASS(priv->self);
		gchar *line = _read_response_line(priv);

		if(_error_extract(priv, line)) {
			g_free(line);
			return;
		}
		g_hash_table_remove_all(priv->playlists);
		while(line && g_strcmp0(line, "OK")) {
			if(g_str_has_prefix(line, "playlist: ")) {
				gchar *newList = g_strdup(line + 10);
				g_hash_table_insert(priv->playlists, 
					newList, g_strdup("playlist"));
			}
			g_free(line);
			line = _read_response_line(priv);
		}
		g_signal_emit(priv->self, klass->signal[SIGNAL_PLAYLIST], 0);
	}
}

static void _update_player(GMpdPrivate *priv, gchar *changeDetail) {
	gchar *oldSongID = g_strdup(g_hash_table_lookup(priv->stateinfo, "songid"));
	_update_status(priv, changeDetail);
	if(g_strcmp0(oldSongID, g_hash_table_lookup(priv->stateinfo, "songid")))
		_update_currentsong(priv, changeDetail);
	g_free(oldSongID);
}

static void _update_socket(GMpdPrivate *priv, gchar *changeDetail) {
	// we have gotten hang up.
	LOG("_update_socket: %s.", changeDetail);
	_disconnect(priv, FALSE);
	priv->connection = NULL;
}

static void _update_status(GMpdPrivate *priv, gchar *changeDetail) {
	LOG("_update_status: %s.", changeDetail);
	if(_send_command_raw(priv, "status\n")) {
		GMpdClass *klass = G_MPD_GET_CLASS(priv->self);
		gchar *line = _read_response_line(priv);
		gboolean changed = FALSE;
		if(_error_extract(priv, line)) {
			g_free(line);
			return;
		}
		while(line && g_strcmp0(line, "OK")) {
			gchar **tokens = g_strsplit(line, ":", 2);
			gchar *oldKey = tokens[0];
			gchar *newValue = g_strdup(tokens[1]+1);
			changed |= g_strcmp0(newValue, g_hash_table_lookup(priv->stateinfo, oldKey));
			g_hash_table_insert(priv->stateinfo, g_strdup(oldKey), newValue);
			g_free(line);
			g_strfreev(tokens);
			line = _read_response_line(priv);
		}
		if(changed)
			g_signal_emit(priv->self, klass->signal[SIGNAL_STATUS], 0);
	}
}

static void _update_currentsong(GMpdPrivate *priv, gchar *changeDetail) {
	if(_send_command_raw(priv, "currentsong\n")) {
		GMpdClass *klass = G_MPD_GET_CLASS(priv->self);
		gchar *line = _read_response_line(priv);
		gboolean changed = FALSE;
		if(_error_extract(priv, line)) {
			g_free(line);
			return;
		}
		while(line && !g_str_has_prefix(line, "OK")) {
			gchar **tokens = g_strsplit(line, ":", 2);
			gchar *oldKey = tokens[0];
			gchar *newValue = g_strdup(tokens[1]+1);
			changed |= g_strcmp0(newValue, g_hash_table_lookup(priv->currentsong, oldKey));
			g_hash_table_insert(priv->currentsong, g_strdup(oldKey), newValue);
			g_free(line);
			g_strfreev(tokens);
			line = _read_response_line(priv);
		}
		if(changed)
			g_signal_emit(priv->self, klass->signal[SIGNAL_SONG], 0);
	}
}

/* endregion */


/* region public realm */
static void g_mpd_dispose (GObject *gobject) {
	/* Free all stateful data */
	GMpdPrivate *priv = G_MPD_GET_PRIVATE(gobject);
	LOG("Dispose");
	g_mpd_disconnect(G_MPD(gobject));
	_error_clear(priv);
	g_hash_table_destroy(priv->currentsong);
	g_hash_table_destroy(priv->playlists);
	g_hash_table_destroy(priv->stateinfo);
	//g_hash_table_destroy(priv->changes);
	/* Chain up to the parent class */
	G_OBJECT_CLASS (g_mpd_parent_class)->dispose (gobject);
}

static void g_mpd_finalize (GObject *gobject) {

	LOG("Finalize");
	/* Chain up to the parent class */
	G_OBJECT_CLASS (g_mpd_parent_class)->finalize (gobject);
}

static void g_mpd_set_property(GObject *self, guint property_id,
                        const GValue *value, GParamSpec *pspec) {
	GMpdPrivate *priv;
	g_return_if_fail (G_IS_MPD (self));
	priv = G_MPD_GET_PRIVATE(self);
	switch(property_id) {
		/*
		case PROP_HOST:
			g_free(priv->host);
			priv->host = g_value_dup_string(value);
			break;
		case PROP_PORT:
			priv->port = g_value_get_int(value);
			break;
		*/
		case PROP_PASS:
			g_free(priv->pass);
			priv->pass = g_value_dup_string(value);
			break;
	}
}

static void g_mpd_get_property(GObject *self, guint property_id,
                        GValue *value, GParamSpec *pspec) {
	GMpdPrivate *priv;
	g_return_if_fail (G_IS_MPD (self));
	priv = G_MPD_GET_PRIVATE(self);
	switch(property_id) {
		case PROP_HOST:
			g_value_set_string(value, 
				g_network_address_get_hostname(G_NETWORK_ADDRESS(priv->address)));
			break;
		case PROP_PORT:
			g_value_set_int(value, 
				g_network_address_get_port(G_NETWORK_ADDRESS(priv->address)));
			break;
		case PROP_PASS:
			g_value_set_string(value, "");
			break;
	}
}						
   

static void g_mpd_class_init (GMpdClass *klass)
{
	/* class init */
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GParamSpec *pspec;

	gobject_class->dispose = g_mpd_dispose;
	gobject_class->finalize = g_mpd_finalize;
	gobject_class->set_property = g_mpd_set_property;  
	gobject_class->get_property = g_mpd_get_property;  

	/* signals */	
	klass->signal[SIGNAL_IDLE] = g_signal_new ("idle-changed",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		0 /* offset */,
		NULL /* accumulator */,
		NULL /* accumulator data */,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE /* return_type */,
		1     /* n_params */,
		G_TYPE_STRING /* what has changed */
		);
	
	klass->signal[SIGNAL_STATUS] = g_signal_new ("status-changed",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		0 /* offset */,
		NULL /* accumulator */,
		NULL /* accumulator data */,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE /* return_type */,
		0     /* no params */
		);
	
	klass->signal[SIGNAL_SONG] = g_signal_new ("song-changed",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		0 /* offset */,
		NULL /* accumulator */,
		NULL /* accumulator data */,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE /* return_type */,
		0     /* no params */
		);
	
	klass->signal[SIGNAL_PLAYLIST] = g_signal_new ("playlist-changed",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		0 /* offset */,
		NULL /* accumulator */,
		NULL /* accumulator data */,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE /* return_type */,
		0     /* no params */
		);
		
	/* properties */
	pspec = g_param_spec_string ("host",
		"MPD Host", "Set MPD's host", "localhost" /* default value */,
		G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_PASS, pspec);	

	pspec = g_param_spec_int ("port",
		"MPD Port", "Set MPD's port", 1, 65535, 6600 /* default value */,
		G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_PASS, pspec);	

	pspec = g_param_spec_string ("password",
		"MPD Passphrase", "Set MPD's passphrase", NULL /* default value */,
		G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PASS, pspec);	

	/* private data */
	g_type_class_add_private (klass, sizeof (GMpdPrivate));
}

static void
g_mpd_init (GMpd *self)
{
	GMpdPrivate *priv;
	G_MPD_DOMAIN = g_quark_from_static_string("MPD");
	self->priv = priv = G_MPD_GET_PRIVATE (self);
	priv->currentsong = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	priv->playlists = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	priv->stateinfo = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	priv->changes = 
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _free);
		
	priv->self = self;
	
	g_hash_table_insert(priv->changes, "database", _update_database);
	g_hash_table_insert(priv->changes, "update", _update_database);
	g_hash_table_insert(priv->changes, "stored_playlist", _update_playlists);
	g_hash_table_insert(priv->changes, "playlist", _update_playlists);
	g_hash_table_insert(priv->changes, "player", _update_player);
	g_hash_table_insert(priv->changes, "mixer", _update_status);
	g_hash_table_insert(priv->changes, "output", _update_status);
	g_hash_table_insert(priv->changes, "options", _update_status);
	g_hash_table_insert(priv->changes, "socket", _update_socket);
}

GMpd *g_mpd_new(void) {
	return G_MPD(g_object_new(G_MPD_TYPE, NULL));
}

void g_mpd_disconnect(GMpd *self) {
	g_return_if_fail (G_IS_MPD (self));
	{
		GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);
		_disconnect(priv, TRUE);
	}
}
gboolean g_mpd_connect(GMpd *self, const gchar* host, const int port) {
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	{
		GMpdPrivate *priv = G_MPD_GET_PRIVATE(self);
		GSocketConnectable *connectable;
		
		_disconnect(priv, FALSE);
		LOG("Connect");
		
		priv->address = g_network_address_new(host, port);
		connectable = priv->address;
		priv->client = g_socket_client_new();
		_error_clear(priv);

		priv->connection = g_socket_client_connect(priv->client, connectable, NULL, &priv->error);
		LOG("%p/%p", priv->client, priv->connection);

		if(NULL == priv->connection && priv->error) {
			LOG("%s", priv->error->message);
		} else {
			gchar *line;
			priv->istm = g_data_input_stream_new(
				g_io_stream_get_input_stream(G_IO_STREAM(priv->connection)));

			line = _read_response_line(priv);
			g_free(line);
			if(priv->pass) {
				_send_command_simple(priv, "password %s", priv->pass);
				line = _read_response_line(priv); 
				g_free(line);
			}
			_update_player(priv, "");
			_update_playlists(priv, "");
			if(NULL != priv->error) {
				LOG("Error %s", priv->error->message);
			}
			_idle_enter(priv);
			priv->thread = g_thread_new("mpdIdle", _idle_thread, self);
			return (priv->error == NULL);
		}
	}
	return FALSE;
}

gboolean g_mpd_next(GMpd *self){
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "next\n");
}
gboolean g_mpd_previous(GMpd *self){
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "previous\n");
}
gboolean g_mpd_play(GMpd *self){
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "play\n");
}
gboolean g_mpd_pause(GMpd *self){
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);

	return _send_command_simple(priv, "pause\n");
}
gboolean g_mpd_stop(GMpd *self){
   GMpdPrivate *priv;
   g_return_val_if_fail (G_IS_MPD (self), FALSE);
   priv = G_MPD_GET_PRIVATE(self);

   return _send_command_simple(priv, "stop\n");
}

GHashTable *g_mpd_get_current_track(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return priv->currentsong;
}

GHashTable *g_mpd_get_state_info(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return priv->stateinfo;
}

GHashTable *g_mpd_get_playlists(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return priv->playlists;
}

gboolean g_mpd_switch_playlist(GMpd *self, const gchar *playlist) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	_send_command_simple(priv, "clear\n");
	//_idle_cb(self);
	_send_command_simple(priv, "load \"%s\"\n", playlist);
	return TRUE;
}

gboolean g_mpd_is_playing(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return !g_strcmp0(g_hash_table_lookup(priv->stateinfo, "state"), "play");
}

gboolean g_mpd_is_online(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return (NULL != priv->connection);
}

gboolean g_mpd_get_random(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return !g_strcmp0(g_hash_table_lookup(priv->stateinfo, "random"), "1");
}

gboolean g_mpd_set_random(GMpd *self, gboolean newRandow) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return _send_command_simple(priv, "random %d\n",  newRandow ? 1 : 0);
}

gboolean g_mpd_get_repeat(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return !g_strcmp0(g_hash_table_lookup(priv->stateinfo, "repeat"), "1");
}

gboolean g_mpd_set_repeat(GMpd *self, gboolean newRepeat) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), FALSE);
	priv = G_MPD_GET_PRIVATE(self);
	return _send_command_simple(priv, "repeat %d\n",  newRepeat ? 1 : 0);
}

GError * g_mpd_get_last_error(GMpd *self) {
	GMpdPrivate *priv;
	g_return_val_if_fail (G_IS_MPD (self), NULL);
	priv = G_MPD_GET_PRIVATE(self);
	
	return (priv->error) ? g_error_copy(priv->error) : NULL;
}

/* endregion */
