# Plugin UI file
project_uidir = $(anjuta_ui_dir)
project_ui_DATA = anjuta-project-manager.ui

# Plugin glade file
project_gladedir = $(anjuta_glade_dir)
project_glade_DATA = create_dialogs.glade

# Plugin Icon file
project_pixmapsdir = $(anjuta_image_dir)
project_pixmaps_DATA = \
	anjuta-project-manager-plugin.svg \
	anjuta-project-manager-plugin-48.png \
	gbf-build.png \
	gbf-install.png

# Plugin description file
plugin_in_files = anjuta-project-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

project_plugindir = $(anjuta_plugin_dir)
project_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"libanjuta-project-manager\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-project-manager.la

# Plugin sources
libanjuta_project_manager_la_SOURCES = \
	plugin.c \
	plugin.h \
	gbf-tree-data.h		\
	gbf-tree-data.c		\
	gbf-project-model.h	\
	gbf-project-model.c	\
	gbf-project-view.h	\
	gbf-project-view.c	\
	gbf-project-util.h	\
	gbf-project-util.c

libanjuta_project_manager_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_project_manager_la_LIBADD = \
	$(GNOME_UI_LIBS) \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(project_plugin_DATA) \
	$(project_ui_DATA) \
	$(project_pixmaps_DATA) \
	$(project_glade_DATA)

DISTCLEANFILES = \
	$(project_plugin_DATA) \
	$(project_in_files)

-include $(top_srcdir)/git.mk
