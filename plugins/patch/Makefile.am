# Plugin UI file
patch_uidir = $(anjuta_ui_dir)
patch_ui_DATA = anjuta-patch.xml

# Plugin glade file
patch_gladedir = $(anjuta_glade_dir)
patch_glade_DATA = patch-plugin.ui

# Plugin Icon file
patch_pixmapsdir = $(anjuta_image_dir)
patch_pixmaps_DATA = \
	anjuta-patch-plugin.svg \
	anjuta-patch-plugin-48.png

# Plugin description file
plugin_in_files = anjuta-patch.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache


patch_plugindir = $(anjuta_plugin_dir)
patch_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
AM_CPPFLAGS= \
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

libanjuta_patch_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_patch_la_LIBADD = \
	$(LIBANJUTA_LIBS) 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(patch_plugin_DATA) \
	$(patch_ui_DATA) \
	$(patch_pixmaps_DATA) \
	$(patch_glade_DATA)

-include $(top_srcdir)/git.mk
