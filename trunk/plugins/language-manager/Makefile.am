# Plugin Icon file
language_manager_pixmapsdir = $(anjuta_image_dir)
language_manager_pixmaps_DATA = \
	anjuta-language-manager-plugin.svg \
	anjuta-language-manager-plugin-48.png

language_manager_xmldir = $(anjuta_data_dir)
language_manager_xml_DATA = languages.xml

# Plugin description file
plugin_in_files = language-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

language_manager_plugindir = $(anjuta_plugin_dir)
language_manager_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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
plugin_LTLIBRARIES = liblanguage-manager.la

# Plugin sources
liblanguage_manager_la_SOURCES = plugin.c plugin.h

# Plugin dependencies
liblanguage_manager_la_LIBADD = \
	$(LIBANJUTA_LIBS) 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(language_manager_plugin_DATA) \
	$(langauge_manager_xml_DATA) \
	$(language_manager_pixmaps_DATA) \
	languages.xml

