
SUBDIRS = interfaces

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
	$(top_builddir)/libegg/libanjuta-egg.la\
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

libanjuta_la_SOURCES= \
	cell-renderer-captioned-image.c	\
	cell-renderer-captioned-image.h \
	e-splash.h \
	e-splash.c \
	anjuta-shell.c \
	anjuta-marshal.c \
	anjuta-encodings.c \
	anjuta-preferences-dialog.c \
	anjuta-preferences.c \
	anjuta-plugin.c \
	anjuta-plugin-description.c \
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
	plugins.h \
	gnomefilelist.c \
	gnomefilelist.h \
	fileselection.c \
	fileselection.h

libanjutaincludedir=$(includedir)/libanjuta-1.0/libanjuta
libanjutainclude_HEADERS = \
	anjuta-shell.h \
	anjuta-encodings.h \
	anjuta-plugin.h \
	anjuta-plugin-description.h \
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
