# Plugin description file
plugin_in_files = anjuta-terminal.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

gsettings_in_file = org.gnome.anjuta.terminal.gschema.xml.in
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PLUGIN_TERMINAL_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

if HAVE_PLUGIN_TERMINAL
# Plugin glade file
terminal_gladedir = $(anjuta_glade_dir)
dist_terminal_glade_DATA = anjuta-terminal-plugin.ui

# Plugin ui file
terminal_uidir = $(anjuta_ui_dir)
dist_terminal_ui_DATA = anjuta-terminal-plugin.xml

# Plugin Icon file
terminal_pixmapsdir = $(anjuta_image_dir)
dist_terminal_pixmaps_DATA = \
	anjuta-terminal-plugin.svg \
	anjuta-terminal-plugin-48.png

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-terminal.la

# Plugin sources
libanjuta_terminal_la_SOURCES = terminal.c

libanjuta_terminal_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_terminal_la_LIBADD = \
	$(PLUGIN_TERMINAL_LIBS) \
	$(LIBANJUTA_LIBS)

terminal_plugindir = $(anjuta_plugin_dir)
dist_terminal_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
endif

EXTRA_DIST = \
	$(plugin_in_files) \
        $(gsettings_in_file)

CLEANFILES = \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
