# Plugin glade file
indent_c_gladedir = $(anjuta_glade_dir)
dist_indent_c_glade_DATA = anjuta-indentation-c-style.ui

# Plugin UI file
indent_c_uidir = $(anjuta_ui_dir)
dist_indent_c_ui_DATA = anjuta-indentation-c-style.xml

# Plugin Icon file
indent_c_pixmapsdir = $(anjuta_image_dir)
dist_indent_c_pixmaps_DATA = \
 	anjuta-indentation-c-style-plugin.png \
	anjuta-indentation-c-style-auto-16.png \
	anjuta-indentation-c-style-auto-24.png

# Plugin description file
plugin_in_files = anjuta-indentation-c-style.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

indent_c_plugindir = $(anjuta_plugin_dir)
dist_indent_c_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(EXTRA_CFLAGS) \
	-DG_LOG_DOMAIN=\"indentation-c-style\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-indentation-c-style.la

# Plugin sources
libanjuta_indentation_c_style_la_SOURCES = \
	plugin.c \
	plugin.h \
	indentation.c \
	indentation.h

libanjuta_indentation_c_style_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

gsettings_in_file = org.gnome.anjuta.plugins.indent-c.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@


EXTRA_DIST = \
	$(plugin_in_files) \
        $(gsettings_in_file)

CLEANFILES = \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
