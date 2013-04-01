# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
dist_anjuta_glade_DATA = anjuta-run-program.ui

# Plugin UI file
anjuta_uidir = $(anjuta_ui_dir)
dist_anjuta_ui_DATA = anjuta-run-program.xml

# Plugin Icon file
anjuta_pixmapsdir = $(anjuta_image_dir)
dist_anjuta_pixmaps_DATA = \
        anjuta-run-program-48.png

gsettings_in_file = org.gnome.anjuta.plugins.run.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

# Plugin description file
plugin_in_files = anjuta-run-program.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
dist_anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-run-program.la

AM_CPPFLAGS= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_run_program_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_run_program_la_LIBADD = \
	$(LIBANJUTA_LIBS)

libanjuta_run_program_la_SOURCES = \
	plugin.c \
	plugin.h \
	parameters.c \
	parameters.h \
	execute.c \
	execute.h \
	utils.c \
	utils.h

EXTRA_DIST = \
	$(plugin_in_files) \
        $(gsettings_in_file)

CLEANFILES = \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
