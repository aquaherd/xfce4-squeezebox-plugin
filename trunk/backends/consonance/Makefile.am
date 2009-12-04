include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/consonance
plugin_LTLIBRARIES = consonance.la

consonance_la_LIBADD = $(COMMONLIBS)

consonance_la_SOURCES = \
	squeezebox-cs.c

plugin_DATA = consonance.png
EXTRA_DIST = $(plugin_DATA)
