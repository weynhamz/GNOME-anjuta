# Plugin UI file
loader_uidir = $(anjuta_ui_dir)
loader_ui_DATA =  anjuta-loader-plugin.ui

# Plugin Icon file
loader_pixmapsdir = $(anjuta_image_dir)
loader_pixmaps_DATA = anjuta-loader-plugin.png

# Plugin description file
plugin_in_files = anjuta-loader.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

loader_plugindir = $(anjuta_plugin_dir)
loader_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-loader.la

# Plugin sources
libanjuta_loader_la_SOURCES = plugin.c

libanjuta_loader_la_LDFLAGS = -module

libanjuta_loader_la_LIBADD = \
	$(LIBANJUTA_LIBS)\
	$(top_builddir)/libegg/libanjuta-egg.la

# Plugin dependencies

EXTRA_DIST = \
	$(plugin_in_files) \
	$(loader_plugin_DATA) \
	$(loader_ui_DATA) \
	$(loader_pixmaps_DATA)
