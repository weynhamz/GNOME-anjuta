# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta-debug-manager.glade

plugin_pixmapsdir = $(anjuta_image_dir)
plugin_pixmaps_DATA = \
	anjuta-debug-manager.plugin.png

anjuta_uidir = $(anjuta_ui_dir)
anjuta_ui_DATA =  anjuta-debug-manager.ui

plugin_in_files = anjuta-debug-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-debug-manager.la

libanjuta_debug_manager_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_debug_manager_la_LIBADD = \
	$(top_builddir)/libanjuta/libanjuta.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

# Marshallers
BUILT_SOURCES = 			\
	anjuta-marshal.c			\
	anjuta-marshal.h              

anjuta-marshal.h: anjuta-marshal.list
	glib-genmarshal anjuta-marshal.list --header --prefix=anjuta_marshal > $@

anjuta-marshal.c: anjuta-marshal.list
	echo "#include \"anjuta-marshal.h\"" > $@ && \
	glib-genmarshal anjuta-marshal.list --body --prefix=anjuta_marshal >> $@

libanjuta_debug_manager_la_SOURCES = \
	$(BUILT_SOURCES) \
	plugin.c \
	plugin.h \
	start.c \
	start.h \
	utilities.c \
	utilities.h \
	breakpoints.c \
	breakpoints.h \
	watch.c \
	watch.h \
	debug_tree.c \
	debug_tree.h \
	locals.c \
	locals.h \
	stack_trace.c \
	stack_trace.h \
	signals.c \
	signals.h \
	sharedlib.c \
	sharedlib.h \
	registers.c \
	registers.h \
	memory.c \
	memory.h \
	info.c \
	info.h \
	debugger.c \
	debugger.h \
	data_view.c \
	data_view.h \
	chunk_view.c \
	chunk_view.h \
	data_buffer.c \
	data_buffer.h

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA) \
	$(anjuta_glade_DATA)

SUBDIRS = \
	images 

