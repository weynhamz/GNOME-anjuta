## Process this file with automake to produce Makefile.in

@INTLTOOL_DESKTOP_RULE@

SUBDIRS = po tagmanager global-tags scintilla \
	libegg pixmaps scripts data \
	doc libanjuta plugins src manuals mime 
#	intl

anjutadocdir = ${prefix}/@NO_PREFIX_PACKAGE_DOC_DIR@
anjutadoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO\
	FUTURE

anjuta_glade_filedir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
anjuta_glade_file_DATA = anjuta.glade

EXTRA_DIST = \
	anjuta.desktop.in.in \
	anjuta.spec.in \
	autogen.sh \
	anjuta.prj \
	setup-gettext \
	intltool-extract.in\
	intltool-merge.in\
	intltool-update.in\
	$(anjutadoc_DATA)\
	$(anjuta_glade_file_DATA)\
	anjuta_dialogs_strings.c

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
