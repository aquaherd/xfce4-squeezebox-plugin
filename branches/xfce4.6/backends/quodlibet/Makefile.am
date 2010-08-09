include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/quodlibet
plugin_LTLIBRARIES = quodlibet.la

quodlibet_la_LIBADD = $(COMMONLIBS)

quodlibet_la_SOURCES = \
	squeezebox-ql.c

plugin_DATA = quodlibet.png
EXTRA_DIST = $(plugin_DATA)
