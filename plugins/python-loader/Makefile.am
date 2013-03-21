# Plugin description file
plugin_in_files = anjuta-python-loader.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

if HAVE_PYTHON

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

dist_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DANJUTA_DATA_DIR=\"$(anjuta_data_dir)\" \
	-DANJUTA_PLUGIN_DIR=\"$(anjuta_plugin_dir)\" \
	-DANJUTA_IMAGE_DIR=\"$(anjuta_image_dir)\" \
	-DANJUTA_GLADE_DIR=\"$(anjuta_glade_dir)\" \
	-DANJUTA_UI_DIR=\"$(anjuta_ui_dir)\" \
	$(LIBANJUTA_CFLAGS) \
	$(PYTHON_CFLAGS) \
	$(PYGOBJECT_CFLAGS) \
	-DPACKAGE_SRC_DIR=\"$(srcdir)\" \
	-DG_LOG_DOMAIN=\"python-loader\"

# The plugin
plugin_LTLIBRARIES = libanjuta-python-loader.la

# Plugin sources
libanjuta_python_loader_la_SOURCES = plugin.c plugin.h

libanjuta_python_loader_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_python_loader_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(PYTHON_LIBS) \
	$(PYGOBJECT_LIBS)

endif # HAVE_PYTHON

EXTRA_DIST = \
	$(plugin_in_files)

-include $(top_srcdir)/git.mk

