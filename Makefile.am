## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjtua Project configuration

SUBDIRS = intl po macros scintilla widgets src pixmaps data help doc launcher

anjutadocdir = ${prefix}/doc/anjuta
anjutadoc_DATA = \
	README \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS \
	TODO \
	ABOUT-NLS \
	HACKING

EXTRA_DIST = 
	anjuta.prj \
	$(anjutadoc_DATA)

gnomemenudir = $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@/Development
gnomemenu_DATA = anjuta.desktop

install-data-local:
	install -d -m 0755 $(pkgdatadir)
	install -d -m 0755 $(pkgdatadir)/gnome
	install -m 0644 macros/Makefile.am $(pkgdatadir)/gnome
	install -m 0644 macros/autogen.sh $(pkgdatadir)/gnome
	install -m 0644 macros/*.m4 $(pkgdatadir)/gnome

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
