[+ autogen5 template +]
# Sample Makefile for a anjuta plugin.
[+IF (=(get "HasUI") "1")+]
# Plugin UI file
[+(string->c-name! (string-downcase (get "PluginName")))+]_uidir = $(anjuta_ui_dir)
[+(string->c-name! (string-downcase (get "PluginName")))+]_ui_DATA =  [+PluginName+].ui
[+ENDIF+]
[+IF (=(get "HasGladeFile") "1")+]
# Plugin Glade file
[+(string->c-name! (string-downcase (get "PluginName")))+]_gladedir = $(anjuta_glade_dir)
[+(string->c-name! (string-downcase (get "PluginName")))+]_glade_DATA =  [+PluginName+].ui
[+ENDIF+]
# Plugin Icon file
[+(string->c-name! (string-downcase (get "PluginName")))+]_pixmapsdir = $(anjuta_image_dir)
[+(string->c-name! (string-downcase (get "PluginName")))+]_pixmaps_DATA = [+PluginName+].png

# Plugin description file
plugin_in_files = [+PluginName+].plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

[+(string->c-name! (string-downcase (get "PluginName")))+]_plugindir = $(anjuta_plugin_dir)
[+(string->c-name! (string-downcase (get "PluginName")))+]_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
INCLUDES = \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = lib[+PluginName+].la

# Plugin sources
lib[+(string->c-name! (string-downcase (get "PluginName")))+]_la_SOURCES = plugin.c plugin.h

# Plugin dependencies
lib[+(string->c-name! (string-downcase (get "PluginName")))+]_la_LIBADD = \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$([+(string->c-name! (string-downcase (get "PluginName")))+]_plugin_DATA) \
	[+IF (=(get "HasUI") "1")+]$([+(string->c-name! (string-downcase (get "PluginName")))+]_ui_DATA) \[+ENDIF+]
	[+IF (=(get "HasGladeFile") "1")+]$([+(string->c-name! (string-downcase (get "PluginName")))+]_glade_DATA) \[+ENDIF+]
	$([+(string->c-name! (string-downcase (get "PluginName")))+]_pixmaps_DATA)
