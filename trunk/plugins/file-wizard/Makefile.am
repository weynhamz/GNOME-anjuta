# Plugin glade file
wizard_gladedir = $(anjuta_glade_dir)
wizard_glade_DATA =  anjuta-file-wizard.glade

# Plugin pixmaps
wizard_pixmapsdir = $(anjuta_image_dir)
wizard_pixmaps_DATA = \
	anjuta-file-wizard-plugin.png

# Plugin Icon file
plugin_in_files = anjuta-file-wizard.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

wizard_plugindir = $(anjuta_plugin_dir)
wizard_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-file-wizard.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_file_wizard_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_file_wizard_la_LIBADD = \
	$(GLADE_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_file_wizard_la_SOURCES= \
	file.c \
	file.h \
	plugin.c \
	plugin.h

EXTRA_DIST = \
	$(plugin_in_files) \
	$(wizard_pixmaps_DATA) \
	$(wizard_plugin_DATA) \
	$(wizard_glade_DATA)
