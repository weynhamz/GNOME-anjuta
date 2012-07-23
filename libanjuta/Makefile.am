include interfaces/Makefile.am.gir

SUBDIRS = interfaces . tests

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I . \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(ANJUTA_CFLAGS) \
	$(XML_CFLAGS) \
	$(GDL_CFLAGS) \
	-DDATADIR="\"$(datadir)\"" \
	-DPACKAGE_PIXMAPS_DIR="\"$(datadir)/pixmaps/$(PACKAGE)\"" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DPACKAGE_HELP_DIR="\"$(datadir)/gnome/help/$(PACKAGE)\"" \
	-DPACKAGE_DOC_DIR="\"$(docdir)\"" \
	-DG_LOG_DOMAIN=\"libanjuta\"

lib_LTLIBRARIES = libanjuta-3.la 
libanjuta_3_la_LDFLAGS = $(ANJUTA_LDFLAGS) 
libanjuta_3_la_LIBADD = \
	interfaces/libanjuta-interfaces.la \
	$(ANJUTA_LIBS) \
	$(XML_LIBS) \
	$(GDL_LIBS)

libanjuta_3_la_SOURCES= \
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
	anjuta-language-provider.c \
	anjuta-language-provider.h \
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
	anjuta-cell-renderer-captioned-image.c	\
	anjuta-cell-renderer-captioned-image.h \
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
	anjuta-command-queue.c \
	anjuta-command-queue.h \
	anjuta-token.c \
	anjuta-token.h \
	anjuta-token-list.h \
	anjuta-token-list.c \
	anjuta-token-file.h \
	anjuta-token-file.c \
	anjuta-token-stream.c \
	anjuta-token-stream.h \
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
	anjuta-pkg-config.h \
	anjuta-pkg-config.c \
	anjuta-pkg-scanner.h \
	anjuta-pkg-scanner.c \
	anjuta-column-text-view.h \
	anjuta-column-text-view.c \
	anjuta-file-drop-entry.h \
	anjuta-file-drop-entry.c \
	anjuta-entry.h \
	anjuta-entry.c \
	anjuta-environment-editor.h \
	anjuta-environment-editor.c \
	anjuta-tree-combo.h \
	anjuta-tree-combo.c \
	anjuta-autogen.h \
	anjuta-autogen.c

# Glade module
if HAVE_PLUGIN_GLADE

# For now install in our own prefix
# gladeanjutadir = @GLADE_PLUGINS_PATH@

gladeanjutadir = $(libdir)/glade/modules

gladeanjuta_LTLIBRARIES = libgladeanjuta.la
libgladeanjuta_la_SOURCES = anjuta-glade-catalog.c
libgladeanjuta_la_LDFLAGS = $(ANJUTA_LDFLAGS) -module -avoid-version
libgladeanjuta_la_LIBADD = \
	libanjuta-3.la \
	$(GLADE_LIBS)

# For now install in our own prefix
# catalogdir = @GLADE_CATALOG_PATH@

catalogdir = $(datadir)/glade/catalogs
catalog_DATA = anjuta-glade.xml

endif

# libanjuta

libanjutaincludedir=$(includedir)/libanjuta-3.0/libanjuta

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
	anjuta-project.h \
	anjuta-command-queue.h \
	anjuta-drop-entry.h \
	anjuta-tabber.h \
	anjuta-command-bar.h \
	anjuta-dock.h \
	anjuta-dock-pane.h \
	anjuta-file-list.h \
	anjuta-pkg-config-chooser.h \
	anjuta-pkg-config.h \
	anjuta-pkg-scanner.h \
	anjuta-column-text-view.h \
	anjuta-file-drop-entry.h \
	anjuta-entry.h \
	anjuta-token.h \
	anjuta-pkg-config-chooser.h

libanjutainclude_HEADERS = \
	$(libanjuta_include) \
	anjuta-enum-types.h \
	anjuta-marshal.h

BUILT_SOURCES = anjuta-marshal.c anjuta-marshal.h  anjuta-enum-types.h anjuta-enum-types.c

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
pkgconfig_DATA = libanjuta-3.0.pc

EXTRA_DIST = \
	$(libanjutainclude_HEADERS) \
	libanjuta-3.0.pc.in \
	anjuta-marshal.list \
	$(catalog_DATA)

DISTCLEANFILES = \
  anjuta-enum-types.h \
  anjuta-enum-types.c \
  stamp-anjuta-enum-types.h

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(builddir) -I $(top_builddir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
anjuta_introspection_sources = $(libanjuta_3_la_SOURCES)
ianjuta_introspection_sources = $(libanjuta_source_files) $(libanjuta_header_files)

Anjuta-3.0.gir: libanjuta-3.la
Anjuta_3_0_gir_INCLUDES = GObject-2.0 Gtk-3.0
Anjuta_3_0_gir_CFLAGS = $(ANJUTA_CFLAGS) $(XML_CFLAGS) $(GDL_CFLAGS) -I$(top_srcdir)
Anjuta_3_0_gir_LIBS = libanjuta-3.la
Anjuta_3_0_gir_FILES = $(anjuta_introspection_sources)
Anjuta_3_0_gir_SCANNERFLAGS = --c-include=libanjuta/libanjuta.h
INTROSPECTION_GIRS += Anjuta-3.0.gir

IAnjuta-3.0.gir: interfaces/libanjuta-interfaces.la libanjuta-3.la Anjuta-3.0.gir
IAnjuta_3_0_gir_INCLUDES = GObject-2.0 Gtk-3.0 Anjuta-3.0
IAnjuta_3_0_gir_CFLAGS = $(ANJUTA_CFLAGS) $(XML_CFLAGS) -I$(top_srcdir)
IAnjuta_3_0_gir_LIBS = interfaces/libanjuta-interfaces.la libanjuta-3.la
IAnjuta_3_0_gir_FILES = $(ianjuta_introspection_sources)
IAnjuta_3_0_gir_SCANNERFLAGS = --add-include-path=$(top_srcdir)/libanjuta/ --symbol-prefix=ianjuta --c-include=libanjuta/interfaces/libanjuta-interfaces.h
INTROSPECTION_GIRS += IAnjuta-3.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(gir_DATA) $(typelib_DATA)
endif


-include $(top_srcdir)/git.mk
