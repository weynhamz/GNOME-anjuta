# Plugin glade file
terminal_gladedir = $(anjuta_glade_dir)
terminal_glade_DATA = anjuta-terminal-plugin.glade

# Plugin Icon file
terminal_pixmapsdir = $(anjuta_image_dir)
terminal_pixmaps_DATA = \
	anjuta-terminal-plugin.png \
	preferences-terminal.png

# Plugin description file
plugin_in_files = anjuta-terminal.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

terminal_plugindir = $(anjuta_plugin_dir)
terminal_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
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

# Plugin dependencies
libanjuta_terminal_la_LIBADD = \
	$(VTE_LIBS) \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(terminal_plugin_DATA) \
	$(terminal_pixmaps_DATA) \
	$(terminal_glade_DATA)
