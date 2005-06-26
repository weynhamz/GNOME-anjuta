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
	-I.. -I. -DPACKAGE_BIN_DIR=\"$(bindir)\"

include $(top_srcdir)/scintilla/lexers.make

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
	main.c

anjuta_LDADD =  \
	$(GNOME_UI_LIBS) \
	$(GDL_LIBS) \
	$(top_builddir)/libanjuta/libanjuta.la

anjuta_shell_SOURCES = shell.c shell.h
anjuta_shell_LDADD =  \
	$(top_builddir)/libanjuta/libanjuta.la

EXTRA_DIST = \
	$(anjuta_ui_DATA) \
	$(anjuta_layout_DATA) \
	$(anjuta_glade_DATA)
