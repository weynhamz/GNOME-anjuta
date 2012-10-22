# Plugin Icon file
jhbuild_pixmapsdir = $(anjuta_image_dir)
jhbuild_pixmaps_DATA = \
 	anjuta-jhbuild-plugin-48.png

# Plugin description file
plugin_in_files = anjuta-jhbuild.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

jhbuild_plugindir = $(anjuta_plugin_dir)
jhbuild_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(EXTRA_CFLAGS) \
	-DG_LOG_DOMAIN=\"jhbuild\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-jhbuild.la

# Plugin sources
libanjuta_jhbuild_la_SOURCES = \
	plugin.c \
	plugin.h

libanjuta_jhbuild_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)


EXTRA_DIST = \
	$(plugin_in_files) \
	$(jhbuild_plugin_DATA) \
	$(jhbuild_pixmaps_DATA)

-include $(top_srcdir)/git.mk
