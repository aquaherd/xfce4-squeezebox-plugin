include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/rhythmbox
plugin_LTLIBRARIES = rhythmbox.la

rhythmbox_la_LIBADD = $(COMMONLIBS)

rhythmbox_la_SOURCES = \
	squeezebox-rb.c rb-shell-binding.h rb-shell-player-binding.h

plugin_DATA = rhythmbox.png
EXTRA_DIST = $(plugin_DATA)
