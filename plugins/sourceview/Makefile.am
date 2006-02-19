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

# The plugin
plugin_LTLIBRARIES = libanjuta-sourceview.la

# Plugin sources
libanjuta_sourceview_la_SOURCES = \
	plugin.c \
	plugin.h \
	sourceview.c \
	sourceview.h

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
	$(sourceview_pixmaps_DATA)
