## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

@INTLTOOL_DESKTOP_RULE@

SUBDIRS = po tagmanager global-tags scintilla \
	widgets printing omf-install pixmaps scripts data \
	doc launcher plugins src macros2

anjutadocdir = ${prefix}/@NO_PREFIX_PACKAGE_DOC_DIR@
anjutadoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO\
	ABOUT-NLS\
	FUTURE

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

gnomemenudir = $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@/Development

gnomemenu_premerge_file = anjuta.desktop.in
gnomemenu_DATA = $(gnomemenu_premerge_file:.desktop.in=.desktop)

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

rpm: dist
	rpm -ta $(distdir).tar.gz

## Install in RedHat's sytem menu, only if the system is RH.
install-data-local:
	-dfi=`which desktop-file-install` ; \
	if [ "x$$dfi" != "x" -a -x $$dfi -a -w $(datadir)/applications ] ; then \
		mkdir -p $(datadir)/applications ; \
		$$dfi --vendor anjuta --delete-original \
		  --dir $(datadir)/applications \
		  --add-category X-Red-Hat-Base --add-category Application \
		  --add-category Development \
		  $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@/Development/anjuta.desktop ; \
	fi
