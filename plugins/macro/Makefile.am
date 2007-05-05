# Plugin glade file
macro_gladedir = $(anjuta_glade_dir)
macro_glade_DATA = anjuta-macro.glade

macro_macrosdir = $(anjuta_data_dir)
macro_macros_DATA = macros.xml

macro_pixmapsdir = $(anjuta_image_dir)
macro_pixmaps_DATA = \
	anjuta-macro.png

macro_uidir = $(anjuta_ui_dir)
macro_ui_DATA = anjuta-macro.ui

plugin_in_files = anjuta-macro.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

macro_plugindir = $(anjuta_plugin_dir)
macro_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(PCRE_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-macro.la

libanjuta_macro_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_macro_la_LIBADD = \
	$(GLADE_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS) $(PCRE_LIBS)

libanjuta_macro_la_SOURCES = \
	plugin.c\
	plugin.h\
	macro-actions.c\
	macro-actions.h\
	macro-db.c\
	macro-db.h                  \
	macro-dialog.c                  \
	macro-dialog.h                  \
	macro-edit.c                  \
	macro-edit.h                  \
	macro-util.c                  \
	macro-util.h 
	
EXTRA_DIST = \
	$(plugin_in_files) \
	$(macro_pixmaps_DATA) \
	$(macro_plugin_DATA) \
	$(macro_ui_DATA) \
	$(macro_glade_DATA) \
	$(macro_macros_DATA)
