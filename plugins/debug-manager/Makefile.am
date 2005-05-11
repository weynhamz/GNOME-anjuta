# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta-debug-manager.glade

plugin_pixmapsdir = $(anjuta_image_dir)
plugin_pixmaps_DATA = \
	anjuta-debug-manager.plugin.png

anjuta_uidir = $(anjuta_ui_dir)
anjuta_ui_DATA =  anjuta-debug-manager.ui

plugin_in_files = anjuta-debug-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-debug-manager.la

libanjuta_debug_manager_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_debug_manager_la_LIBADD = \
	$(top_builddir)/libanjuta/libanjuta.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

libanjuta_debug_manager_la_SOURCES = \
	plugin.c \
	plugin.h \
	attach_process.c \
	attach_process.h \
	utilities.c \
	utilities.h

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA) \
	$(anjuta_glade_DATA)

SUBDIRS = \
	images 

