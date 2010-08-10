# Plugin UI files
python_plugin_uidir = $(anjuta_ui_dir)
python_plugin_ui_DATA =  python-plugin.xml

# Plugin Glade file
python_plugin_gladedir = $(anjuta_glade_dir)
python_plugin_glade_DATA = python-plugin-properties.ui

# Plugin Icon file
python_plugin_pixmapsdir = $(anjuta_image_dir)
python_plugin_pixmaps_DATA = python_plugin.png

# Plugin Python Scripts
scriptsdir = $(anjuta_plugin_dir)
scripts_SCRIPTS = anjuta-python-autocomplete.py

# Plugin description file
plugin_in_files = python-plugin.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

python_plugin_plugindir = $(anjuta_plugin_dir)
python_plugin_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = \
	$(LIBXML_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DSCRIPTS_DIR=\"$(scriptsdir)\" \
	-DG_LOG_DOMAIN=\"language-support-python\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libpython_plugin.la

# Plugin sources
libpython_plugin_la_SOURCES = plugin.c plugin.h python-assist.c python-assist.h python-utils.c python-utils.h

# Plugin dependencies
libpython_plugin_la_LIBADD = \
	$(LIBANJUTA_LIBS) 

prefs_ui_files = $(python_plugin_glade_DATA)
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(python_plugin_plugin_DATA) \
	$(python_plugin_ui_DATA) \
	$(python_plugin_glade_DATA) \
	$(python_plugin_pixmaps_DATA)
