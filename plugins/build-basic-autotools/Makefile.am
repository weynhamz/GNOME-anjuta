
# Plugin glade file
build_gladedir = $(anjuta_glade_dir)
dist_build_glade_DATA = anjuta-build-basic-autotools-plugin.ui

# Plugin UI file
build_uidir = $(anjuta_ui_dir)
dist_build_ui_DATA = anjuta-build-basic-autotools-plugin.xml

# Plugin Icon file
build_pixmapsdir = $(anjuta_image_dir)
dist_build_pixmaps_DATA = \
	anjuta-build-basic-autotools-plugin-48.png \
	anjuta-build-basic-autotools-plugin.svg \
	anjuta-build-16.png \
	anjuta-build-24.png

# Plugin description file
plugin_in_files = anjuta-build-basic-autotools.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

build_plugindir = $(anjuta_plugin_dir)
dist_build_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Plugin data files
build_datadir = $(anjuta_data_dir)/build
dist_build_data_DATA = automake-c.filters

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
	build.c \
	build.h \
	build-options.c \
	build-options.h \
	configuration-list.c \
	configuration-list.h \
	program.c \
	program.h

# Plugin dependencies
libanjuta_build_basic_autotools_la_LIBADD = \
	$(GIO_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_build_basic_autotools_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

gsettings_in_file = org.gnome.anjuta.plugins.build.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(plugin_in_files) \
        $(gsettings_in_file)

CLEANFILES = \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
