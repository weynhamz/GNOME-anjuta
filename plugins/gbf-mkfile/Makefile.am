# Plugin UI file
plugin_uidir = $(anjuta_ui_dir)
plugin_ui_DATA = 

# Plugin glade file
plugin_gladedir = $(anjuta_glade_dir)
plugin_glade_DATA =

# Plugin icon file
plugin_pixmapsdir = $(anjuta_image_dir)
plugin_pixmaps_DATA = gbf-mkfile-plugin-48.png

# Plugin scripts
scriptsdir = $(bindir)
scripts_SCRIPTS = gbf-mkfile-parse

# Plugin description file
plugin_in_files = gbf-mkfile.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugindir = $(anjuta_plugin_dir)
plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = 					\
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DSCRIPTS_DIR=\"$(scriptsdir)\"

plugin_LTLIBRARIES = \
	libgbf-mkfile.la

libgbf_mkfile_la_SOURCES = \
	plugin.c \
	plugin.h \
	gbf-mkfile-project.c \
	gbf-mkfile-project.h \
	gbf-mkfile-config.c \
	gbf-mkfile-config.h \
	gbf-mkfile-properties.c \
	gbf-mkfile-properties.h 

libgbf_mkfile_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libgbf_mkfile_la_LIBADD = \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(plugin_DATA) \
	$(plugin_ui_DATA) \
	$(plugin_pixmaps_DATA)

DISTCLEANFILES = \
	$(plugin_DATA) \
	$(plugin_in_files)

SUBDIRS = GBF

