include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/mpd
plugin_LTLIBRARIES = mpd.la

mpd_la_LIBADD = $(COMMONLIBS) @LIBXFCE4UTIL_LIBS@ @LIBGTHREAD_LIBS@
mpd_la_CFLAGS = $(AM_CFLAGS) @LIBXFCE4UTIL_CFLAGS@

mpd_la_SOURCES = \
	gmpd.c gmpd.h gmpd-private.h squeezebox-mpd.c settings-mpd.ui

plugin_DATA = settings-mpd.ui mpd.png
EXTRA_DIST = $(plugin_DATA)

