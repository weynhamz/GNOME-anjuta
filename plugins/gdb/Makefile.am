# Plugin glade file
anjuta_gladedir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
anjuta_glade_DATA = anjuta-gdb.glade

plugin_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_pixmaps_DATA = \
	anjuta-gdb.plugin.png

plugin_in_files = anjuta-gdb.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
plugin_LTLIBRARIES = libanjuta-gdb.la

libanjuta_gdb_la_LIBADD = $(LIBANJUTA_LIBS)

libanjuta_gdb_la_SOURCES = \
	breakpoints.c \
	breakpoints.h \
	debugger.c \
	debugger.h \
	debug_tree.c \
	debug_tree.h \
	memory.c \
	memory.h \
	plugin.c \
	plugin.h \
	registers.c \
	registers_cbs.c \
	registers_cbs.h \
	registers_gui.c \
	registers.h \
	sharedlib.c \
	sharedlib_cbs.c \
	sharedlib_cbs.h \
	sharedlib_gui.c \
	sharedlib.h \
	signals.c \
	signals_cbs.c \
	signals_cbs.h \
	signals_gui.c \
	signals.h \
	stack_trace.c \
	stack_trace.h \
	utilities.c \
	utilities.h \
	watch.c \
	watch_cbs.c \
	watch_cbs.h \
	watch_gui.c \
	watch_gui.h \
	watch.h

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA)

