SUBDIRS = images

# Plugin glade file
document_manager_gladedir = $(anjuta_glade_dir)
document_manager_glade_DATA = anjuta-document-manager.glade

# Plugin UI file
document_manager_uidir = $(anjuta_ui_dir)
document_manager_ui_DATA = anjuta-document-manager.ui

# Plugin file
plugin_in_files = anjuta-document-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

document_manager_plugindir = $(anjuta_plugin_dir)
document_manager_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-document-manager.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PCRE_CFLAGS) \
	$(GDL_CFLAGS) \
	$(GNOME_PRINT_UI_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

libanjuta_document_manager_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_document_manager_la_LIBADD = \
	$(GDL_LIBS) \
	$(PCRE_LIBS) \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(top_builddir)/libegg/libanjuta-egg.la \
	$(LIBANJUTA_LIBS)

libanjuta_document_manager_la_SOURCES= \
	plugin.c \
	plugin.h \
	anjuta-docman.c \
	anjuta-docman.h \
	action-callbacks.c\
	action-callbacks.h\
	file_history.c \
	file_history.h \
	editor-tooltips.c \
	editor-tooltips.h \
	search-box.h \
	search-box.c
	
prefs_glade_files = anjuta-document-manager.glade
include $(top_srcdir)/scripts/build-schemas.mk

libanjuta_document_manager_la_DEPENDENCIES = \
	build-schema-files 

EXTRA_DIST = \
	$(document_manager_pixmaps_DATA) \
	$(plugin_in_files) \
	$(document_manager_plugin_DATA) \
	$(document_manager_ui_DATA) \
	$(document_manager_glade_DATA)
