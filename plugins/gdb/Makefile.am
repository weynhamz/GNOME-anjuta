# Plugin UI file
gdb_uidir = $(anjuta_ui_dir)
gdb_ui_DATA = anjuta-gdb-plugin.ui

# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta-gdb.glade

plugin_pixmapsdir = $(anjuta_image_dir)
plugin_pixmaps_DATA = \
	anjuta-gdb.plugin.png \
	pointer.png

plugin_in_files = anjuta-gdb.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-gdb.la

libanjuta_gdb_la_LIBADD = $(LIBANJUTA_LIBS)

libanjuta_gdb_la_SOURCES = \
	gdbmi.c \
	gdbmi.h \
	debugger.c \
	debugger.h \
	utilities.c \
	utilities.h \
	plugin.c \
	plugin.h \
	actions.c \
	actions.h \
	info.c \
	info.h \
	breakpoints.c \
	breakpoints.h \
	watch.c \
	watch_cbs.c \
	watch_cbs.h \
	watch_gui.c \
	watch_gui.h \
	watch.h \
	locals.c \
	locals.h \
	debug_tree.c \
	debug_tree.h \
	memory.c \
	memory.h \
	stack_trace.c \
	stack_trace.h \
	registers.c \
	registers.h \
	sharedlib.c \
	sharedlib.h \
	signals.c \
	signals.h

noinst_PROGRAMS = gdbmi-test
gdbmi_test_SOURCES = gdbmi-test.c
gdbmi_test_LDADD = libanjuta-gdb.la

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(gdb_ui_DATA) \
	$(anjuta_glade_DATA)

SUBDIRS = \
	images 

