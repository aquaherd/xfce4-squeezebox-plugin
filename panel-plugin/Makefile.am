plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-squeezebox-plugin

xfce4_squeezebox_plugin_CFLAGS = 	-DDATA_DIR=\"$(datadir)/xfce4/xfce4-squeezebox-plugin\"		\
    -DPACKAGE_LOCALE_DIR=\"$(localedir)\" @LIBXFCE4PANEL_CFLAGS@ @DBUSGLIB_CFLAGS@ @LIBMPD_CFLAGS@		@NOTIFY_CFLAGS@  @THUNARVFS_CFLAGS@

xfce4_squeezebox_plugin_LDFLAGS = @LIBXFCE4PANEL_LIBS@ @DBUSGLIB_LIBS@ @LIBMPD_LIBS@ \
	@NOTIFY_LIBS@ @THUNARVFS_LIBS@

xfce4_squeezebox_plugin_SOURCES = squeezebox.c  squeezebox-dbus.c squeezebox-mpd.c squeezebox-ql.c mmkeys.c
	

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
# 
desktop_in_in_files = squeezebox.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" $< > $@
	
EXTRA_DIST = $(desktop_in_in_files)

DISTCLEANFILES = $(desktop_DATA) $(desktop_in_files)
