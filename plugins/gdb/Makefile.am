# Plugin UI file
gdb_uidir = $(anjuta_ui_dir)
gdb_ui_DATA =

# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta-gdb.glade

anjuta_datadir = $(anjuta_data_dir)
anjuta_data_DATA = gdb.init

plugin_pixmapsdir = $(anjuta_image_dir)
plugin_pixmaps_DATA = \
	anjuta-gdb.plugin.png \
	pointer.png

plugin_in_files = anjuta-gdb.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"libanjuta-gdb\"

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-gdb.la

libanjuta_gdb_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_gdb_la_LIBADD = \
	$(GTK_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_gdb_la_SOURCES = \
	gdbmi.c \
	gdbmi.h \
	debugger.c \
	debugger.h \
	plugin.c \
	plugin.h \
	utilities.c \
	utilities.h

noinst_PROGRAMS = gdbmi-test
gdbmi_test_SOURCES = gdbmi-test.c
gdbmi_test_LDADD = libanjuta-gdb.la $(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(gdb_ui_DATA) \
	$(anjuta_glade_DATA) \
	$(anjuta_data_DATA)

SUBDIRS = \
	images 

