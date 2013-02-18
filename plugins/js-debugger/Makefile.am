# Plugin Icon file
js_debugger_pixmapsdir = $(anjuta_image_dir)
js_debugger_pixmaps_DATA = js_debugger.png

# Plugin description file
plugin_in_files = js_debugger.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

js_debugger_plugindir = $(anjuta_plugin_dir)
js_debugger_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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
	-DPACKAGE_SRC_DIR=\"$(srcdir)\" \
	$(LIBANJUTA_CFLAGS) 

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libjs_debugger.la

# Plugin sources
libjs_debugger_la_SOURCES = debugger-js.c debugger-js.h debugger-server.c debugger-server.h plugin.c plugin.h

libjs_debugger_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libjs_debugger_la_LIBADD = \
	$(LIBANJUTA_LIBS) 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(js_debugger_plugin_DATA) \
	$(js_debugger_pixmaps_DATA)

-include $(top_srcdir)/git.mk
