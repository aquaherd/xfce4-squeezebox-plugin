plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-squeezebox-plugin

xfce4_squeezebox_plugin_CFLAGS = -Wall -DDATA_DIR=\"$(datadir)/xfce4/xfce4-squeezebox-plugin\"		\
    -DPACKAGE_LOCALE_DIR=\"$(localedir)\" @LIBXFCE4PANEL_CFLAGS@ \
    $(LIBX11_CFLAGS) \
    @EXO_CFLAGS@ @LIBWNCK_CFLAGS@ $(PATH_DEFINES) \
    -DG_LOG_DOMAIN=\"SBP\" \
    @LIBGIO_CFLAGS@ @ID3TAG_CFLAGS@ @LIBXFCONF_CFLAGS@ \
    -export-dynamic -fvisibility=hidden

#todo: use --dynamic-list=someFile instead of -Wl -rdynamic
#    -Wl -rdynamic 

xfce4_squeezebox_plugin_LDFLAGS = @LIBXFCE4PANEL_LIBS@ @LIBXFCONF_LIBS@ \
	$(LIBX11_LIBS) \
	@EXO_LIBS@ @LIBWNCK_LIBS@ @LIBGIO_LIBS@ @ID3TAG_LIBS@ @LIBGTHREAD_LIBS@ \
	-Wl -rdynamic

xfce4_squeezebox_plugin_SOURCES = \
	squeezebox.h     squeezebox-private.h squeezebox.c \
	notifications.c  notifications.h notifications-binding.h \
	xfce-shortcuts-grabber.c xfce-shortcuts-grabber.h

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

DISTCLEANFILES = $(desktop_DATA) $(desktop_in_files) $(BUILT_SOURCES)

splint:
	splint -warnposix +ignoresigns -stats \
	*.c  \
	@LIBXFCE4PANEL_CFLAGS@ @DBUSGLIB_CFLAGS@ \
	@LIBMPD_CFLAGS@ @EXO_CFLAGS@ @LIBWNCK_CFLAGS@ \
	@LIBGIO_CFLAGS@ @ID3TAG_CFLAGS@

