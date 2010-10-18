
SUBDIRS = interfaces 

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I . \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(ANJUTA_CFLAGS) \
	-DDATADIR="\"$(datadir)\"" \
	-DPACKAGE_PIXMAPS_DIR="\"$(datadir)/pixmaps/$(PACKAGE)\"" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DPACKAGE_HELP_DIR="\"$(datadir)/gnome/help/$(PACKAGE)\"" \
	-DPACKAGE_DOC_DIR="\"$(docdir)\"" \
	-DG_LOG_DOMAIN=\"libanjuta\"

lib_LTLIBRARIES = libanjuta.la 
libanjuta_la_LDFLAGS = $(ANJUTA_LDFLAGS) 
libanjuta_la_LIBADD = \
	interfaces/libanjuta-interfaces.la \
	$(ANJUTA_LIBS)

libanjuta_la_SOURCES= \
	anjuta-enum-types.h \
	anjuta-enum-types.c \
	anjuta-marshal.h \
	anjuta-marshal.c \
	anjuta-shell.c \
	anjuta-shell.h \
	anjuta-status.c \
	anjuta-status.h \
	anjuta-encodings.c \
	anjuta-encodings.h \
	anjuta-convert.c \
	anjuta-convert.h \
	anjuta-preferences-dialog.c \
	anjuta-preferences-dialog.h \
	anjuta-preferences.c \
	anjuta-preferences.h \
	anjuta-plugin.c \
	anjuta-plugin.h \
	anjuta-plugin-handle.h \
	anjuta-plugin-handle.c \
	anjuta-plugin-description.c \
	anjuta-plugin-description.h \
	anjuta-plugin-manager.c \
	anjuta-plugin-manager.h \
	anjuta-profile.c \
	anjuta-profile.h \
	anjuta-profile-manager.c \
	anjuta-profile-manager.h \
	anjuta-ui.c \
	anjuta-ui.h \
	anjuta-launcher.c \
	anjuta-launcher.h \
	anjuta-utils.c \
	anjuta-utils.h \
	anjuta-utils-priv.h \
	anjuta-session.c \
	anjuta-session.h \
	anjuta-save-prompt.c \
	anjuta-save-prompt.h \
	anjuta-serializer.c \
	anjuta-serializer.h \
	resources.c \
	resources.h \
	anjuta-c-plugin-factory.c \
	anjuta-c-plugin-factory.h \
	anjuta-c-module.c \
	anjuta-c-module.h \
	cell-renderer-captioned-image.c	\
	cell-renderer-captioned-image.h \
	e-splash.h \
	e-splash.c \
	anjuta-vcs-status.h \
	anjuta-vcs-status-tree-view.c \
	anjuta-vcs-status-tree-view.h \
	anjuta-command.c \
	anjuta-command.h \
	anjuta-async-command.c \
	anjuta-async-command.h \
	anjuta-sync-command.c \
	anjuta-sync-command.h \
	anjuta-async-notify.h \
	anjuta-async-notify.c \
	anjuta-debug.c \
	anjuta-debug.h \
	gbf-project.c \
	gbf-project.h \
	anjuta-command-queue.c \
	anjuta-command-queue.h \
	anjuta-project.c \
	anjuta-project.h \
	anjuta-drop-entry.c \
	anjuta-drop-entry.h \
	anjuta-tabber.c \
	anjuta-tabber.h \
	anjuta-command-bar.c \
	anjuta-command-bar.h \
	anjuta-dock.c \
	anjuta-dock.h \
	anjuta-dock-pane.c \
	anjuta-dock-pane.h \
	anjuta-file-list.c \
	anjuta-file-list.h \
	anjuta-pkg-config-chooser.h \
	anjuta-pkg-config-chooser.c \
	anjuta-column-text-view.h \
	anjuta-column-text-view.c \
	anjuta-file-drop-entry.h \
	anjuta-file-drop-entry.c \
	anjuta-entry.h \
	anjuta-entry.c



if HAVE_PLUGIN_GLADE

# For now install in our own prefix
# gladeanjutadir = @GLADE_PLUGINS_PATH@

gladeanjutadir = $(libdir)/glade3/modules

gladeanjuta_LTLIBRARIES = libgladeanjuta.la
libgladeanjuta_la_SOURCES = anjuta-glade-catalog.c
libgladeanjuta_la_LDFLAGS = $(ANJUTA_LDFLAGS) -module -avoid-version
libgladeanjuta_la_LIBADD = \
	libanjuta.la \
	$(GLADE_LIBS)

# For now install in our own prefix
# catalogdir = @GLADE_CATALOG_PATH@

catalogdir = $(datadir)/glade3/catalogs
catalog_DATA = anjuta-glade.xml

endif

libanjutaincludedir=$(includedir)/libanjuta-1.0/libanjuta

libanjuta_include = \
	libanjuta.h \
	anjuta-debug.h \
	anjuta-shell.h \
	anjuta-status.h \
	anjuta-encodings.h \
	anjuta-convert.h \
	anjuta-plugin.h \
	anjuta-plugin-description.h \
	anjuta-plugin-manager.h \
	anjuta-plugin-handle.h \
	anjuta-profile.h \
	anjuta-profile-manager.h \
	anjuta-utils.h \
	anjuta-preferences-dialog.h \
	anjuta-preferences.h \
	anjuta-ui.h \
	anjuta-launcher.h \
	anjuta-session.h \
	anjuta-save-prompt.h \
	anjuta-serializer.h \
	resources.h \
	anjuta-vcs-status-tree-view.h \
	anjuta-vcs-status.h \
	anjuta-command.h \
	anjuta-async-command.h \
	anjuta-async-notify.h \
	anjuta-sync-command.h \
	anjuta-version.h \
	gbf-project.h \
	anjuta-project.h \
	anjuta-command-queue.h \
	anjuta-drop-entry.h \
	anjuta-tabber.h \
	anjuta-command-bar.h \
	anjuta-dock.h \
	anjuta-dock-pane.h \
	anjuta-file-list.h \
	anjuta-pkg-config-chooser.h \
	anjuta-column-text-view.h \
	anjuta-file-drop-entry.h \
	anjuta-entry.h

libanjutainclude_HEADERS = \
	$(libanjuta_include) \
	anjuta-enum-types.h \
	anjuta-marshal.h

BUILT_SOURCES=anjuta-marshal.c anjuta-marshal.h  anjuta-enum-types.h anjuta-enum-types.c

anjuta-marshal.h: anjuta-marshal.list
	$(AM_V_GEN)@GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)/anjuta-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

anjuta-marshal.c: anjuta-marshal.h anjuta-marshal.list
	$(AM_V_GEN)echo "#include \"anjuta-marshal.h\"" > xgenc-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)/anjuta-marshal.list --body >> xgenc-gmc \
	&& cp xgenc-gmc $(@F) \
	&& rm -f xgenc-gmc

anjuta-enum-types.h: stamp-anjuta-enum-types.h $(libanjuta_include)
	@true
stamp-anjuta-enum-types.h: Makefile $(libanjuta_include) anjuta-enum-types.c
	$(AM_V_GEN)( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef __ANJUTA_ENUM_TYPES_H__\n#define __ANJUTA_ENUM_TYPES_H__\n\n#include <libanjuta/libanjuta.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define ANJUTA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* __ANJUTA_ENUM_TYPES_H__ */" \
		$(libanjuta_include) ) >> xgen-geth \
	&& (cmp -s xgen-geth anjuta-enum-types.h || cp xgen-geth anjuta-enum-types.h ) \
	&& rm -f xgen-geth \
	&& echo timestamp > $(@F)

anjuta-enum-types.c: Makefile
	$(AM_V_GEN)( cd $(srcdir) && $(GLIB_MKENUMS) \
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
	anjuta-marshal.list \
	$(catalog_DATA)

DISTCLEANFILES = \
  anjuta-enum-types.h \
  anjuta-enum-types.c \
  stamp-anjuta-enum-types.h

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
anjuta_introspection_sources = $(libanjuta_la_SOURCES)
ianjuta_introspection_sources = $(srcdir)/interfaces/*.c $(srcdir)/interfaces/*.h

Anjuta-1_0.gir: libanjuta.la
Anjuta_1_0_gir_INCLUDES = GObject-2.0 Gtk-2.0
Anjuta_1_0_gir_CFLAGS = $(ANJUTA_CFLAGS) -I$(top_srcdir)
Anjuta_1_0_gir_LIBS = anjuta
Anjuta_1_0_gir_FILES = $(anjuta_introspection_sources)
INTROSPECTION_GIRS += Anjuta-1.0.gir

IAnjuta-1_0.gir: interfaces/libanjuta-interfaces.la libanjuta.la Anjuta-1.0.gir
IAnjuta_1_0_gir_INCLUDES = GObject-2.0 Gtk-2.0 Anjuta-1.0
IAnjuta_1_0_gir_CFLAGS = $(LIBANJUTA_CFLAGS) -I$(top_srcdir)
IAnjuta_1_0_gir_LIBS = interfaces/libanjuta-interfaces.la anjuta
IAnjuta_1_0_gir_FILES = $(ianjuta_introspection_sources)
INTROSPECTION_GIRS += IAnjuta-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(dist_gir_DATA) $(typelib_DATA)
endif


-include $(top_srcdir)/git.mk
