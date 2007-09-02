
# Plugin glade file
file_manager_gladedir = $(anjuta_glade_dir)
file_manager_glade_DATA = anjuta-file-manager-plugin.glade

# Plugin UI file
file_manager_uidir = $(anjuta_ui_dir)
file_manager_ui_DATA = anjuta-file-manager.ui

# Plugin Icon file
file_manager_pixmapsdir = $(anjuta_image_dir)
file_manager_pixmaps_DATA = anjuta-file-manager-plugin.png

# Plugin description file
plugin_in_files = anjuta-file-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

file_manager_plugindir = $(anjuta_plugin_dir)
file_manager_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GDL_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-file-manager.la

# Plugin sources
libanjuta_file_manager_la_SOURCES = \
	plugin.c \
	plugin.h \
	an_file_view.c

libanjuta_file_manager_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_file_manager_la_LIBADD = \
	$(GDL_LIBS) \
	$(LIBANJUTA_LIBS)
	
prefs_glade_files = anjuta-file-manager-plugin.glade
include $(top_srcdir)/scripts/build-schemas.mk

libanjuta_file_manager_la_DEPENDENCIES = \
	build-schema-files 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(file_manager_plugin_DATA) \
	$(file_manager_ui_DATA) \
	$(file_manager_glade_DATA) \
	$(file_manager_pixmaps_DATA)
