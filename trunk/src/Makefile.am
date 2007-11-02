## Process this file with automake to produce Makefile.in

anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta.glade

anjuta_uidir = $(anjuta_ui_dir)
anjuta_ui_DATA = anjuta.ui anjuta-shell.ui

anjuta_layoutdir = $(anjuta_data_dir)
anjuta_layout_DATA = layout.xml

INCLUDES = \
	$(GNOME_UI_CFLAGS)\
	$(GDL_CFLAGS)\
	-I$(top_srcdir)\
	-I.. -I. -DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DPACKAGE_DOC_DIR=\"$(datadir)/doc/$(PACKAGE)\" \
	-DPACKAGE_PIXMAPS_DIR=\"$(datadir)/pixmaps/$(PACKAGE)\" \
	-DPACKAGE_PLUGIN_DIR=\"$(libdir)/$(PACKAGE)\" \
	-DPACKAGE_LOCALE_DIR=\"$(datadir)/locale\"


include $(top_srcdir)/plugins/editor/scintilla/lexers.make

bin_PROGRAMS = anjuta anjuta-shell

anjuta_SOURCES = \
	about.c \
	about.h \
	anjuta.c \
	anjuta.h \
	anjuta-app.c \
	anjuta-app.h \
	anjuta-callbacks.h \
	anjuta-callbacks.c \
	anjuta-actions.h \
	action-callbacks.h \
	action-callbacks.c \
	bacon-message-connection.h \
	bacon-message-connection.c \
	main.c

anjuta_LDADD =  \
	$(GNOME_UI_LIBS) \
	$(GDL_LIBS) \
	$(top_builddir)/libanjuta/libanjuta.la

anjuta_LDFLAGS = $(ANJUTA_LDFLAGS)

prefs_glade_files = anjuta.glade
include $(top_srcdir)/scripts/build-schemas.mk

anjuta_DEPENDENCIES = \
	build-schema-files

anjuta_shell_SOURCES = shell.c shell.h
anjuta_shell_LDADD =  \
	$(top_builddir)/libanjuta/libanjuta.la

EXTRA_DIST = \
	$(anjuta_ui_DATA) \
	$(anjuta_layout_DATA) \
	$(anjuta_glade_DATA)
