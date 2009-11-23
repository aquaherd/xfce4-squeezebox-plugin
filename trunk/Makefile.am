@SET_MAKE@
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = panel-plugin po

distclean-local:
	rm -rf *.cache *~

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
	
rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz

deb:
	dpkg-buildpackage -uc -us -rfakeroot -i.git

PKGFLAGS = --default \
	--pkgname=$(PACKAGE) \
	--pkgversion=$(VERSION) \
	--pkgrelease=0 \
	--pkglicense="GNU GPL" \
	--pkggroup="xfce"

EXTRA_DIST = README intltool-extract.in intltool-merge.in intltool-update.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

splint:
	cd panel-plugin && $(MAKE) splint

