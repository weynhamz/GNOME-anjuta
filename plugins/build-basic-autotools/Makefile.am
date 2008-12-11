
# Plugin glade file
build_gladedir = $(anjuta_glade_dir)
build_glade_DATA = anjuta-build-basic-autotools-plugin.glade

# Plugin UI file
build_uidir = $(anjuta_ui_dir)
build_ui_DATA = anjuta-build-basic-autotools-plugin.ui

# Plugin Icon file
build_pixmapsdir = $(anjuta_image_dir)
build_pixmaps_DATA = \
	anjuta-build-basic-autotools-plugin-48.png \
	anjuta-build-basic-autotools-plugin.svg \
	anjuta-build-16.png \
	anjuta-build-24.png

# Plugin description file
plugin_in_files = anjuta-build-basic-autotools.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

build_plugindir = $(anjuta_plugin_dir)
build_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Plugin data files
build_datadir = $(anjuta_data_dir)/build
build_data_DATA = automake-c.filters

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-build-basic-autotools.la

# Plugin sources
libanjuta_build_basic_autotools_la_SOURCES = \
	plugin.c \
	plugin.h \
	executer.c \
	executer.h \
	build-options.c \
	build-options.h \
	configuration-list.c \
	configuration-list.h \
	program.c \
	program.h

# Plugin dependencies
libanjuta_build_basic_autotools_la_LIBADD = \
	$(GLADE_LIBS) \
	$(GNOME_LIBS) \
	$(GIO_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_build_basic_autotools_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

prefs_glade_files = anjuta-build-basic-autotools-plugin.glade
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(build_plugin_DATA) \
	$(build_ui_DATA) \
	$(build_pixmaps_DATA) \
	$(build_glade_DATA) \
	$(build_data_DATA)
