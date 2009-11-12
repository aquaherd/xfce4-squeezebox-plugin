@SET_MAKE@

SUBDIRS = panel-plugin po

distclean-local:
	rm -rf *.cache *~

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
	
splint:
	splint -weak -warnposix +ignoresigns \
	./panel-plugin/*.c @LIBXFCE4PANEL_CFLAGS@ @DBUSGLIB_CFLAGS@ \
    @LIBMPD_CFLAGS@ @EXO_CFLAGS@ @LIBWNCK_CFLAGS@ \
    @LIBGIO_CFLAGS@ @ID3TAG_CFLAGS@

EXTRA_DIST = README intltool-extract.in intltool-merge.in intltool-update.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

