## Process this file with automake to produce Makefile.in

anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta.ui

anjuta_layoutdir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@
anjuta_layout_DATA = layout.xml

INCLUDES = \
	$(ANJUTA_CFLAGS)\
	$(GDL_CFLAGS)\
	$(PCRE_CFLAGS)\
	$(FREEBSD_BUILD)\
	-I$(top_srcdir)\
	-I$(top_srcdir)/libegg\
	-I$(srcdir)/../scintilla/include \
	-I$(srcdir)/../tagmanager/include \
	-I$(srcdir)/../pixmaps \
	-I$(srcdir)/../printing \
	-I.. -I. -DPACKAGE_BIN_DIR=\"$(bindir)\"
#	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
#	-DGDK_PIXBUF_DEPRECATED \
#	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED

include $(top_srcdir)/scintilla/lexers.make

bin_PROGRAMS = anjuta

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
	main.cpp

anjuta_LDADD =  \
	$(ANJUTA_LIBS) $(GDL_LIBS) -lutil\
	$(PCRE_LIBS)\
	$(top_builddir)/libanjuta/libanjuta.la\
	$(top_builddir)/libanjuta/anjuta-preferences.o
	$(top_builddir)/tagmanager/libanjuta-ctags.la\
	$(top_builddir)/scintilla/libanjuta-scintilla.la

#	$(top_builddir)/libegg/treeviewutils/libegg-treeview.a\
#	$(top_builddir)/libegg/menu/libegg-menu.a\
#	$(top_builddir)/libegg/toolbar/libegg-toolbar.a\
#	$(top_builddir)/libegg/util/libeggutil.a\
#	$(top_builddir)/printing/libanjutaprint.a\
#	$(LEXER_OBJS) \
#	$(top_builddir)/gdl/libgdl.a

#anjuta_DEPENDENCIES = \
#	$(top_builddir)/libanjuta/libanjuta.la
#	$(top_builddir)/libegg/menu/libegg-menu.a \
#	$(top_builddir)/libegg/treeviewutils/libegg-treeview.a
