# Sample Makefile for a anjuta plugin.

# Plugin UI file
glade_uidir = $(anjuta_ui_dir)
glade_ui_DATA = anjuta-glade.ui

# Plugin Icon file
glade_pixmapsdir = $(anjuta_image_dir)
glade_pixmaps_DATA = anjuta-glade-plugin.png

# Plugin description file
plugin_in_files = anjuta-glade.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

glade_plugindir = $(anjuta_plugin_dir)
glade_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PLUGIN_GLADE_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-glade.la

# Plugin sources
libanjuta_glade_la_SOURCES = plugin.c plugin.h

# Plugin dependencies
libanjuta_glade_la_LIBADD = \
	$(PLUGIN_GLADE_LIBS) \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(glade_plugin_DATA) \
	$(glade_ui_DATA) \
	$(glade_pixmaps_DATA)
