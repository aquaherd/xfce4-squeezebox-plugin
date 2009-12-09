@SET_MAKE@
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = panel-plugin glade backends po redhat

distclean-local:
	rm -rf *.cache *~

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION)-$(REV).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION)-$(REV).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION)-$(REV).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION)-$(REV).tar.bz2
	
rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz

deb:
	dpkg-buildpackage -rfakeroot -i.svn
#	dpkg-buildpackage -uc -us -rfakeroot -i.git

PUBLOC = ~/apt/repo/dists/custom/main
PUBARCH = `/usr/bin/dpkg-architecture -qDEB_HOST_ARCH`
PVERSION = $(VERMAJOR).$(VERMINOR)-$(VERMICRO)
debpub: 
	cp ../$(PACKAGE)_$(PVERSION).tar.gz $(PUBLOC)/source
	cp ../$(PACKAGE)_$(PVERSION).dsc $(PUBLOC)/source
	cp ../$(PACKAGE)_$(PVERSION)_$(PUBARCH).deb $(PUBLOC)/binary-$(PUBARCH)
	cp ../$(PACKAGE)_$(PVERSION)_$(PUBARCH).changes $(PUBLOC)/binary-$(PUBARCH)

PKGFLAGS = --default \
	--pkgname=$(PACKAGE) \
	--pkgversion=$(VERSION) \
	--pkgrelease=$(REV) \
	--pkglicense="GNU GPL" \
	--pkggroup="xfce"

EXTRA_DIST = README TODO THANKS intltool-extract.in intltool-merge.in intltool-update.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

splint:
	cd panel-plugin && $(MAKE) splint

