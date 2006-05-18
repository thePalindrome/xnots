# ---------------------------------------------------------------------------- #
#
# Created	: Fri 14 Apr 2006 07:20:44 PM CDT
# Modified	: Wed 17 May 2006 10:39:13 PM CDT
# Author	: Gautam Iyer <gi1242@users.sourceforge.net>
# Licence	: GPL2
#
# ---------------------------------------------------------------------------- #

VERSION		= 0.11

#
# Files installed by this package
#
inst_bins	= src/xnots
inst_mans	= doc/xnots.1
inst_docs	= doc/examples/
inst_extras	= README LICENCE

#
# Files packed into the source archive
#
dist_sources	= src/Makefile src/*.c src/*.h
dist_mans	= $(inst_mans)
dist_docs	= $(inst_docs)
dist_extras	= $(inst_extras)
dist_makefile	= Makefile
dist_etc	= etc/

#
# Directory variables
#
prefix		= /usr/local
bindir		= $(prefix)/bin
mandir		= $(prefix)/man
man1dir		= $(mandir)/man1
datadir		= $(prefix)/share
docdir		= $(datadir)/doc/xnots-$(VERSION)

#
# Programs
#
CC		= gcc
INSTALL		= install
INSTALL_PROGRAM	= $(INSTALL)
INSTALL_DATA	= ${INSTALL} -m 644


#
# Make rules
#
.PHONY: xnots
xnots:
	cd src && $(MAKE)

.PHONY: install-strip
install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

.PHONY: installdirs
installdirs:
	mkdir -p $(DESTDIR)$(bindir) $(DESTDIR)$(man1dir) $(DESTDIR)$(docdir)

.PHONY: install
install: xnots installdirs $(inst_bins) $(inst_mans) $(inst_docs) $(inst_extras)
	$(INSTALL)      $(inst_bins)	$(DESTDIR)$(bindir)
	$(INSTALL_DATA) $(inst_mans)	$(DESTDIR)$(man1dir)
	$(INSTALL_DATA) $(inst_extras)	$(DESTDIR)$(docdir)
	cp -R		$(inst_docs)	$(DESTDIR)$(docdir)

.PHONY: dist
dist: $(dist_sources) $(dist_docs) $(dist_mans) $(dist_extras)		    \
		$(dist_makefile) $(dist_etc)
	export distdir=xnots-$(VERSION)					    \
	&& mkdir -p $$distdir/doc $$distdir/src				    \
	&& cp $(dist_sources) $$distdir/src				    \
	&& cp -R $(dist_mans) $(dist_docs) $$distdir/doc		    \
	&& cp $(dist_extras) $(dist_makefile) $$distdir/		    \
	&& cp -R $(dist_etc) $$distdir/doc				    \
	&& tar -c $$distdir --exclude .svn | gzip -9 > $$distdir.tar.gz     \
	&& rm -rf $$distdir
