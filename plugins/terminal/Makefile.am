# Plugin glade file
terminal_gladedir = $(anjuta_glade_dir)
terminal_glade_DATA = anjuta-terminal-plugin.glade

# Plugin ui file
terminal_uidir = $(anjuta_ui_dir)
terminal_ui_DATA = anjuta-terminal-plugin.ui

# Plugin Icon file
terminal_pixmapsdir = $(anjuta_image_dir)
terminal_pixmaps_DATA = \
	anjuta-terminal-plugin.svg \
	anjuta-terminal-plugin-48.png

# Plugin description file
plugin_in_files = anjuta-terminal.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

terminal_plugindir = $(anjuta_plugin_dir)
terminal_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(VTE_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-terminal.la

# Plugin sources
libanjuta_terminal_la_SOURCES = terminal.c

libanjuta_terminal_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_terminal_la_LIBADD = \
	$(GCONF_LIBS) \
	$(GLADE_LIBS) \
	$(VTE_LIBS) \
	$(LIBANJUTA_LIBS)
	
prefs_glade_files = anjuta-terminal-plugin.glade
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(terminal_plugin_DATA) \
	$(terminal_pixmaps_DATA) \
	$(terminal_ui_DATA) \
	$(terminal_glade_DATA)
