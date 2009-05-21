# Plugin glade file
cvs_plugin_gladedir = $(anjuta_glade_dir)
cvs_plugin_glade_DATA = anjuta-cvs-plugin.ui

cvs_plugin_pixmapsdir = $(anjuta_image_dir)
cvs_plugin_pixmaps_DATA = \
	anjuta-cvs-plugin.svg \
	anjuta-cvs-plugin-48.png

cvs_plugin_uidir = $(anjuta_ui_dir)
cvs_plugin_ui_DATA = anjuta-cvs.ui

plugin_in_files = anjuta-cvs.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

cvs_plugin_plugindir = $(anjuta_plugin_dir)
cvs_plugin_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-cvs-plugin.la

libanjuta_cvs_plugin_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_cvs_plugin_la_LIBADD = \
	$(GIO_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_cvs_plugin_la_SOURCES = \
	plugin.c \
	plugin.h \
	cvs-actions.c \
	cvs-actions.h \
	cvs-execute.c \
	cvs-execute.h \
	cvs-callbacks.c \
	cvs-callbacks.h \
	cvs-interface.c \
	cvs-interface.h
	
prefs_ui_files = anjuta-cvs-plugin.ui
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(cvs_plugin_pixmaps_DATA) \
	$(cvs_plugin_plugin_DATA) \
	$(cvs_plugin_ui_DATA) \
	$(cvs_plugin_glade_DATA)

-include $(top_srcdir)/git.mk
