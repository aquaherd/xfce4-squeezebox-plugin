/* vi:set expandtab sw=2 sts=2: */
/*-
 * Copyright (c) 2008 Jannis Pohlmann <jannis@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include <X11/Xlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "xfce-shortcuts-grabber.h"



#define XFCE_SHORTCUTS_GRABBER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFCE_TYPE_SHORTCUTS_GRABBER, XfceShortcutsGrabberPrivate))



#define MODIFIER_MASK (GDK_SHIFT_MASK | \
                       GDK_CONTROL_MASK | \
                       GDK_MOD1_MASK | \
                       GDK_MOD2_MASK | \
                       GDK_MOD3_MASK | \
                       GDK_MOD4_MASK | \
                       GDK_MOD5_MASK)

#define IGNORE_MASK   (0x2000 | \
                       GDK_LOCK_MASK | \
                       GDK_HYPER_MASK | \
                       GDK_SUPER_MASK | \
                       GDK_META_MASK)



/* Property identifiers */
enum
{
  PROP_0,
};



/* Cache indices for modifiers */
enum
{
  CACHE_SUPER,
  CACHE_HYPER,
  CACHE_META,
  CACHE_CAPS_LOCK,
  CACHE_NUM_LOCK,
  CACHE_SCROLL_LOCK,
  CACHE_LAST,
};



typedef struct _XfceKey XfceKey;



static void            xfce_shortcuts_grabber_class_init       (XfceShortcutsGrabberClass *klass);
static void            xfce_shortcuts_grabber_init             (XfceShortcutsGrabber      *grabber);
static void            xfce_shortcuts_grabber_constructed      (GObject                   *object);
static void            xfce_shortcuts_grabber_finalize         (GObject                   *object);
static void            xfce_shortcuts_grabber_get_property     (GObject                   *object,
                                                                guint                      prop_id,
                                                                GValue                    *value,
                                                                GParamSpec                *pspec);
static void            xfce_shortcuts_grabber_set_property     (GObject                   *object,
                                                                guint                      prop_id,
                                                                const GValue              *value,
                                                                GParamSpec                *pspec);
static void            xfce_shortcuts_grabber_keys_changed     (GdkKeymap                 *keymap,
                                                                XfceShortcutsGrabber      *grabber);
static void            xfce_shortcuts_grabber_grab_all         (XfceShortcutsGrabber      *grabber);
static void            xfce_shortcuts_grabber_ungrab_all       (XfceShortcutsGrabber      *grabber);
static void            xfce_shortcuts_grabber_reload_modifiers (XfceShortcutsGrabber      *grabber);
static void            xfce_shortcuts_grabber_parse_shortcut   (XfceShortcutsGrabber      *grabber,
                                                                const gchar               *shortcut,
                                                                guint                     *keycode,
                                                                guint                     *modifiers);
static void            xfce_shortcuts_grabber_grab             (XfceShortcutsGrabber      *grabber,
                                                                XfceKey                   *key,
                                                                gboolean                   grab);
static guint           xfce_shortcuts_grabber_get_ignore_mask  (XfceShortcutsGrabber      *grabber);
static GdkFilterReturn xfce_shortcuts_grabber_event_filter     (GdkXEvent                 *gdk_xevent,
                                                                GdkEvent                  *event,
                                                                XfceShortcutsGrabber      *grabber);



struct _XfceShortcutsGrabberPrivate
{
  GHashTable *keys;
  guint       modifiers[CACHE_LAST];
};

struct _XfceKey
{
  guint keycode;
  guint modifiers;
};



static GObjectClass *xfce_shortcuts_grabber_parent_class = NULL;



GType
xfce_shortcuts_grabber_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info =
      {
        sizeof (XfceShortcutsGrabberClass),
        NULL,
        NULL,
        (GClassInitFunc) xfce_shortcuts_grabber_class_init,
        NULL,
        NULL,
        sizeof (XfceShortcutsGrabber),
        0,
        (GInstanceInitFunc) xfce_shortcuts_grabber_init,
        NULL,
      };

      type = g_type_register_static (G_TYPE_OBJECT, "XfceShortcutsGrabber", &info, 0);
    }

  return type;
}



static void
xfce_shortcuts_grabber_class_init (XfceShortcutsGrabberClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (XfceShortcutsGrabberPrivate));

  /* Determine the parent type class */
  xfce_shortcuts_grabber_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
#if GLIB_CHECK_VERSION (2,14,0)
  gobject_class->constructed = xfce_shortcuts_grabber_constructed; 
#endif
  gobject_class->finalize = xfce_shortcuts_grabber_finalize; 
  gobject_class->get_property = xfce_shortcuts_grabber_get_property;
  gobject_class->set_property = xfce_shortcuts_grabber_set_property;

  g_signal_new ("shortcut-activated",
                XFCE_TYPE_SHORTCUTS_GRABBER,
                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                0,
                NULL,
                NULL,
                g_cclosure_marshal_VOID__STRING,
                G_TYPE_NONE,
                1,
                G_TYPE_STRING);
}



static void
xfce_shortcuts_grabber_init (XfceShortcutsGrabber *grabber)
{
  grabber->priv = XFCE_SHORTCUTS_GRABBER_GET_PRIVATE (grabber);
  grabber->priv->keys = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  xfce_shortcuts_grabber_reload_modifiers (grabber);
}



static void
xfce_shortcuts_grabber_constructed (GObject *object)
{
  GdkDisplay *display;
  GdkKeymap  *keymap;

  XfceShortcutsGrabber *grabber = XFCE_SHORTCUTS_GRABBER (object);

  keymap = gdk_keymap_get_default ();
  g_signal_connect (keymap, "keys-changed", G_CALLBACK (xfce_shortcuts_grabber_keys_changed), 
                    grabber);

  display = gdk_display_get_default ();

  /* Flush events before adding the event filter */
  XAllowEvents (GDK_DISPLAY_XDISPLAY (display), AsyncBoth, CurrentTime);

  /* Add event filter */
  gdk_window_add_filter (NULL, (GdkFilterFunc) xfce_shortcuts_grabber_event_filter, grabber);
}



static void
xfce_shortcuts_grabber_finalize (GObject *object)
{
  XfceShortcutsGrabber *grabber = XFCE_SHORTCUTS_GRABBER (object);

  xfce_shortcuts_grabber_ungrab_all (grabber);
  g_hash_table_unref (grabber->priv->keys);

  (*G_OBJECT_CLASS (xfce_shortcuts_grabber_parent_class)->finalize) (object);
}



static void
xfce_shortcuts_grabber_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
#if 0
  XfceShortcutsGrabber *grabber = XFCE_SHORTCUTS_GRABBER (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_shortcuts_grabber_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
#if 0
  XfceShortcutsGrabber *grabber = XFCE_SHORTCUTS_GRABBER (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_shortcuts_grabber_keys_changed (GdkKeymap            *keymap,
                                     XfceShortcutsGrabber *grabber)
{
  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));

  xfce_shortcuts_grabber_ungrab_all (grabber);
  xfce_shortcuts_grabber_reload_modifiers (grabber);
  xfce_shortcuts_grabber_grab_all (grabber);
}



static gboolean
grab_key (const gchar          *shortcut,
          XfceKey              *key,
          XfceShortcutsGrabber *grabber)
{
  xfce_shortcuts_grabber_grab (grabber, key, TRUE);
  return FALSE;
}



static void
xfce_shortcuts_grabber_grab_all (XfceShortcutsGrabber *grabber)
{
  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));
  g_hash_table_foreach (grabber->priv->keys, (GHFunc) grab_key, grabber);
}



static gboolean
ungrab_key (const gchar          *shortcut,
            XfceKey              *key,
            XfceShortcutsGrabber *grabber)
{
  xfce_shortcuts_grabber_grab (grabber, key, FALSE);
  return FALSE;
}



static void
xfce_shortcuts_grabber_ungrab_all (XfceShortcutsGrabber *grabber)
{
  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));
  g_hash_table_foreach (grabber->priv->keys, (GHFunc) ungrab_key, grabber);
}



static void
xfce_shortcuts_grabber_reload_modifiers (XfceShortcutsGrabber *grabber)
{
  XModifierKeymap *modmap;
  const KeySym    *keysyms;
  Display         *display;
  KeyCode          keycode;
  KeySym          *keymap;
  gint             keysyms_per_keycode = 0;
  gint             min_keycode = 0;
  gint             max_keycode = 0;
  gint             mask;
  gint             i;
  gint             j;

  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));

  for (i = 0; i < CACHE_LAST; ++i)
    grabber->priv->modifiers[i] = 0;

  display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

  gdk_error_trap_push ();
  
  XDisplayKeycodes (display, &min_keycode, &max_keycode);

  keymap = XGetKeyboardMapping (display, min_keycode, max_keycode - min_keycode + 1, 
                                &keysyms_per_keycode);

  if (G_UNLIKELY (keymap == NULL))
    return;

  modmap = XGetModifierMapping (display);

  if (G_UNLIKELY (modmap == NULL))
    {
      XFree (keymap);
      return;
    }

  for (i = 0; i < 8 * modmap->max_keypermod; ++i)
    {
      keycode = modmap->modifiermap[i];

      if (keycode == 0 || keycode < min_keycode || keycode > max_keycode)
        continue;

      keysyms = keymap + (keycode - min_keycode) * keysyms_per_keycode;
      mask = 1 << (i / modmap->max_keypermod);

      for (j = 0; j < keysyms_per_keycode; ++j)
        {
          if (keysyms[j] == GDK_Super_L || keysyms[j] == GDK_Super_R)
            grabber->priv->modifiers[CACHE_SUPER] = mask;

          if (keysyms[j] == GDK_Meta_L || keysyms[j] == GDK_Meta_R)
            grabber->priv->modifiers[CACHE_META] = mask;

          if (keysyms[j] == GDK_Hyper_L || keysyms[j] == GDK_Hyper_R)
            grabber->priv->modifiers[CACHE_HYPER] = mask;

          if (keysyms[j] == GDK_Scroll_Lock)
            grabber->priv->modifiers[CACHE_SCROLL_LOCK] = mask;

          if (keysyms[j] == GDK_Num_Lock)
            grabber->priv->modifiers[CACHE_NUM_LOCK] = mask;

          if (keysyms[j] == GDK_Caps_Lock)
            grabber->priv->modifiers[CACHE_CAPS_LOCK] = mask;
        }
    }

  XFreeModifiermap (modmap);
  XFree (keymap);

  gdk_flush ();
  gdk_error_trap_pop ();
}



static void
xfce_shortcuts_grabber_parse_shortcut (XfceShortcutsGrabber *grabber,
                                       const gchar          *shortcut,
                                       guint                *keycode,
                                       guint                *modifiers)
{
  guint keyval;

  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));

  gtk_accelerator_parse (shortcut, &keyval, modifiers);

  *keycode = XKeysymToKeycode (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), keyval);

  if ((*modifiers & GDK_SUPER_MASK) == GDK_SUPER_MASK)
    {
      *modifiers |= grabber->priv->modifiers[CACHE_SUPER];
      *modifiers ^= GDK_SUPER_MASK;
    }

  if ((*modifiers & GDK_HYPER_MASK) == GDK_HYPER_MASK)
    {
      *modifiers |= grabber->priv->modifiers[CACHE_HYPER];
      *modifiers ^= GDK_HYPER_MASK;
    }

  if ((*modifiers & GDK_META_MASK) == GDK_META_MASK)
    {
      *modifiers |= grabber->priv->modifiers[CACHE_META];
      *modifiers ^= GDK_META_MASK;
    }

  *modifiers &= MODIFIER_MASK;
  *modifiers &= ~xfce_shortcuts_grabber_get_ignore_mask (grabber);
}



gchar *
xfce_shortcuts_grabber_shortcut_name (XfceShortcutsGrabber *grabber,
                                      guint                 keycode,
                                      guint                 modifiers)
{
  Display *display;
  KeySym   keysym;

  g_return_val_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber), NULL);

  display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
  keysym = XKeycodeToKeysym (display, keycode, 0);

  modifiers &= MODIFIER_MASK;
  modifiers &= ~xfce_shortcuts_grabber_get_ignore_mask (grabber);

  if ((modifiers & grabber->priv->modifiers[CACHE_SUPER]) != 0)
    {
      modifiers |= GDK_SUPER_MASK;
      modifiers ^= grabber->priv->modifiers[CACHE_SUPER];
    }

  if ((modifiers & grabber->priv->modifiers[CACHE_HYPER]) != 0)
    {
      modifiers |= GDK_HYPER_MASK;
      modifiers ^= grabber->priv->modifiers[CACHE_HYPER];
    }

#if 0
  if ((modifiers & grabber->priv->modifiers[CACHE_META]) != 0)
    {
      modifiers |= GDK_META_MASK;
      modifiers ^= grabber->priv->modifiers[CACHE_META];
    }
#endif

  return gtk_accelerator_name (keysym, modifiers);
}



static void
xfce_shortcuts_grabber_grab (XfceShortcutsGrabber *grabber,
                             XfceKey              *key,
                             gboolean              grab)
{
  GdkDisplay *display;
  GdkScreen  *screen;
  Window      window;
  guint       bits[32];
  guint       current_mask;
  guint       n_bits;
  guint       screens;
  guint       modifiers;
  guint       ignored_modifiers = 0;
  gint        i;
  guint       j;
  guint       k;

  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));
  g_return_if_fail (key != NULL);

  display = gdk_display_get_default ();
  screens = gdk_display_get_n_screens (display);

  ignored_modifiers = xfce_shortcuts_grabber_get_ignore_mask (grabber);

  modifiers = key->modifiers & MODIFIER_MASK & ~ignored_modifiers;

  /* Store indices of all the set bits of the ignore mask in an array */
  for (i = 0, n_bits = 0; i < 32; ++i, ignored_modifiers >>= 1)
    if ((ignored_modifiers & 0x1) == 0x1)
      bits[n_bits++] = i;

  for (i = 0; i < (1 << n_bits); ++i)
    {
      /* Map bits in the counter to those in the mask and thereby retrieve all ignored bit
       * mask combinations */
      for (current_mask = 0, j = 0; j < n_bits; ++j)
        if ((i & (1 << j)) != 0)
          current_mask |= (1 << bits[j]);

      /* Grab key on all screens */
      for (k = 0; k < screens; ++k)
        {
          /* Get current screen and X root window */
          screen = gdk_display_get_screen (display, k);
          window = GDK_WINDOW_XWINDOW (gdk_screen_get_root_window (screen));

          gdk_error_trap_push ();

          if (grab)
            {
              XGrabKey (GDK_DISPLAY_XDISPLAY (display), key->keycode, current_mask | modifiers, 
                        window, FALSE, GrabModeAsync, GrabModeAsync);
            }
          else
            XUngrabKey (GDK_DISPLAY_XDISPLAY (display), key->keycode, current_mask | modifiers, 
                        window);

          gdk_flush ();
          gdk_error_trap_pop ();
        }
    }
}



static guint
xfce_shortcuts_grabber_get_ignore_mask (XfceShortcutsGrabber *grabber)
{
  guint mask = 0;

  g_return_val_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber), 0);
  
  mask |= 0x200 | GDK_LOCK_MASK | GDK_HYPER_MASK | GDK_SUPER_MASK | GDK_META_MASK;
  mask |= grabber->priv->modifiers[CACHE_CAPS_LOCK];
  mask |= grabber->priv->modifiers[CACHE_NUM_LOCK];
  mask |= grabber->priv->modifiers[CACHE_SCROLL_LOCK];

  return mask;
}



struct EventKeyFindContext
{
  XfceShortcutsGrabber *grabber;
  XKeyEvent            *xevent;
  const gchar          *result;
};



static gboolean
find_event_key (const gchar                *shortcut,
                XfceKey                    *key,
                struct EventKeyFindContext *context)
{
  gchar   *name;
  gboolean result = FALSE;

  g_return_val_if_fail (context != NULL, TRUE);
  g_return_val_if_fail (context->xevent != NULL, TRUE);
  
  gdk_error_trap_push ();

  name = xfce_shortcuts_grabber_shortcut_name (context->grabber, context->xevent->keycode, 
                                               context->xevent->state);

  if (G_UNLIKELY (g_str_equal (shortcut, name)))
    {
      context->result = shortcut;
      result = TRUE;
    }

  g_free (name);

  gdk_flush ();
  gdk_error_trap_pop ();

  return result;
}



static GdkFilterReturn
xfce_shortcuts_grabber_event_filter (GdkXEvent            *gdk_xevent,
                                     GdkEvent             *event,
                                     XfceShortcutsGrabber *grabber)
{
  struct EventKeyFindContext context;
  XEvent                    *xevent;

  g_return_val_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber), GDK_FILTER_CONTINUE);

  xevent = (XEvent *) gdk_xevent;

  if (xevent->type != KeyPress)
    return GDK_FILTER_CONTINUE;

  context.grabber = grabber;
  context.xevent = (XKeyEvent *) xevent;
  context.result = NULL;

  g_hash_table_foreach (grabber->priv->keys, (GHFunc) find_event_key, &context);

  if (G_LIKELY (context.result != NULL))
    g_signal_emit_by_name (grabber, "shortcut-activated", context.result);

  return GDK_FILTER_CONTINUE;
}



XfceShortcutsGrabber *
xfce_shortcuts_grabber_new (void)
{
  GObject *object;

  object = g_object_new (XFCE_TYPE_SHORTCUTS_GRABBER, NULL);

#if !GLIB_CHECK_VERSION (2,14,0)
  xfce_shortcuts_grabber_constructed (object);
#endif

  return XFCE_SHORTCUTS_GRABBER (object);
}



void
xfce_shortcuts_grabber_add (XfceShortcutsGrabber *grabber,
                            const gchar          *shortcut)
{
  XfceKey *key;

  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));
  g_return_if_fail (shortcut != NULL);

  key = g_new0 (XfceKey, 1);
  
  xfce_shortcuts_grabber_parse_shortcut (grabber, shortcut, &key->keycode, &key->modifiers);

  if (G_LIKELY (key->keycode != 0))
    {
      xfce_shortcuts_grabber_grab (grabber, key, TRUE);
      g_hash_table_insert (grabber->priv->keys, g_strdup (shortcut), key);
    }
}



void
xfce_shortcuts_grabber_remove (XfceShortcutsGrabber *grabber,
                               const gchar          *shortcut)
{
  XfceKey *key;

  g_return_if_fail (XFCE_IS_SHORTCUTS_GRABBER (grabber));
  g_return_if_fail (shortcut != NULL);

  key = g_hash_table_lookup (grabber->priv->keys, shortcut);

  if (G_LIKELY (key != NULL))
    {
      xfce_shortcuts_grabber_grab (grabber, key, FALSE);
      g_hash_table_remove (grabber->priv->keys, shortcut);
    }
}
