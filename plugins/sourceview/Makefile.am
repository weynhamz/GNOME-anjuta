# Plugin glade file
sourceview_gladedir = $(anjuta_glade_dir)
sourceview_glade_DATA = sourceview.glade

# Plugin Icon file
sourceview_pixmapsdir = $(anjuta_image_dir)
sourceview_pixmaps_DATA = sourceview.png

# Plugin description file
plugin_in_files = sourceview.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

sourceview_plugindir = $(anjuta_plugin_dir)
sourceview_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PCRE_CFLAGS) \
	$(GNOME_PRINT_UI_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(GTKSOURCEVIEW_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-I$(top_srcdir)/tagmanager/include

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# Marshallers
BUILT_SOURCES = 			\
	anjuta-marshal.c			\
	anjuta-marshal.h              

anjuta-marshal.h: anjuta-marshal.list
	glib-genmarshal anjuta-marshal.list --header --prefix=anjuta_marshal > $@

anjuta-marshal.c: anjuta-marshal.list
	echo "#include \"anjuta-marshal.h\"" > $@ && \
	glib-genmarshal anjuta-marshal.list --body --prefix=anjuta_marshal >> $@

# The plugin
plugin_LTLIBRARIES = libanjuta-sourceview.la

# Plugin sources
libanjuta_sourceview_la_SOURCES = \
	$(BUILT_SOURCES)		\
	plugin.c \
	plugin.h \
	sourceview.c \
	sourceview.h \
	anjuta-view.h \
	anjuta-view.c \
	anjuta-document-loader.c \
	anjuta-document-loader.h \
	anjuta-document-saver.c \
	anjuta-document-saver.h \
	anjuta-document.c \
	anjuta-document.h \
	anjuta-encodings.h \
	anjuta-encodings.c \
	anjuta-languages-manager.h \
	anjuta-languages-manager.c \
	anjuta-convert.h \
	anjuta-convert.c \
	anjuta-utils.h \
	anjuta-utils.c \
	sourceview-prefs.h \
	sourceview-prefs.c \
	sourceview-private.h \
	sourceview-autocomplete.c \
	sourceview-autocomplete.h 
	
# Plugin dependencies
libanjuta_sourceview_la_LIBADD = \
	$(PCRE_LIBS) \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(GTKSOURCEVIEW_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(top_builddir)/tagmanager/libanjuta-ctags.la

EXTRA_DIST = \
	$(plugin_in_files) \
	$(sourceview_plugin_DATA) \
	$(sourceview_pixmaps_DATA) \
	$(sourceview_glade_DATA)
