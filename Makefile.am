## Process this file with automake to produce Makefile.in

@INTLTOOL_DESKTOP_RULE@

SUBDIRS = tagmanager global-tags \
	libegg pixmaps scripts data \
	libanjuta plugins src manuals mime launcher po doc

anjutadocdir = $(datadir)/doc/anjuta
anjutadoc_DATA = \
	MAINTAINERS\
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO\
	FUTURE\
	ROADMAP\
	THANKS

## Install it separately in datadir also because it is used in credits
## listing and packaging could compress the ones in docs dir.

anjuta_datadir = $(datadir)/anjuta
anjuta_data_DATA = AUTHORS

EXTRA_DIST = \
	anjuta.desktop.in.in \
	anjuta.spec.in \
	autogen.sh \
	anjuta.prj \
	setup-gettext \
	intltool-extract.in\
	intltool-merge.in\
	intltool-update.in\
	$(anjutadoc_DATA)

gnomemenudir = $(datadir)/applications
gnomemenu_premerge_file = anjuta.desktop.in
gnomemenu_DATA = $(gnomemenu_premerge_file:.desktop.in=.desktop)

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

rpm: dist
	rpmbuild -ta $(distdir).tar.gz

## Install in RedHat's sytem menu, only if the system is RH.
## install-data-local:
## 	-dfi=`which desktop-file-install` ; \
## 	if ( [ "x$$dfi" != "x" ] && [ -x $$dfi ] && [ -w $(datadir)/applications ] ) ; then \
## 		mkdir -p $(datadir)/applications ; \
## 		$$dfi --vendor anjuta --delete-original \
## 		  --dir $(datadir)/applications \
## 		  --add-category X-Red-Hat-Base --add-category Application \
## 		  --add-category Development \
## 		  $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@/Development/anjuta.desktop ; \
## 	fi


CLEANFILES = \
	$(gnomemenu_DATA)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(gnomemenu_DATA)
