# Plugin glade file
anjuta_gladedir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
anjuta_glade_DATA = anjuta-debug-manager.glade

plugin_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_pixmaps_DATA = \
	anjuta-debug-manager.plugin.png

anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta-debug-manager.ui

plugin_in_files = anjuta-debug-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
plugin_LTLIBRARIES = libanjuta-debug-manager.la

libanjuta_debug_manager_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_debug_manager_la_LIBADD = \
	$(top_builddir)/libanjuta/libanjuta.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

libanjuta_debug_manager_la_SOURCES = \
	plugin.c \
	plugin.h \
	attach_process.c \
	attach_process.h \
	utilities.c \
	utilities.h

#	breakpoints.c \
#	breakpoints.h \
#	debugger.c \
#	debugger.h \
#	debug_tree.c \
#	debug_tree.h \
#	memory.c \
#	memory.h \
#	plugin.c \
#	plugin.h \
#	registers.c \
#	registers_cbs.c \
#	registers_cbs.h \
#	registers_gui.c \
#	registers.h \
#	sharedlib.c \
#	sharedlib_cbs.c \
#	sharedlib_cbs.h \
#	sharedlib_gui.c \
#	sharedlib.h \
#	signals.c \
#	signals_cbs.c \
#	signals_cbs.h \
#	signals_gui.c \
#	signals.h \
#	stack_trace.c \
#	stack_trace.h \
#	watch.c \
#	watch_cbs.c \
#	watch_cbs.h \
#	watch_gui.c \
#	watch_gui.h \
#	watch.h

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA)
