
# Profiles
profile_profiledir = $(anjuta_data_dir)/profiles
profile_profile_DATA =  default.anjuta

# Plugin UI file
profile_uidir = $(anjuta_ui_dir)
profile_ui_DATA =  anjuta-default-profile.ui

# Plugin Icon file
profile_pixmapsdir = $(anjuta_image_dir)
profile_pixmaps_DATA = anjuta-default-profile-plugin.png

# Plugin description file
plugin_in_files = anjuta-default-profile.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

profile_plugindir = $(anjuta_plugin_dir)
profile_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBXML_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-default-profile.la

# Plugin sources
libanjuta_default_profile_la_SOURCES = \
	plugin.c \
	plugin.h

# Plugin dependencies
libanjuta_default_profile_la_LIBADD = $(LIBANJUTA_LIBS) $(LIBXML_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(profile_plugin_DATA) \
	$(profile_ui_DATA) \
	$(profile_pixmaps_DATA) \
	$(profile_profile_DATA)
