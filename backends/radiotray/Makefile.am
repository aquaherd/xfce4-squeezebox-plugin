include $(top_srcdir)/backends/common.mk
plugindir = $(backenddir)/radiotray
plugin_LTLIBRARIES = radiotray.la

radiotray_la_LIBADD = $(COMMONLIBS)

radiotray_la_SOURCES = \
	squeezebox-radiotray.c

plugin_DATA = radiotray.png
EXTRA_DIST = $(plugin_DATA)
