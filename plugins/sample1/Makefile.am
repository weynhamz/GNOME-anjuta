# Sample Makefile for a anjuta plugin.

# Plugin UI file
sample_uidir = $(anjuta_ui_dir)
sample_ui_DATA =  anjuta-sample.ui

# Plugin Icon file
sample_pixmapsdir = $(anjuta_image_dir)
sample_pixmaps_DATA = anjuta-sample-plugin.png

# Plugin description file
plugin_in_files = anjuta-sample.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

sample_plugindir = $(anjuta_plugin_dir)
sample_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-sample.la

# Plugin sources
libanjuta_sample_la_SOURCES = \
	plugin.c \
	plugin.h

# Plugin dependencies
libanjuta_sample_la_LIBADD = \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(sample_plugin_DATA) \
	$(sample_ui_DATA) \
	$(sample_pixmaps_DATA)
