# Plugin glade file
cvs_plugin_gladedir = $(anjuta_glade_dir)
cvs_plugin_glade_DATA = anjuta-cvs-plugin.glade

cvs_plugin_pixmapsdir = $(anjuta_image_dir)
cvs_plugin_pixmaps_DATA = \
	anjuta-cvs-plugin.png

cvs_plugin_uidir = $(anjuta_ui_dir)
cvs_plugin_ui_DATA = anjuta-cvs.ui

plugin_in_files = anjuta-cvs.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

cvs_plugin_plugindir = $(anjuta_plugin_dir)
cvs_plugin_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(PCRE_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-cvs-plugin.la

libanjuta_cvs_plugin_la_LDFLAGS = -module
libanjuta_cvs_plugin_la_LIBADD = \
	$(LIBANJUTA_LIBS) $(PCRE_LIBS)

libanjuta_cvs_plugin_la_SOURCES = \
	plugin.c\
	plugin.h\
	cvs-actions.c\
	cvs-actions.h\
	cvs-execute.c\
	cvs-execute.h                     \
	cvs-callbacks.c                     \
	cvs-callbacks.h 
EXTRA_DIST = \
	$(plugin_in_files) \
	$(cvs_plugin_pixmaps_DATA) \
	$(cvs_plugin_plugin_DATA) \
	$(cvs_plugin_ui_DATA) \
	$(cvs_plugin_glade_DATA)
