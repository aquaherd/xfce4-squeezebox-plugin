include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/exaile
plugin_LTLIBRARIES = exaile.la

exaile_la_LIBADD = $(COMMONLIBS)

exaile_la_SOURCES = \
	squeezebox-ex.c exaile-player-binding.h

plugin_DATA = exaile.png
EXTRA_DIST = $(plugin_DATA)
