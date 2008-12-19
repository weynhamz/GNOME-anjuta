# Plugin UI file
loader_uidir = $(anjuta_ui_dir)
loader_ui_DATA =  anjuta-loader-plugin.ui

# Plugin Icon file
loader_pixmapsdir = $(anjuta_image_dir)
loader_pixmaps_DATA = \
	anjuta-loader-plugin.svg \
	anjuta-loader-plugin-48.png

# Plugin description file
plugin_in_files = anjuta-loader.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

loader_plugindir = $(anjuta_plugin_dir)
loader_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"libanjuta-loader\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-loader.la

# Plugin sources
libanjuta_loader_la_SOURCES = \
	plugin.c \
	plugin.h \
	dnd.c \
	dnd.h \
	anjuta-recent-chooser-menu.c \
	anjuta-recent-chooser-menu.h

libanjuta_loader_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_loader_la_LIBADD = \
	$(GTK_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS)

# Plugin dependencies

EXTRA_DIST = \
	$(plugin_in_files) \
	$(loader_plugin_DATA) \
	$(loader_ui_DATA) \
	$(loader_pixmaps_DATA)
