# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta-run-program.ui

# Plugin UI file
anjuta_uidir = $(anjuta_ui_dir)
anjuta_ui_DATA =  anjuta-run-program.xml

# Plugin Icon file
anjuta_pixmapsdir = $(anjuta_image_dir)
anjuta_pixmaps_DATA = \
        anjuta-run-program-48.png

schema_in_files = org.gnome.anjuta.run.gschema.xml.in
gsettings_SCHEMAS = $(schema_in_files:.gschema.xml.in=.gschema.xml)

.PRECIOUS: $(gsettings_SCHEMAS)

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

# Plugin description file
plugin_in_files = anjuta-run-program.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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
	$(anjuta_pixmaps_DATA) \
	$(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA) \
	$(anjuta_glade_DATA) \
	$(schema_in_files)

DISTCLEANFILES = \
	$(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
