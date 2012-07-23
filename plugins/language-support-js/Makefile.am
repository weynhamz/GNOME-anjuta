# Sample Makefile for a anjuta plugin.

#SUBDIRS = unit_test

js_support_plugin_gladedir = $(anjuta_glade_dir)
js_support_plugin_glade_DATA = anjuta-language-javascript.ui

js_support_plugin_uidir = $(anjuta_ui_dir)
js_support_plugin_ui_DATA = anjuta-language-javascript.xml

# Plugin Icon file
js_support_plugin_pixmapsdir = $(anjuta_image_dir)
js_support_plugin_pixmaps_DATA = js_support_plugin.png

# Plugin description file
plugin_in_files = js-support-plugin.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

js_support_plugin_plugindir = $(anjuta_plugin_dir)
js_support_plugin_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
AM_CPPFLAGS = \
	-I$(top_srcdir)/plugins/symbol-db/anjuta-tags/ \
	$(WARN_CFLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DANJUTA_DATA_DIR=\"$(anjuta_data_dir)\" \
	-DANJUTA_PLUGIN_DIR=\"$(anjuta_plugin_dir)\" \
	-DANJUTA_IMAGE_DIR=\"$(anjuta_image_dir)\" \
	-DANJUTA_GLADE_DIR=\"$(anjuta_glade_dir)\" \
	-DANJUTA_UI_DIR=\"$(anjuta_ui_dir)\" \
	$(XML_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DPACKAGE_SRC_DIR=\"$(srcdir)\" \
	-DGIR_PATH=\"$(INTROSPECTION_GIRDIR)\" \
	-DGJS_PATH=\"$(gjsdir)\" \
	-DG_LOG_DOMAIN=\"language-support-js\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libjs-support-plugin.la

# Plugin sources
libjs_support_plugin_la_SOURCES = plugin.c plugin.h code-completion.c code-completion.h util.c util.h \
js-parser-y-tab.c js-parser-y-tab.h lex.yy.c lex.yy.h js-node.c js-node.h js-context.c js-context.h jsparse.c jsparse.h \
ijs-symbol.h ijs-symbol.c gir-symbol.c gir-symbol.h gi-symbol.c gi-symbol.h simple-symbol.c \
simple-symbol.h local-symbol.c local-symbol.h  node-symbol.c node-symbol.h import-symbol.c \
import-symbol.h dir-symbol.c dir-symbol.h std-symbol.c std-symbol.h database-symbol.c database-symbol.h \
db-anjuta-symbol.c db-anjuta-symbol.h jstypes.h prefs.h

libjs_support_plugin_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libjs_support_plugin_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(XML_LIBS)

gsettings_in_file = org.gnome.anjuta.plugins.js.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(plugin_in_files)\
	$(js_support_plugin_plugin_DATA)\
	$(js_support_plugin_glade_DATA)\
	$(js_support_plugin_ui_DATA)\
	$(js_support_plugin_pixmaps_DATA) \
        $(gsettings_in_file)

CLEANFILES = \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk

