if HAVE_PLUGIN_SOURCEVIEW

# Plugin glade file
sourceview_gladedir = $(anjuta_glade_dir)
sourceview_glade_DATA = anjuta-editor-sourceview.glade

# Plugin Icon file
sourceview_pixmapsdir = $(anjuta_image_dir)
sourceview_pixmaps_DATA = \
	anjuta-editor-sourceview-plugin-48.png \
	anjuta-linemark-16.png \
	anjuta-pcmark-16.png \
	anjuta-breakpoint-disabled-16.png \
	anjuta-breakpoint-enabled-16.png \
	anjuta-bookmark-16.png

# Plugin description file
plugin_in_files = sourceview.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

sourceview_plugindir = $(anjuta_plugin_dir)
sourceview_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GNOME_PRINT_UI_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(PLUGIN_SOURCEVIEW_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# Marshallers
BUILT_SOURCES = 			\
	anjuta-marshal.c			\
	anjuta-marshal.h              

anjuta-marshal.h: anjuta-marshal.list
	glib-genmarshal anjuta-marshal.list --header --prefix=anjuta_marshal > $@

anjuta-marshal.c: anjuta-marshal.list
	echo "#include \"anjuta-marshal.h\"" > $@ && \
	glib-genmarshal anjuta-marshal.list --body --prefix=anjuta_marshal >> $@

# The plugin
plugin_LTLIBRARIES = libanjuta-sourceview.la

# Plugin sources
libanjuta_sourceview_la_SOURCES = \
	$(BUILT_SOURCES)		\
	plugin.c \
	plugin.h \
	sourceview.c \
	sourceview.h \
	assist-window.h \
	assist-window.c \
	anjuta-view.h \
	anjuta-view.c \
	sourceview-prefs.h \
	sourceview-prefs.c \
	sourceview-private.h \
	sourceview-cell.c \
	sourceview-cell.h \
	sourceview-print.h \
	sourceview-print.c \
	assist-tip.h \
	assist-tip.c \
	sourceview-io.h \
	sourceview-io.c

libanjuta_sourceview_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_sourceview_la_LIBADD = \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(PLUGIN_SOURCEVIEW_LIBS) \
	$(GLADE_LIBS) \
	$(LIBANJUTA_LIBS)

prefs_glade_files = anjuta-editor-sourceview.glade
include $(top_srcdir)/scripts/build-schemas.mk

endif

EXTRA_DIST = \
	anjuta-marshal.list \
	$(plugin_in_files) \
	$(sourceview_plugin_DATA) \
	$(sourceview_pixmaps_DATA) \
	$(sourceview_glade_DATA)


