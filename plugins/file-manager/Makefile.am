# Sample Makefile for a anjuta plugin.

# Plugin glade file
anjuta_gladedir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
anjuta_glade_DATA =  anjuta-file-manager-plugin.glade

# Plugin UI file
anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta-file-manager.ui

# Plugin Icon file
plugin_icondir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_icon_DATA = anjuta-file-manager-plugin.png

# Plugin description file
plugin_in_files = anjuta-file-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(GDL_CFLAGS)

# Where to install the plugin
plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@

# The plugin
plugin_LTLIBRARIES = libanjuta-file-manager.la

# Plugin sources
libanjuta_file_manager_la_SOURCES = \
	plugin.c \
	plugin.h \
	an_file_view.c

# Plugin dependencies

EXTRA_DIST = \
        $(plugin_in_files) \
        $(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA)
