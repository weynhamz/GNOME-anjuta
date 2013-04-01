# Plugin UI file
project_uidir = $(anjuta_ui_dir)
dist_project_ui_DATA = anjuta-project-manager.xml

# Plugin glade file
project_gladedir = $(anjuta_glade_dir)
dist_project_glade_DATA = pm_dialogs.ui

# Plugin Icon file
project_pixmapsdir = $(anjuta_image_dir)
dist_project_pixmaps_DATA = \
	anjuta-project-manager-plugin.svg \
	anjuta-project-manager-plugin-48.png \
	anjuta-project-manager-plugin-16.png \
	gbf-build.png \
	gbf-install.png

# Plugin description file
plugin_in_files = anjuta-project-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

project_plugindir = $(anjuta_plugin_dir)
dist_project_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Additional marshaller
project-marshal.h: project-marshal.list
	$(AM_V_GEN)@GLIB_GENMARSHAL@ \
	        --prefix=pm_cclosure_marshal $(srcdir)/project-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

project-marshal.c: project-marshal.h project-marshal.list
	$(AM_V_GEN)echo "#include \"project-marshal.h\"" > xgenc-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=pm_cclosure_marshal $(srcdir)/project-marshal.list --body >> xgenc-gmc \
	&& cp xgenc-gmc $(@F) \
	&& rm -f xgenc-gmc

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
BUILT_SOURCES = project-marshal.c project-marshal.h

libanjuta_project_manager_la_SOURCES = \
	plugin.c \
	plugin.h \
	tree-data.h		\
	tree-data.c		\
	project-model.h		\
	project-model.c		\
	project-view.h		\
	project-view.c		\
	project-chooser.c	\
	project-chooser.h	\
	project-util.h		\
	project-util.c		\
	project.h		\
	project.c		\
	dialogs.c		\
	dialogs.h		\
	$(BUILT_SOURCES)

libanjuta_project_manager_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_project_manager_la_LIBADD = \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	project-marshal.list

DISTCLEANFILES = \
	$(project_plugin_DATA) \
	$(project_in_files)

-include $(top_srcdir)/git.mk
