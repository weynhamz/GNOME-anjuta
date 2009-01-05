# Plugin UI file
plugin_uidir = $(anjuta_ui_dir)
plugin_ui_DATA = 

# Plugin glade file
plugin_gladedir = $(anjuta_glade_dir)
plugin_glade_DATA = gbf-am-dialogs.glade

# Plugin icon file
plugin_pixmapsdir = $(anjuta_image_dir)
plugin_pixmaps_DATA = gbf-am-plugin-48.png

# Plugin scripts
scriptsdir = $(bindir)
scripts_SCRIPTS = gbf-am-parse

# Plugin description file
plugin_in_files = gbf-am.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugindir = $(anjuta_plugin_dir)
plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = 					\
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DSCRIPTS_DIR=\"$(scriptsdir)\"

plugin_LTLIBRARIES = \
	libgbf-am.la

libgbf_am_la_SOURCES = \
	plugin.c \
	plugin.h \
	gbf-am-project.c \
	gbf-am-project.h \
	gbf-am-config.c \
	gbf-am-config.h \
	gbf-am-properties.c \
	gbf-am-properties.h 

libgbf_am_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libgbf_am_la_LIBADD = \
	$(LIBANJUTA_LIBS)

# Test program

noinst_PROGRAMS = test

test_SOURCES = \
	test.c

test_LDADD = \
	libgbf-am.la \
	$(LIBANJUTA_LIBS)


EXTRA_DIST = \
	$(plugin_in_files) \
	$(plugin_DATA) \
	$(plugin_ui_DATA) \
	$(plugin_pixmaps_DATA) \
	$(plugin_glade_DATA)

DISTCLEANFILES = \
	$(plugin_DATA) \
	$(plugin_in_files)

SUBDIRS = GBF

