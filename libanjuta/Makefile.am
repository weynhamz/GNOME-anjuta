
SUBDIRS = interfaces

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GNOME_UI_CFLAGS) \
	$(GLADE_CFLAGS) \
	-I$(top_srcdir) \
	-DPACKAGE_PIXMAPS_DIR="\"$(datadir)/pixmaps/$(PACKAGE)\"" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DPACKAGE_HELP_DIR="\"$(datadir)/gnome/help/$(PACKAGE)\"" \
	-DPACKAGE_DOC_DIR="\"$(datadir)/doc/$(PACKAGE)\"" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\""

lib_LTLIBRARIES=libanjuta.la

libanjuta_la_LIBADD = \
	-lutil \
	$(GNOME_UI_LIBS) \
	$(GLADE_LIBS) \
	$(top_builddir)/libegg/libanjuta-egg.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

libanjuta_la_SOURCES= \
	anjuta-enum-types.h \
	anjuta-enum-types.c \
	cell-renderer-captioned-image.c	\
	cell-renderer-captioned-image.h \
	e-splash.h \
	e-splash.c \
	anjuta-marshal.c \
	anjuta-shell.c \
	anjuta-status.c \
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
	plugins.h \
	anjuta-save-prompt.c \
	anjuta-serializer.c \
	anjuta-serializer.h 

libanjutaincludedir=$(includedir)/libanjuta-1.0/libanjuta

libanjuta_include = \
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
	anjuta-save-prompt.h \
	anjuta-serializer.h \
	glue-plugin.h \
	glue-factory.h \
	resources.h

libanjutainclude_HEADERS = \
	$(libanjuta_include) \
	anjuta-enum-types.h

BUILT_SOURCES=anjuta-marshal.c anjuta-marshal.h  anjuta-enum-types.h anjuta-enum-types.c

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

anjuta-enum-types.h: stamp-anjuta-enum-types.h
	@true
stamp-anjuta-enum-types.h: Makefile $(libanjuta_include) anjuta-enum-types.c
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef __ANJUTA_ENUM_TYPES_H__\n#define __ANJUTA_ENUM_TYPES_H__\n\n#include <libanjuta/libanjuta.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define ANJUTA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* __ANJUTA_ENUM_TYPES_H__ */" \
		$(libanjuta_include) ) >> xgen-geth \
	&& (cmp -s xgen-geth anjuta-enum-types.h || cp xgen-geth anjuta-enum-types.h ) \
	&& rm -f xgen-geth \
	&& echo timestamp > $(@F)
	
anjuta-enum-types.c: Makefile
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include \"anjuta-enum-types.h\"\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--ftail "\n#define __ANJUTA_ENUM_TYPES_C__\n" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (g_intern_static_string (\"@EnumName@\"), values);\n  }\n  return etype;\n}\n" \
		$(libanjuta_include) ) > xgen-getc \
	&& cp xgen-getc anjuta-enum-types.c  \
	&& rm -f xgen-getc


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libanjuta-1.0.pc

EXTRA_DIST = \
	$(libanjutainclude_HEADERS) \
	libanjuta-1.0.pc.in \
	anjuta-marshal.list
