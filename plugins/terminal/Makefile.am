# Sample Makefile for a anjuta plugin.

# Plugin glade file
anjuta_gladedir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
anjuta_glade_DATA =  anjuta-terminal-plugin.glade

# Plugin UI file
# anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
# anjuta_ui_DATA =  anjuta-terminal.ui

# Plugin Icon file
plugin_icondir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_icon_DATA = anjuta-terminal-plugin.png

# Plugin description file
plugin_in_files = anjuta-terminal.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; \
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u \
		-c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@

# The plugin
plugin_LTLIBRARIES = libanjuta-terminal.la

# Plugin sources
libanjuta_terminal_la_SOURCES = terminal.c

# Plugin dependencies

EXTRA_DIST = \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA)
