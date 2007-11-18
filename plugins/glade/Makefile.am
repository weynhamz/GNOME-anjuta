
if HAVE_PLUGIN_GLADE

# Plugin UI file
glade_uidir = $(anjuta_ui_dir)
glade_ui_DATA = anjuta-glade.ui

# Plugin Icon file
glade_pixmapsdir = $(anjuta_image_dir)
glade_pixmaps_DATA = \
	anjuta-glade-plugin.svg \
	anjuta-glade-plugin-48.png

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
libanjuta_glade_la_SOURCES = plugin.c plugin.h \
	anjuta-design-document.h                      \
	anjuta-design-document.c 

libanjuta_glade_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_glade_la_LIBADD = \
	$(PLUGIN_GLADE_LIBS) \
	$(LIBANJUTA_LIBS)
endif

EXTRA_DIST = \
	$(plugin_in_files) \
	$(glade_plugin_DATA) \
	$(glade_ui_DATA) \
	$(glade_pixmaps_DATA)

