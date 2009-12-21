include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/mpd
plugin_LTLIBRARIES = mpd.la

mpd_la_LIBADD = $(COMMONLIBS) @LIBMPD_LIBS@ @LIBXFCE4UTIL_LIBS@
mpd_la_CFLAGS = $(AM_CFLAGS) @LIBMPD_CFLAGS@ @LIBXFCE4UTIL_CFLAGS@

mpd_la_SOURCES = \
	gmpd.c squeezebox-mpd.c settings-mpd.ui

plugin_DATA = settings-mpd.ui mpd.png
EXTRA_DIST = $(plugin_DATA)

