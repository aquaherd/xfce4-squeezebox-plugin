/*
 * 		gmpd-private.h
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

/* inclusion guard */
#ifndef __G_MPD_PRIVATE_H__
#define __G_MPD_PRIVATE_H__

static GQuark G_MPD_DOMAIN;
#define G_MPD_ERROR 4711

static void _clear_object(GObject **socket);
static void _error_clear(GMpdPrivate *priv);
static gboolean _error_extract(GMpdPrivate *priv, const gchar* line);
static gchar * _read_response_line(GMpdPrivate *priv);
static gboolean _send_command_raw(GMpdPrivate *priv, const gchar* command);
static void _update_dispatch(GMpdPrivate *priv, gchar *changeDetail);
static void _idle_cb(GObject *source_object, GAsyncResult *res,  gpointer user_data);
static void _idle_enter(GMpdPrivate *priv);
static void _idle_cancel(GMpdPrivate *priv);
static gboolean _send_command_simple(GMpdPrivate *priv, const gchar* format, ...);
static void _update_playlists(GMpdPrivate *priv, gchar *changeDetail);
static void _update_status(GMpdPrivate *priv, gchar *changeDetail);
static void _update_currentsong(GMpdPrivate *priv, gchar *changeDetail);

#endif
