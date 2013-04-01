# Plugin glade file
quick_open_gladedir = $(anjuta_glade_dir)
dist_quick_open_glade_DATA = anjuta-quick-open.ui

# Plugin UI file
quick_open_uidir = $(anjuta_ui_dir)
dist_quick_open_ui_DATA =  anjuta-quick-open.xml

# Plugin Icon file
quick_open_pixmapsdir = $(anjuta_image_dir)
dist_quick_open_pixmaps_DATA = anjuta-quick-open-48.png

# Plugin description file
plugin_in_files = anjuta-quick-open.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

quick_open_plugindir = $(anjuta_plugin_dir)
dist_quick_open_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-quick-open.la

AM_CPPFLAGS= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_quick_open_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_quick_open_la_LIBADD = \
	$(LIBANJUTA_LIBS)

libanjuta_quick_open_la_SOURCES = \
	plugin.c \
	plugin.h \
	quick-open-dialog.c \
	quick-open-dialog.h

EXTRA_DIST = \
	$(plugin_in_files) \
	$(schema_in_files)

-include $(top_srcdir)/git.mk
