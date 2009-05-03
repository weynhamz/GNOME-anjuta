SUBDIRS = images

# Plugin glade file
document_manager_gladedir = $(anjuta_glade_dir)
document_manager_glade_DATA = anjuta-document-manager.ui

# Plugin UI file
document_manager_uidir = $(anjuta_ui_dir)
document_manager_ui_DATA = anjuta-document-manager.xml

# Plugin file
plugin_in_files = anjuta-document-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

document_manager_plugindir = $(anjuta_plugin_dir)
document_manager_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-document-manager.la

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GDL_CFLAGS) \
	$(GNOME_PRINT_UI_CFLAGS) \
	$(GIO_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"libanjuta-document-manager\"

libanjuta_document_manager_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_document_manager_la_LIBADD = \
	$(GDL_LIBS) \
	$(GNOME_PRINT_UI_LIBS) \
	$(GIO_LIBS) \
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
	search-box.h \
	search-box.c \
	anjuta-bookmarks.h \
	anjuta-bookmarks.c
	
prefs_ui_files = anjuta-document-manager.ui
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(document_manager_pixmaps_DATA) \
	$(plugin_in_files) \
	$(document_manager_plugin_DATA) \
	$(document_manager_ui_DATA) \
	$(document_manager_glade_DATA)

-include $(top_srcdir)/git.mk
