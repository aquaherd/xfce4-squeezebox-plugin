/***************************************************************************
 *            settings.c - settings handler of xfce4-squeezebox-plugin
 *
 *  Copyright  2006-2014  Hakan Erduman
 *  Email hakan@erduman.de
 ****************************************************************************
 * $Rev:: 225         $: Revision of last commit
 * $Author:: herd     $: Author of last commit
 * $Date:: 2014-08-31#$: Date of last commit
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include "squeezebox.h"
#include "squeezebox-private.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gmodule.h>
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>

typedef enum eColumns
{
   PIXBUF_COLUMN, TEXT_COLUMN, INDEX_COLUMN, AVAIL_COLUMN, N_COLUMNS,
} eColumns;

EXPORT void on_dialogSettings_response(GtkWidget * dlg, int reponse,
      SqueezeBoxData * sd)
{
   LOG("Enter DialogResponse");
   gtk_widget_destroy(dlg);
   xfce_panel_plugin_unblock_menu(sd->plugin);
   squeezebox_write_rc_file(sd->plugin, sd);
   sd->dlg = NULL;
   LOG("Leave DialogResponse");
}

EXPORT void on_btnAdd_clicked(GtkButton * btn, SqueezeBoxData * sd)
{
}
EXPORT void on_btnRemove_clicked(GtkButton * btn, SqueezeBoxData * sd)
{
}
EXPORT void on_btnEdit_clicked(GtkButton * btn, SqueezeBoxData * sd)
{
   LOG("Enter config_show_backend_properties");
   if (sd->player.Configure)
      sd->player.Configure(sd->player.db, GTK_WIDGET(sd->dlg));
   else
   {
      GtkWidget *dlg = gtk_message_dialog_new(
            GTK_WINDOW(g_object_get_data (G_OBJECT(sd->plugin), "dialog")),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("This backend has no configurable properties"));

      gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(sd->dlg));
      gtk_dialog_run(GTK_DIALOG(dlg));
      gtk_widget_destroy(dlg);
   }
   LOG("Leave config_show_backend_properties");
}

EXPORT void on_chkAutoAttach_toggled(GtkToggleButton *button,
      SqueezeBoxData * sd)
{
   sd->autoAttach = gtk_toggle_button_get_active(button);
   gtk_tree_view_column_set_visible(
         GTK_TREE_VIEW_COLUMN(
               g_object_get_data(G_OBJECT(sd->dlg), "tvcEnabled")),
         sd->autoAttach);
}

EXPORT void on_chkShowPrevious_toggled(GtkToggleButton *button,
      SqueezeBoxData *sd)
{
   gboolean *pBtn = &sd->show[ebtnPrev];
   gint size = xfce_panel_plugin_get_size(sd->plugin);

   *pBtn = gtk_toggle_button_get_active(button);

   if (*pBtn)
      gtk_widget_show(sd->button[ebtnPrev]);
   else
      gtk_widget_hide(sd->button[ebtnPrev]);

   squeezebox_set_size(sd->plugin, size, sd);
}

EXPORT void on_chkShowNext_toggled(GtkToggleButton *button, SqueezeBoxData *sd)
{
   gboolean *pBtn = &sd->show[ebtnNext];
   gint size = xfce_panel_plugin_get_size(sd->plugin);

   *pBtn = gtk_toggle_button_get_active(button);

   if (*pBtn)
      gtk_widget_show(sd->button[ebtnNext]);
   else
      gtk_widget_hide(sd->button[ebtnNext]);

   squeezebox_set_size(sd->plugin, size, sd);
}

EXPORT void on_chkShowToolTips_toggled(GtkToggleButton *button,
      SqueezeBoxData *sd)
{
   if (gtk_toggle_button_get_active(button))
   {
      sd->toolTipStyle = ettSimple;
   }
   else
   {
      sd->toolTipStyle = ettNone;
   }
}

EXPORT void on_chkShowNotifications_toggled(GtkToggleButton *button,
      SqueezeBoxData *sd)
{
   sd->notify = gtk_toggle_button_get_active(button);
}

EXPORT void on_spinNotificationTimeout_change_value(GtkSpinButton *button,
      SqueezeBoxData *sd)
{
   sd->notifyTimeout = gtk_spin_button_get_value(button);
}

EXPORT void on_tvPlayers_cursor_changed(GtkTreeView * tv, SqueezeBoxData * sd)
{
   if (!sd->autoAttach)
   {
      GtkTreeIter iter =
      { 0 };
      GtkTreeSelection *selection = gtk_tree_view_get_selection(tv);
      GtkTreeModel *model = NULL;
      GtkWidget *button = GTK_WIDGET(
            g_object_get_data(G_OBJECT(sd->dlg), "btnEdit"));
      LOG("Backend change...");
      if (gtk_tree_selection_get_selected(selection, &model, &iter))
      {
         LOG("Have iter %d", iter.stamp);
         if (model)
         {
            gchar *backendLoc = NULL;
            gchar *backend = NULL;
            gtk_tree_model_get(model, &iter, TEXT_COLUMN, &backendLoc,
                  INDEX_COLUMN, &backend, -1);
            LOG("Have model %s", backendLoc);
            squeezebox_init_backend(sd, backend);
            gtk_widget_set_sensitive(button,
            NULL != sd->player.Configure);
         }
      }
   }
}

EXPORT void on_cellrenderertoggle1_toggled(GtkCellRendererToggle * crt,
      gchar *path_string, SqueezeBoxData * sd)
{
   GtkTreeIter iter =
   { 0 };
   GtkTreeView *view = GTK_TREE_VIEW(
         g_object_get_data(G_OBJECT(sd->dlg), "tvPlayers"));
   GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
   GtkTreeModel *model = NULL;
   gboolean active = !gtk_cell_renderer_toggle_get_active(crt);
   LOG("ClickCRT %d", active);
   if (gtk_tree_selection_get_selected(selection, &model, &iter))
   {
      gchar *propAuto = NULL;
      gchar *backend = NULL;
      gtk_tree_model_get(model, &iter, 2, &backend, -1);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, AVAIL_COLUMN, active,
            -1);
      propAuto = g_strconcat("/AutoConnects/", backend, NULL);
      xfconf_channel_set_bool(sd->channel, propAuto, active);
      g_free(propAuto);
      g_free(backend);
   }
}

EXPORT void on_cellrenderShortCut_accel_cleared(GtkCellRendererAccel *accel,
      gchar *path_string, SqueezeBoxData *sd)
{
   GtkListStore *storeShortCuts = g_object_get_data(G_OBJECT(sd->dlg),
         "liststoreShortcuts");
   GtkTreeIter iter =
   { 0 };
   gchar *path1 = NULL;

   LOG("Dropped key #%s", path_string);
   if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(storeShortCuts),
         &iter, path_string))
   {
      guint accelKey;
      GdkModifierType accelKeyMask;
      gint idx = (gint) g_strtod(path_string, NULL);
      sd->shortcuts[idx] = 0;
      gtk_tree_model_get(GTK_TREE_MODEL(storeShortCuts), &iter, 2,
            &accelKeyMask, 1, &accelKey, -1);
      path1 = gtk_accelerator_name(accelKey, accelKeyMask);
      keybinder_unbind(path1, (KeybinderHandler) on_shortcutActivated);
      gtk_list_store_set(storeShortCuts, &iter, 1, 0, 2, 0, -1);
      path1 = g_strdup_printf("/MediaKeys/Key%s", path_string);
      xfconf_channel_set_string(sd->channel, path1, "");
      g_free(path1);
   }
   sd->inShortcutEdit = FALSE;
}

EXPORT void on_cellrendererShortCut_editing_started(GtkCellRenderer *renderer,
      GtkCellEditable *editable, gchar *path, SqueezeBoxData *sd)
{
   sd->inShortcutEdit = TRUE;
}

EXPORT void on_cellrenderShortCut_accel_edited(GtkCellRendererAccel *accel,
      gchar *path_string, guint accel_key, GdkModifierType accel_mods,
      guint hardware_keycode, SqueezeBoxData *sd)
{
   GtkListStore *storeShortCuts = g_object_get_data(G_OBJECT(sd->dlg),
         "liststoreShortcuts");
   GtkTreeIter iter =
   { 0 };
   gchar *path1 = NULL;
   gchar *path2 = NULL;
   LOG("Accel %s %d %d", path_string, accel_key, accel_mods);
   if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(storeShortCuts),
         &iter, path_string))
   {
      guint accelKey;
      GdkModifierType accelKeyMask;
      gint idx = (gint) g_strtod(path_string, NULL);
      gtk_tree_model_get(GTK_TREE_MODEL(storeShortCuts), &iter, 2,
            &accelKeyMask, 1, &accelKey, -1);
      path1 = gtk_accelerator_name(accelKey, accelKeyMask);
      path2 = gtk_accelerator_name(accel_key, accel_mods);
      keybinder_unbind(path1, (KeybinderHandler) on_shortcutActivated);
      if (!keybinder_bind_full(path1, (KeybinderHandler) on_shortcutActivated,
            sd, NULL))
      {
         GtkWidget *dlg = gtk_message_dialog_new(
               GTK_WINDOW(g_object_get_data (G_OBJECT(sd->plugin), "dialog")),
               GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
               _("This shortcut is already taken"));

         gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(sd->dlg));
         gtk_dialog_run(GTK_DIALOG(dlg));
         gtk_widget_destroy(dlg);
      }
      sd->shortcuts[idx] = g_quark_from_string(path2);
      LOG("Changed key #%s from %s with %s", path_string, path1, path2);
      g_free(path1);
      gtk_list_store_set(storeShortCuts, &iter, 2, accel_mods, 1, accel_key,
            -1);
      path1 = g_strdup_printf("/MediaKeys/Key%s", path_string);
      xfconf_channel_set_string(sd->channel, path1, path2);
      g_free(path1);
      g_free(path2);
   }
   sd->inShortcutEdit = FALSE;
}

static void squeezebox_connect_signals(GtkBuilder *builder, GObject *object,
      const gchar *signal_name, const gchar *handler_name,
      GObject *connect_object, GConnectFlags flags, gpointer user_data)
{
#define MAP_SIG(x) \
      if(!g_strcmp0(handler_name, #x)) { \
         g_signal_connect(object, signal_name, G_CALLBACK(x), user_data); \
         bFound = TRUE; \
         }
   gboolean bFound = FALSE;
   MAP_SIG(on_dialogSettings_response)
   MAP_SIG(on_cellrenderShortCut_accel_edited)
   MAP_SIG(on_cellrendererShortCut_editing_started)
   MAP_SIG(on_cellrenderShortCut_accel_cleared)
   MAP_SIG(on_spinNotificationTimeout_change_value)
   MAP_SIG(on_chkShowNotifications_toggled)
   MAP_SIG(on_chkShowToolTips_toggled)
   MAP_SIG(on_chkShowNext_toggled)
   MAP_SIG(on_chkShowPrevious_toggled)
   MAP_SIG(on_btnEdit_clicked)
   MAP_SIG(on_btnRemove_clicked)
   MAP_SIG(on_btnAdd_clicked)
   MAP_SIG(on_tvPlayers_cursor_changed)
   MAP_SIG(on_cellrenderertoggle1_toggled)
   MAP_SIG(on_chkAutoAttach_toggled)
   if (!bFound)
   {
      LOGWARN("Can't connect signal %s to handler %s", signal_name, handler_name);
   }
}

void squeezebox_properties_dialog(XfcePanelPlugin * plugin, SqueezeBoxData * sd)
{
   // backends
   GtkListStore *store;
   GtkTreeIter iter =
   { 0 };
   GList *list = g_list_first(sd->list);
   GdkPixbuf *pix;
   GtkTreeView *view;
   gboolean valid;
   gint idx;
   GtkBuilder* builder;
   GError *error = NULL;
   LOG("Enter squeezebox_properties_dialog " GLADEDIR "/settings.ui");

   if (xfce_titled_dialog_get_type() == 0)
   {
      LOGWARN("Can't build UI: XfceTitledDialog missing.");
      return;
   }

   // new
   builder = gtk_builder_new();
   if (!gtk_builder_add_from_file(builder, GLADEDIR "/settings.ui", &error))
   {
      LOGWARN("Can't add builder: %s", error->message);
      return;
   }
   xfce_panel_plugin_block_menu(plugin);
   sd->dlg = GTK_WIDGET(gtk_builder_get_object(builder, "dialogSettings"));
   store = GTK_LIST_STORE(gtk_builder_get_object(builder, "datastore"));
   view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tvPlayers"));
   // the button sensitivity update
   g_object_set_data(G_OBJECT(sd->dlg), "btnAdd",
         gtk_builder_get_object(builder, "btnAdd"));
   g_object_set_data(G_OBJECT(sd->dlg), "btnEdit",
         gtk_builder_get_object(builder, "btnEdit"));
   g_object_set_data(G_OBJECT(sd->dlg), "btnRemove",
         gtk_builder_get_object(builder, "btnRemove"));
   //cellrenderertoggle1
   g_object_set_data(G_OBJECT(sd->dlg), "tvcEnabled",
         gtk_builder_get_object(builder, "tvcEnabled"));
   g_object_set_data(G_OBJECT(sd->dlg), "tvPlayers",
         gtk_builder_get_object(builder, "tvPlayers"));
   g_object_set_data(G_OBJECT(sd->dlg), "liststoreShortcuts",
         gtk_builder_get_object(builder, "liststoreShortcuts"));
   g_object_set_data(G_OBJECT(sd->dlg), "datastore",
         gtk_builder_get_object(builder, "datastore"));

   /* fill the backend store with data */
   while (list)
   {
      BackendCache *ptr = (BackendCache*) list->data;
      /*
       if(ptr->commandLine) {
       if(!g_find_program_in_path(ptr->commandLine)) {
       LOG("     %s seems not installed", ptr->commandLine);
       list = g_list_next(list);
       continue;
       }
       }
       */
      LOG("Have %s", ptr->basename);
      gtk_list_store_append(store, &iter);
      pix = exo_gdk_pixbuf_scale_down(ptr->icon, TRUE, 24, 32);
      gtk_list_store_set(store, &iter, PIXBUF_COLUMN, pix, TEXT_COLUMN,
            ptr->name, INDEX_COLUMN, ptr->basename, AVAIL_COLUMN,
            ptr->autoAttach, -1);
      if (sd->current && !g_utf8_collate(sd->current->basename, ptr->basename))
      {
         GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
         gtk_tree_selection_select_iter(selection, &iter);
         gtk_widget_set_sensitive(
               GTK_WIDGET((gtk_builder_get_object(builder, "btnEdit"))),
               NULL != sd->player.Configure);
      }

      list = g_list_next(list);
   }

   gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), TEXT_COLUMN,
         GTK_SORT_ASCENDING);

   // Appearance tab
   gtk_toggle_button_set_active(
         GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkAutoAttach")),
         sd->autoAttach);
   gtk_toggle_button_set_active(
         GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkShowPrevious")),
         sd->show[ebtnPrev]);
   gtk_toggle_button_set_active(
         GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkShowNext")),
         sd->show[ebtnNext]);
   gtk_toggle_button_set_active(
         GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkShowToolTips")),
         sd->toolTipStyle);
   gtk_toggle_button_set_active(
         GTK_TOGGLE_BUTTON(
               gtk_builder_get_object(builder, "chkShowNotifications")),
         sd->notify);
   gtk_spin_button_set_value(
         GTK_SPIN_BUTTON(
               gtk_builder_get_object(builder, "spinNotificationTimeout")),
         sd->notifyTimeout);
   gtk_tree_view_column_set_visible(
         GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "tvcEnabled")),
         sd->autoAttach);

   // Shortcuts tab
   store = GTK_LIST_STORE(
         gtk_builder_get_object(builder, "liststoreShortcuts"));
   valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
   idx = 0;
   while (valid)
   {
      guint accelKey;
      GdkModifierType accelKeyMask;
      gchar *name = NULL, *path1 = NULL, *path2 = NULL;
      gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &name, -1);
      path1 = g_strdup_printf("/MediaKeys/Key%d", idx);
      path2 = xfconf_channel_get_string(sd->channel, path1, "");
      gtk_accelerator_parse(path2, &accelKey, &accelKeyMask);
      gtk_list_store_set(store, &iter, 2, accelKeyMask, 1, accelKey, -1);
      valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
      idx++;
      g_free(path1);
      g_free(path2);
   }
   // liftoff
   gtk_builder_connect_signals_full(builder, squeezebox_connect_signals, sd);
   gtk_dialog_run(GTK_DIALOG(sd->dlg));
   LOG("Leave squeezebox_properties_dialog");
}

