## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = m4  intl po macros tagmanager global-tags scintilla \
	widgets printing manuals omf-install pixmaps data \
	doc launcher plugins scripts src

anjutadocdir = ${prefix}/doc/anjuta
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

EXTRA_DIST = config.rpath  \
	anjuta.desktop.in.in \
	anjuta.spec.in \
	xml-i18n-extract.in \
	xml-i18n-merge.in \
	xml-i18n-update.in \
	autogen.sh \
	anjuta.prj\
	$(anjutadoc_DATA)

@XML_I18N_MERGE_DESKTOP_RULE@

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

ACLOCAL_AMFLAGS = -I m4
