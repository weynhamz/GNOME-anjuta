# Plugin glade file
wizard_gladedir = $(anjuta_glade_dir)
wizard_glade_DATA =  anjuta-project-mkfile-import.glade

# Plugin pixmaps
wizard_pixmapsdir = $(anjuta_image_dir)
wizard_pixmaps_DATA = \
	anjuta-project-mkfile-import-plugin.png

# Plugin Icon file
plugin_in_files = anjuta-project-mkfile-import.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

wizard_plugindir = $(anjuta_plugin_dir)
wizard_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-project-mkfile-import.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_project_mkfile_import_la_LIBADD = \
	$(LIBANJUTA_LIBS)

libanjuta_project_mkfile_import_la_SOURCES= \
	plugin.c \
	plugin.h \
	project-mkfile-import.c \
	project-mkfile-import.h 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(wizard_pixmaps_DATA) \
	$(wizard_plugin_DATA) \
	$(wizard_glade_DATA)
