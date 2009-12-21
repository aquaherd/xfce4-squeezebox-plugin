/*		
 * 		gmpd.h
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
#ifndef __G_MPD_H__
#define __G_MPD_H__

#include <glib-object.h>
/*
 * Potentially, include other headers on which this header depends.
 */

/*
 * Type macros.
 */
#define G_MPD_TYPE                  (g_mpd_get_type ())
#define G_MPD(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_MPD_TYPE, GMpd))
#define G_IS_MPD(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_MPD_TYPE))
#define G_MPD_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), G_MPD_TYPE, GMpdClass))
#define G_IS_MPD_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), G_MPD_TYPE))
#define G_MPD_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), G_MPD_TYPE, GMpdClass))

typedef struct _GMpd        GMpd;
typedef struct _GMpdClass   GMpdClass;
typedef struct _GMpdPrivate GMpdPrivate;

struct _GMpd
{
  GObject parent_instance;

  /* instance members */
  /*< private >*/
  GMpdPrivate *priv;
};

struct _GMpdClass
{
  GObjectClass parent_class;

  /* class members */
};

/* used by G_MPD_TYPE */
GType g_mpd_get_type (void);

/*
 * Method definitions.
 */
GMpd *g_mpd_new(void);
gboolean g_mpd_connect(GMpd *object, const gchar* host, const int port);
void g_mpd_disconnect(GMpd *object);
#endif /* __G_MPD_H__ */
