# Plugin UI files
indent_python_uidir = $(anjuta_ui_dir)
dist_indent_python_ui_DATA = anjuta-indentation-python-style.xml

# Plugin Glade file
indent_python_gladedir = $(anjuta_glade_dir)
dist_indent_python_glade_DATA = anjuta-indentation-python-style.ui

# Plugin Icon file
indent_python_pixmapsdir = $(anjuta_image_dir)
dist_indent_python_pixmaps_DATA = anjuta-indentation-python-style-plugin.png

# Plugin description file
plugin_in_files = anjuta-indentation-python-style.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

indent_python_plugindir = $(anjuta_plugin_dir)
dist_indent_python_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = \
	$(XML_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"indentation-python-style\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-indentation-python-style.la

# Plugin sources
libanjuta_indentation_python_style_la_SOURCES = \
	plugin.c \
	plugin.h \
	python-indentation.c \
	python-indentation.h

libanjuta_indentation_python_style_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_indentation_python_style_la_LIBADD = $(LIBANJUTA_LIBS) 

gsettings_in_file = org.gnome.anjuta.plugins.indent-python.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(plugin_in_files) \
  $(gsettings_in_file)

CLEANFILES = \
  $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
