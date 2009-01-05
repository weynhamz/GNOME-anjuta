# Plugin file
plugin_in_files = anjuta-starter.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

starter_plugindir = $(anjuta_plugin_dir)
starter_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

starter_pixmapsdir = $(anjuta_image_dir)
starter_pixmaps_DATA = starter-logo.png 

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-starter.la

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GDL_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_starter_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_starter_la_LIBADD = \
	$(GDL_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_starter_la_SOURCES= \
	plugin.c \
	plugin.h \
	starter.h \
	starter.c

starter_pixmapsdir = $(anjuta_image_dir)
starter_pixmaps_DATA = \
	starter_logo.png
	
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(starter_plugin_DATA) \
	$(starter_pixmaps_DATA) 
