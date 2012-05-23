include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/banshee
plugin_LTLIBRARIES = banshee.la

banshee_la_LIBADD = $(COMMONLIBS)

banshee_la_SOURCES = \
	squeezebox-ba.c banshee-player-binding.h

plugin_DATA = banshee.png
EXTRA_DIST = $(plugin_DATA)
