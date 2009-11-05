/*
 *      notifications.h
 *      
 *      Copyright 2009 Hakan Erduman <hakan@erduman.de>
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


#ifndef SQUEEZEBOX_NOTIFICATIONS_H
#define SQUEEZEBOX_NOTIFICATIONS_H
#include "squeezebox.h"
#include "squeezebox-private.h"
void toaster_closed(NotifyNotification * notification, SqueezeBoxData * sd);
void squeezebox_update_UI_hide_toaster(gpointer thsPlayer);
gboolean on_timer(gpointer thsPlayer);
void squeezebox_update_UI_show_toaster(gpointer thsPlayer);
#endif
