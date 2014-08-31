INCLUDES = \
	-I$(top_srcdir) \
	-Wall -DDATA_DIR=\"$(datadir)/xfce4/xfce4-squeezebox-plugin\"		\
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DG_LOG_DOMAIN=\"SBP\" \
	-export-dynamic

# -fvisibility=hidden
	

plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libxfce4-squeezebox-plugin.la

libxfce4_squeezebox_plugin_la_CFLAGS = \
	$(LIBX11_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(GTK_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBEXO_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(PATH_DEFINES) \
	$(LIBGIO_CFLAGS) \
	$(LIBKEYBINDER_CFLAGS) \
	$(LIBID3TAG_CFLAGS) \
	$(LIBZEITGEIST_CFLAGS) \
	$(LIBXFCONF_CFLAGS)

libxfce4_squeezebox_plugin_la_LIBADD = \
	$(LIBX11_LIBS) \
	$(LIBXFCE4PANEL_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(GTK_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBEXO_LIBS) \
	$(LIBWNCK_LIBS) \
	$(LIBGTHREAD_LIBS) \
	$(LIBGIO_LIBS) \
	$(LIBKEYBINDER_LIBS) \
	$(LIBID3TAG_LIBS) \
	$(LIBZEITGEIST_LIBS) \
	$(LIBXFCONF_LIBS)

libxfce4_squeezebox_plugin_la_LDFLAGS = \
	-module -avoid-version \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'
#	-Wl -rdynamic


libxfce4_squeezebox_plugin_la_SOURCES = \
	squeezebox.h \
	squeezebox-private.h\
	squeezebox.c \
	notifications.c \
	notifications.h \
	notifications-binding.h
	

# .desktop file
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_files = squeezebox.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@


EXTRA_DIST = $(desktop_in_files)

DISTCLEANFILES = $(desktop_DATA)

splint:
	splint -warnposix +ignoresigns -stats \
	*.c  \
	$(libxfce4_squeezebox_plugin_la_CFLAGS)
