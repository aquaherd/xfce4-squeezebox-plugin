include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/audacious
plugin_LTLIBRARIES = audacious.la

audacious_la_LIBADD = $(COMMONLIBS)

audacious_la_SOURCES = \
	squeezebox-au.c audacious-player-binding.h

plugin_DATA = audacious.png
EXTRA_DIST = $(plugin_DATA)
