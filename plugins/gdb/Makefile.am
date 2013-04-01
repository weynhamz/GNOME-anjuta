# Plugin UI file
gdb_uidir = $(anjuta_ui_dir)
dist_gdb_ui_DATA =

# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
dist_anjuta_glade_DATA = anjuta-gdb.ui

anjuta_datadir = $(anjuta_data_dir)
dist_anjuta_data_DATA = gdb.init

plugin_pixmapsdir = $(anjuta_image_dir)
dist_plugin_pixmaps_DATA = \
	anjuta-gdb.plugin.png \
	pointer.png

plugin_in_files = anjuta-gdb.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
dist_anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(XML_CFLAGS)
	-DG_LOG_DOMAIN=\"libanjuta-gdb\"

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-gdb.la

libanjuta_gdb_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_gdb_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(XML_LIBS)

libanjuta_gdb_la_SOURCES = \
	gdbmi.c \
	gdbmi.h \
	debugger.c \
	debugger.h \
	plugin.c \
	plugin.h \
	utilities.c \
	utilities.h \
	preferences.c \
	preferences.h

noinst_PROGRAMS = gdbmi-test
gdbmi_test_SOURCES = gdbmi-test.c gdbmi.c gdbmi.h debugger.h
gdbmi_test_LDADD = $(GTK_LIBS) $(LIBANJUTA_LIBS) $(XML_LIBS) $(ANJUTA_LIBS)

# This last line do nothing but it is needed to avoid the error
# gdbmi.o created with both libtool and without
# Need AM_PROG_CC_C_0 in configure.in too
gdbmi_test_CFLAGS = $(AM_CFLAGS)

EXTRA_DIST = \
	$(plugin_in_files)

SUBDIRS = \
	images 


-include $(top_srcdir)/git.mk
