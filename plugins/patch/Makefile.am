# Plugin UI file
patch_uidir = $(anjuta_ui_dir)
patch_ui_DATA = anjuta-patch.ui

# Plugin Icon file
patch_pixmapsdir = $(anjuta_image_dir)
patch_pixmaps_DATA = \
	anjuta-patch-plugin.png

# Plugin description file
plugin_in_files = anjuta-patch.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache


patch_plugindir = $(anjuta_plugin_dir)
patch_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) 

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-patch.la

# Plugin sources
libanjuta_patch_la_SOURCES = \
	plugin.c \
	plugin.h \
	patch-plugin.c \
	patch-plugin.h

# Plugin dependencies
libanjuta_patch_la_LIBADD = \
	$(LIBANJUTA_LIBS) 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(patch_plugin_DATA) \
	$(patch_ui_DATA) \
	$(patch_pixmaps_DATA)
