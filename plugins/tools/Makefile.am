
# Plugin glade file
tools_gladedir = $(anjuta_glade_dir)
dist_tools_glade_DATA = anjuta-tools.ui

# Plugin UI file
tools_uidir = $(anjuta_ui_dir)
dist_tools_ui_DATA = anjuta-tools.xml

# Plugin pixmaps
tools_pixmapsdir = $(anjuta_image_dir)
dist_tools_pixmaps_DATA = \
	anjuta-tools-plugin.svg \
	anjuta-tools-plugin-48.png

# Plugin Icon file
plugin_in_files = anjuta-tools.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

tools_plugindir = $(anjuta_plugin_dir)
dist_tools_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-tools.la

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_tools_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_tools_la_LIBADD = \
	$(LIBANJUTA_LIBS)

libanjuta_tools_la_SOURCES= \
	plugin.c \
	plugin.h \
	dialog.c \
	dialog.h \
	fileop.c \
	fileop.h \
	tool.c \
	tool.h \
	editor.c \
	editor.h \
	execute.c \
	execute.h \
	variable.c \
	variable.h

gsettings_in_file = org.gnome.anjuta.tools.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(plugin_in_files) \
        $(gsettings_in_file)

CLEANFILES = \
        $(gsettings_SCHEMAS)

SUBDIRS = \
	scripts 


-include $(top_srcdir)/git.mk
