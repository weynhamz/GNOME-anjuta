## Process this file with automake to produce Makefile.in

@INTLTOOL_DESKTOP_RULE@

SUBDIRS = tagmanager global-tags \
	pixmaps scripts data \
	doc libanjuta plugins src manuals mime launcher po

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

CLEANFILES = \
	$(gnomemenu_DATA)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(gnomemenu_DATA)
