# Plugin UI file
plugin_uidir = $(anjuta_ui_dir)
plugin_ui_DATA = 

# Plugin glade file
plugin_gladedir = $(anjuta_glade_dir)
plugin_glade_DATA = mk-project.ui

# Plugin icon file
plugin_pixmapsdir = $(anjuta_image_dir)
plugin_pixmaps_DATA = mk-project-plugin-48.png

# Plugin description file
plugin_in_files = mk-project.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugindir = $(anjuta_plugin_dir)
plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = 					\
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GIO_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"mk-project\"

plugin_LTLIBRARIES = \
	libmk-project.la

libmk_project_la_SOURCES = \
	plugin.c \
	plugin.h \
	mk-project.c \
	mk-project.h \
	mk-scanner.l \
	mk-parser.y \
	mk-scanner.h \
	mk-rule.c \
	mk-rule.h \
	mk-project-private.h

libmk_project_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libmk_project_la_LIBADD = \
	$(GIO_LIBS) \
	$(LIBANJUTA_LIBS)

AM_YFLAGS = -t -v -g -rall

mk-scanner.c: $(srcdir)/mk-scanner.l mk-parser.c
	$(LEXCOMPILE) -o $@ $<

mk-parser.c: $(srcdir)/mk-parser.y
	$(YACCCOMPILE) -o $@ $<

mk-scanner.h: mk-parser.c


EXTRA_DIST = \
	$(plugin_in_files) \
	$(plugin_DATA) \
	$(plugin_ui_DATA) \
	$(plugin_pixmaps_DATA) \
	$(plugin_glade_DATA) \
	mk-parser.h

DISTCLEANFILES = \
	$(plugin_DATA)

-include $(top_srcdir)/git.mk
