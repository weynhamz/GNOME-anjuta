# Sample Makefile for a anjuta plugin.

# Plugin glade file
indent_gladedir = $(anjuta_glade_dir)
indent_glade_DATA = indent.glade

# Plugin UI file
indent_uidir = $(anjuta_ui_dir)
indent_ui_DATA =  anjuta-indent.ui

# Plugin Icon file
indent_pixmapsdir = $(anjuta_image_dir)
indent_pixmaps_DATA = anjuta-indent-plugin.png indent_auto.xpm indent_set.xpm

# Plugin description file
plugin_in_files = indent.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

indent_plugindir = $(anjuta_plugin_dir)
indent_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Test file for anjuta indent
indent_datadir = $(anjuta_data_dir)
indent_data_DATA = indent_test.c

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
plugin_LTLIBRARIES = libanjuta-indent.la

# Plugin sources
libanjuta_indent_la_SOURCES = \
	plugin.c \
	plugin.h \
  indent-dialog.c \
	indent-dialog.h \
	indent-util.c \
	indent-util.h

libanjuta_indent_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_indent_la_LIBADD = \
	$(GTK_LIBS) \
	$(LIBANJUTA_LIBS)

prefs_glade_files = indent.glade
include $(top_srcdir)/scripts/build-schemas.mk

libanjuta_indent_la_DEPENDENCIES = \
	build-schema-files 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(indent_plugin_DATA) \
	$(indent_ui_DATA) \
	$(indent_pixmaps_DATA) \
	$(indent_data) \
	$(indent_glade_DATA)
