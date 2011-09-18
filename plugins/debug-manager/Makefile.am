# Plugin glade file
anjuta_gladedir = $(anjuta_glade_dir)
anjuta_glade_DATA = anjuta-debug-manager.ui

anjuta_uidir = $(anjuta_ui_dir)
anjuta_ui_DATA =  anjuta-debug-manager.xml

schema_in_files = org.gnome.anjuta.debug-manager.gschema.xml.in
gsettings_SCHEMAS = $(schema_in_files:.gschema.xml.in=.gschema.xml)

.PRECIOUS: $(gsettings_SCHEMAS)

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

plugin_in_files = anjuta-debug-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(anjuta_plugin_dir)
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS= \
	$(WARN_CFLAGS) \
	$(GIO_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-debug-manager.la

libanjuta_debug_manager_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_debug_manager_la_LIBADD = \
	$(GIO_LIBS) \
	$(LIBANJUTA_LIBS)

# Marshallers
BUILT_SOURCES = 			\
	anjuta-marshal.c			\
	anjuta-marshal.h              

anjuta-marshal.h: anjuta-marshal.list
	$(AM_V_GEN)glib-genmarshal $< --header --prefix=anjuta_marshal > $@

anjuta-marshal.c: anjuta-marshal.list
	$(AM_V_GEN)echo "#include \"anjuta-marshal.h\"" > $@ && \
	glib-genmarshal $< --body --prefix=anjuta_marshal >> $@

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
	queue.c \
	queue.h \
	data_view.c \
	data_view.h \
	chunk_view.c \
	chunk_view.h \
	data_buffer.c \
	data_buffer.h \
	disassemble.c \
	disassemble.h \
	sparse_buffer.c \
	sparse_buffer.h \
	sparse_view.c \
	sparse_view.h \
	threads.c \
	threads.h \
	command.c \
	command.h \
	variable.c \
	variable.h

EXTRA_DIST = \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(schema_in_files) \
	$(anjuta_ui_DATA) \
	$(anjuta_glade_DATA) \
	$(schema_in_files) \
	anjuta-marshal.list

DISTCLEANFILES = \
	$(anjuta_plugin_DATA) \
	$(gsettings_SCHEMAS)

SUBDIRS = \
	images 


-include $(top_srcdir)/git.mk
