INCLUDES = \
	$(WARN_CFLAGS) \
	-I$(top_builddir)/scintilla/include \
	-I$(top_builddir)/widgets \
	-I$(top_builddir) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

lib_LTLIBRARIES=libanjuta.la

libanjuta_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_la_LIBADD = \
	$(top_builddir)/widgets/libwidgets.a\
	$(top_builddir)/libegg/treeviewutils/libegg-treeview.a\
	$(top_builddir)/libegg/toolbar/libegg-toolbar.a\
	$(top_builddir)/libegg/menu/libegg-menu.a\
	$(top_builddir)/libegg/util/libeggutil.a

libanjuta_la_SOURCES= \
	anjuta-shell.c \
	anjuta-marshal.c \
	anjuta-encodings.c \
	anjuta-preferences-dialog.c \
	anjuta-preferences.c \
	anjuta-plugin.c \
	anjuta-plugin-parser.c \
	anjuta-ui.c \
	anjuta-utils.c \
	anjuta-children.c \
	properties.cxx \
	properties.h \
	resources.c \
	resources.h \
	defaults.h \
	defaults.c \
	glue-plugin.c \
	glue-factory.c \
	plugins.c \
	plugins.h

libanjutaincludedir=$(includedir)/libanjuta-1.0/libanjuta
libanjutainclude_HEADERS = \
	anjuta-shell.h \
	anjuta-encodings.h \
	anjuta-plugin.h \
	anjuta-plugin-parser.h \
	anjuta-marshal.h \
	anjuta-utils.h \
	anjuta-preferences-dialog.h \
	anjuta-preferences.h \
	anjuta-ui.h \
	anjuta-children.h

BUILT_SOURCES=anjuta-marshal.c anjuta-marshal.h

anjuta-marshal.h: anjuta-marshal.list
	@GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)/anjuta-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

anjuta-marshal.c: anjuta-marshal.h
	echo "#include \"anjuta-marshal.h\"" > xgen-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)/anjuta-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA=libanjuta.pc

EXTRA_DIST=$(libanjutainclude_HEADERS) libanjuta.pc.in anjuta-marshal.list
