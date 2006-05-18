# Plugin glade file
document_manager_gladedir = $(anjuta_glade_dir)
document_manager_glade_DATA = anjuta-document-manager.glade

# Plugin UI file
document_manager_uidir = $(anjuta_ui_dir)
document_manager_ui_DATA = anjuta-document-manager.ui

# Test file for anjuta indent
document_manager_datadir = $(anjuta_data_dir)
document_manager_data_DATA = indent_test.c

# Plugin pixmaps
document_manager_pixmapsdir = $(anjuta_image_dir)
document_manager_pixmaps_DATA = \
	anjuta-document-manager.png \
	preferences-editor.png \
	undock.png \
	fold_toggle.xpm \
	fold_open.xpm \
	fold_close.xpm \
	indent.xpm \
	indent_set.xpm \
	indent_inc.xpm \
	indent_dcr.xpm \
	indent_auto.xpm \
	autocomplete-16.png \
	autocomplete.png \
	block-end-16.png \
	block-end.png \
	block-start-16.png \
	block-start.png \
	block_select.xpm \
	bookmark-first-16.png \
	bookmark-first.png \
	bookmark-prev-16.png \
	bookmark-prev.png \
	bookmark-next-16.png \
	bookmark-next.png \
	bookmark-last-16.png \
	bookmark-last.png \
	bookmark_toggle.xpm

# Plugin Icon file
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

libanjuta_document_manager_la_LIBADD = \
	$(GDL_LIBS) \
	$(PCRE_LIBS) \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_VFS_LIBS) \
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
	indent-dialog.c \
	indent-dialog.h \
	indent-util.c \
	indent-util.h \
	goto_line.c \
	goto_line.h

EXTRA_DIST = \
	$(document_manager_pixmaps_DATA) \
	$(plugin_in_files) \
	$(document_manager_plugin_DATA) \
	$(document_manager_data_DATA) \
	$(document_manager_ui_DATA) \
	$(document_manager_glade_DATA)
