INCLUDES = \
	$(WARN_CFLAGS) \
	-I$(top_builddir)/scintilla/include \
	-I$(top_builddir)/widgets \
	-I$(top_builddir) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

lib_LTLIBRARIES=libanjuta-2.la

libanjuta_2_la_LIADD=$(LIBANJUTA_LIBS)
libanjuta_2_la_SOURCES= \
	anjuta-shell.c \
	anjuta-marshal.c \
	anjuta-preferences-dialog.c \
	anjuta-preferences.c \
	anjuta-plugin.c \
	anjuta-utils.c \
	properties.cxx \
	properties.h \
	defaults.h \
	glue-plugin.c \
	glue-factory.c 

libanjutaincludedir=$(includedir)/libanjuta-1.0/libanjuta
libanjutainclude_HEADERS = \
	anjuta-shell.h \
	anjuta-plugin.h \
	anjuta-marshal.h \
	anjuta-utils.h \
	anjuta-preferences-dialog.h \
	anjuta-preferences.h 

BUILT_SOURCES=anjuta-marshal.c anjuta-marshal.h

anjuta-marshal.h: anjuta-marshal.list
	@GLIB_GENMARSHAL@ \
	        --prefix=anjuta_marshal $(srcdir)/anjuta-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

anjuta-marshal.c: anjuta-marshal.h
	echo "#include \"anjuta-marshal.h\"" > xgen-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=anjuta_marshal $(srcdir)/anjuta-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA=libanjuta-2.0.pc

EXTRA_DIST=$(libanjutainclude_HEADERS) libanjuta-2.0.pc.in anjuta-marshal.list
