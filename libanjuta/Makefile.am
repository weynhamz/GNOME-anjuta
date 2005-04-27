
SUBDIRS = interfaces

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GNOME_UI_CFLAGS) \
	$(GLADE_CFLAGS) \
	-I$(top_srcdir)

lib_LTLIBRARIES=libanjuta.la

libanjuta_la_LIBADD = \
	$(GNOME_UI_LIBS) \
	$(GLADE_LIBS) \
	$(top_builddir)/libegg/libanjuta-egg.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

libanjuta_la_SOURCES= \
	cell-renderer-captioned-image.c	\
	cell-renderer-captioned-image.h \
	e-splash.h \
	e-splash.c \
	anjuta-shell.c \
	anjuta-status.c \
	anjuta-marshal.c \
	anjuta-encodings.c \
	anjuta-preferences-dialog.c \
	anjuta-preferences.c \
	anjuta-plugin.c \
	anjuta-plugin-description.c \
	anjuta-ui.c \
	anjuta-launcher.c \
	anjuta-utils.c \
	anjuta-children.c \
	anjuta-session.c \
	resources.c \
	resources.h \
	glue-plugin.c \
	glue-factory.c \
	plugins.c \
	plugins.h

libanjutaincludedir=$(includedir)/libanjuta-1.0/libanjuta
libanjutainclude_HEADERS = \
	libanjuta.h \
	anjuta-debug.h \
	anjuta-shell.h \
	anjuta-status.h \
	anjuta-encodings.h \
	anjuta-plugin.h \
	anjuta-plugin-description.h \
	anjuta-marshal.h \
	anjuta-utils.h \
	anjuta-preferences-dialog.h \
	anjuta-preferences.h \
	anjuta-ui.h \
	anjuta-launcher.h \
	anjuta-children.h \
	anjuta-session.h \
	glue-plugin.h \
	glue-factory.h \
	resources.h

BUILT_SOURCES=anjuta-marshal.c anjuta-marshal.h

anjuta-marshal.h: anjuta-marshal.list
	@GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)/anjuta-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

anjuta-marshal.c: anjuta-marshal.h anjuta-marshal.list
	echo "#include \"anjuta-marshal.h\"" > xgen-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)/anjuta-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libanjuta-1.0.pc

EXTRA_DIST = \
	$(libanjutainclude_HEADERS) \
	libanjuta-1.0.pc.in \
	anjuta-marshal.list
