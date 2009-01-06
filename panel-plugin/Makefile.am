plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-squeezebox-plugin

xfce4_squeezebox_plugin_CFLAGS = 	-DDATA_DIR=\"$(datadir)/xfce4/xfce4-squeezebox-plugin\"		\
    -DPACKAGE_LOCALE_DIR=\"$(localedir)\" @LIBXFCE4PANEL_CFLAGS@ @DBUSGLIB_CFLAGS@ \
    @LIBMPD_CFLAGS@ @NOTIFY_CFLAGS@ @EXO_CFLAGS@ @LIBWNCK_CFLAGS@ \
    @LIBGIO_CFLAGS@ @ID3TAG_CFLAGS@

xfce4_squeezebox_plugin_LDFLAGS = @LIBXFCE4PANEL_LIBS@ @DBUSGLIB_LIBS@ @LIBMPD_LIBS@ \
	@NOTIFY_LIBS@ @EXO_LIBS@ @LIBWNCK_LIBS@ @LIBGIO_LIBS@ @ID3TAG_LIBS@

xfce4_squeezebox_plugin_SOURCES = \
	squeezebox.h     squeezebox.c \
	squeezebox-rb.c  squeezebox-rb.png  squeezebox-rb.png.h rb-shell-binding.h rb-shell-player-binding.h \
	squeezebox-mpd.c squeezebox-mpd.png squeezebox-mpd.png.h \
	squeezebox-ql.c  squeezebox-ql.png  squeezebox-ql.png.h \
	squeezebox-au.c  squeezebox-au.png  squeezebox-au.png.h audacious-player-binding.h \
	squeezebox-ex.c  squeezebox-ex.png  squeezebox-ex.png.h exaile-player-binding.h \
	squeezebox-cs.c  squeezebox-cs.png  squeezebox-cs.png.h \
	squeezebox-bmp.c squeezebox-bmp.png squeezebox-bmp.png.h bmpx-player-binding.h \
	squeezebox-mu.c  squeezebox-mu.png  squeezebox-mu.png.h  muine-player-binding.h \
	squeezebox-ba.c  squeezebox-ba.png  squeezebox-ba.png.h  banshee-player-binding.h \
	mmkeys.h mmkeys.c
	

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
