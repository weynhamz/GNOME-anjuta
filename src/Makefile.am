## Process this file with automake to produce Makefile.in

anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta.glade

anjuta_uidir = $(anjuta_ui_dir)
anjuta_ui_DATA = anjuta.ui

anjuta_layoutdir = $(anjuta_data_dir)
anjuta_layout_DATA = layout.xml

anjuta_pixmapsdir = $(anjuta_image_dir)

anjuta_pixmaps_DATA = \
	anjuta-preferences-general.svg \
	anjuta-preferences-general-48.png

AM_CPPFLAGS = \
	$(GNOME_CFLAGS)\
	$(GCONF_CFLAGS)\
	$(GDL_CFLAGS)\
	$(GLADE_CFLAGS)\
	-I$(top_srcdir)\
	-I.. -I. -DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DPACKAGE_DOC_DIR=\"$(datadir)/doc/$(PACKAGE)\" \
	-DPACKAGE_PIXMAPS_DIR=\"$(datadir)/pixmaps/$(PACKAGE)\" \
	-DPACKAGE_PLUGIN_DIR=\"$(libdir)/$(PACKAGE)\" \
	-DPACKAGE_LOCALE_DIR=\"$(datadir)/locale\" \
	-DG_LOG_DOMAIN=\"Anjuta\"


include $(top_srcdir)/plugins/scintilla/scintilla/lexers.make

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
	bacon-message-connection.h \
	bacon-message-connection.c \
	main.c

anjuta_LDADD =  \
	$(GNOME_LIBS) \
	$(GCONF_LIBS) \
	$(GDL_LIBS) \
	$(GLADE_LIBS)\
	$(top_builddir)/libanjuta/libanjuta.la

anjuta_LDFLAGS = $(ANJUTA_LDFLAGS)

prefs_glade_files = anjuta.glade
include $(top_srcdir)/scripts/build-schemas.mk

noinst_PROGRAMS =  anjuta-shell

anjuta_shell_SOURCES = shell.c shell.h
anjuta_shell_LDADD =  \
	$(top_builddir)/libanjuta/libanjuta.la

EXTRA_DIST = \
	$(anjuta_ui_DATA) \
	anjuta-shell.ui \
	$(anjuta_layout_DATA) \
	$(anjuta_glade_DATA) \
	$(anjuta_pixmaps_DATA)
