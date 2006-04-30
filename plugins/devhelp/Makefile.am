# Sample Makefile for a anjuta plugin.

# Plugin UI file
devhelp_uidir = $(anjuta_ui_dir)
devhelp_ui_DATA = anjuta-devhelp.ui

# Plugin Icon file
devhelp_pixmapsdir = $(anjuta_image_dir)
devhelp_pixmaps_DATA = anjuta-devhelp-plugin.png

# Plugin description file
plugin_in_files = anjuta-devhelp.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

devhelp_plugindir = $(anjuta_plugin_dir)
devhelp_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PLUGIN_DEVHELP_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-devhelp.la

# Plugin sources
libanjuta_devhelp_la_SOURCES = \
	plugin.c \
	plugin.h \
	htmlview.c \
	htmlview.h

# Plugin dependencies
libanjuta_devhelp_la_LIBADD = \
	$(PLUGIN_DEVHELP_LIBS) \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(devhelp_plugin_DATA) \
	$(devhelp_ui_DATA) \
	$(devhelp_pixmaps_DATA)
