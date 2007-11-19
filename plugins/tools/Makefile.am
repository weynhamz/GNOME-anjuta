
# Plugin glade file
tools_gladedir = $(anjuta_glade_dir)
tools_glade_DATA = anjuta-tools.glade

# Plugin UI file
tools_uidir = $(anjuta_ui_dir)
tools_ui_DATA = anjuta-tools.ui 

# Plugin pixmaps
tools_pixmapsdir = $(anjuta_image_dir)
tools_pixmaps_DATA = \
	anjuta-tools-plugin.svg \
	anjuta-tools-plugin-48.png

# Plugin Icon file
plugin_in_files = anjuta-tools.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

tools_plugindir = $(anjuta_plugin_dir)
tools_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-tools.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_tools_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_tools_la_LIBADD = \
	$(GNOME_UI_LIBS) \
	$(GLADE_LIBS) \
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

EXTRA_DIST = \
	$(plugin_in_files) \
	$(tools_pixmaps_DATA) \
	$(tools_plugin_DATA) \
	$(tools_glade_DATA) \
	$(tools_ui_DATA)

SUBDIRS = \
	scripts 

