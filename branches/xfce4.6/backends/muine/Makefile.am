include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/muine
plugin_LTLIBRARIES = muine.la

muine_la_LIBADD = $(COMMONLIBS)

muine_la_SOURCES = \
	squeezebox-mu.c muine-player-binding.h

plugin_DATA = muine.png
EXTRA_DIST = $(plugin_DATA)
