## Process this file with automake to produce Makefile.in

anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA = anjuta.ui anjuta-shell.ui

anjuta_layoutdir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@
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
	preferences.c \
	preferences.h \
	main.c

anjuta_LDADD =  \
	-lutil \
	$(GNOME_UI_LIBS) \
	$(GDL_LIBS) \
	$(top_builddir)/libanjuta/libanjuta.la

#anjuta_DEPENDENCIES = \
#	$(top_builddir)/libanjuta/libanjuta.la
#	$(top_builddir)/libegg/menu/libegg-menu.a \
#	$(top_builddir)/libegg/treeviewutils/libegg-treeview.a

anjuta_shell_SOURCES = shell.c
anjuta_shell_LDADD =  \
	-lutil \
	$(top_builddir)/libanjuta/libanjuta.la
