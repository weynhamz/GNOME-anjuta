## Process this file with automake to produce Makefile.in

anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = preferences.ui

anjuta_uidir = $(anjuta_ui_dir)
anjuta_ui_DATA = anjuta.xml

anjuta_layoutdir = $(anjuta_data_dir)
anjuta_layout_DATA = layout.xml

anjuta_pixmapsdir = $(anjuta_image_dir)

anjuta_pixmaps_DATA = \
	anjuta-preferences-general.svg \
	anjuta-preferences-general-48.png

AM_CPPFLAGS = \
	-I$(top_srcdir)\
	-I.. -I. \
	$(LIBANJUTA_CFLAGS) \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DPACKAGE_DOC_DIR=\"$(docdir)\" \
	-DPACKAGE_PIXMAPS_DIR=\"$(datadir)/pixmaps/$(PACKAGE)\" \
	-DPACKAGE_PLUGIN_DIR=\"$(libdir)/$(PACKAGE)\" \
	-DPACKAGE_LOCALE_DIR=\"$(datadir)/locale\" \
	-DG_LOG_DOMAIN=\"Anjuta\"

bin_PROGRAMS = anjuta

anjuta_SOURCES = \
	about.c \
	about.h \
	anjuta.c \
	anjuta.h \
	anjuta-app.c \
	anjuta-app.h \
	anjuta-actions.h \
	action-callbacks.h \
	action-callbacks.c \
	main.c

anjuta_LDADD =  \
	$(GDL_LIBS) \
	$(GLADE_LIBS)\
	$(GLIB_LIBS) \
	$(LIBANJUTA_LIBS)

anjuta_LDFLAGS = $(ANJUTA_LDFLAGS)

prefs_ui_files = preferences.ui
prefs_name = org.gnome.anjuta
prefs_keyfile = anjuta.gschema-part.xml
include $(top_srcdir)/scripts/build-schemas.mk

noinst_PROGRAMS =  anjuta-shell

anjuta_shell_SOURCES = shell.c shell.h
anjuta_shell_LDADD =  \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(anjuta_ui_DATA) \
	anjuta-shell.ui \
	$(anjuta_layout_DATA) \
	$(anjuta_glade_DATA) \
	$(anjuta_pixmaps_DATA)

-include $(top_srcdir)/git.mk
