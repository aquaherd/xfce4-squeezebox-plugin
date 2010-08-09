backenddir = $(libexecdir)/xfce4/xfce4-squeezebox-plugin/backends

BACKEND_CFLAGS = @LIBGTK_CFLAGS@ \
    @LIBWNCK_CFLAGS@ @LIBXFCE4UTIL_CFLAGS@ \
    @LIBGIO_CFLAGS@ @LIBXFCONF_CFLAGS@ \
    -I$(top_srcdir)/panel-plugin -DG_LOG_DOMAIN=\"SBP\" 
BACKEND_LIBS = @LIBGTK_LIBS@ @LIBXFCE4UTIL_LIBS@ \
	@LIBWNCK_LIBS@ \
	@LIBGIO_LIBS@ @LIBXFCONF_LIBS@

AM_CFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\" \
	-DPREFIX=\""$(prefix)"\" \
	-DDOCDIR=\""$(docdir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	$(BACKEND_CFLAGS) \
	$(PATH_DEFINES)

AM_LDFLAGS = -module -avoid-version

COMMONLIBS = \
	$(BACKEND_LIBS) \
	$(INTLLIBS)

