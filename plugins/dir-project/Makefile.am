# Plugin UI file
plugin_uidir = $(anjuta_ui_dir)
dist_plugin_ui_DATA =

# Plugin glade file
plugin_gladedir = $(anjuta_glade_dir)
dist_plugin_glade_DATA =

# Plugin data file
plugin_datadir = $(anjuta_data_dir)
dist_plugin_data_DATA = sources.list

# Plugin icon file
plugin_pixmapsdir = $(anjuta_image_dir)
dist_plugin_pixmaps_DATA = dir-project-plugin-48.png

# Plugin description file
plugin_in_files = dir-project.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugindir = $(anjuta_plugin_dir)
dist_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = 					\
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GIO_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"dir-project\"

plugin_LTLIBRARIES = \
	libdir-project.la

libdir_project_la_SOURCES = \
	plugin.c \
	plugin.h \
	dir-project.c \
	dir-project.h \
	dir-node.c \
	dir-node.h

libdir_project_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libdir_project_la_LIBADD = \
	$(GIO_LIBS) \
	$(LIBANJUTA_LIBS)


EXTRA_DIST = \
	$(plugin_in_files)

DISTCLEANFILES = \
	$(plugin_DATA)

-include $(top_srcdir)/git.mk
