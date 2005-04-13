# Plugin glade file
editor_gladedir = $(anjuta_glade_dir)
editor_glade_DATA = anjuta-document-manager.glade

# Plugin UI file
editor_uidir = $(anjuta_ui_dir)
editor_ui_DATA = anjuta-document-manager.ui

# Plugin pixmaps
editor_pixmapsdir = $(anjuta_image_dir)
editor_pixmaps_DATA = \
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

editor_plugindir = $(anjuta_plugin_dir)
editor_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-editor.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PCRE_CFLAGS) \
	$(GNOME_PRINT_UI_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-I$(top_builddir)/scintilla/include \
	-I$(top_builddir)/tagmanager/include

libanjuta_editor_la_LIBADD = \
	$(PCRE_LIBS) \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(top_builddir)/scintilla/libanjuta-scintilla.la \
	$(top_builddir)/tagmanager/libanjuta-ctags.la

libanjuta_editor_la_SOURCES= \
	properties.cxx \
	properties.h \
	properties_cxx.h \
	aneditor.h \
	aneditor-priv.h \
	aneditor.cxx \
	aneditor-autocomplete.cxx \
	aneditor-calltip.cxx \
	aneditor-indent.cxx \
	text_editor_prefs.c \
	text_editor_prefs.h \
	text_editor.c \
	text_editor.h \
	text_editor_menu.c \
	text_editor_menu.h \
	text_editor_cbs.c \
	text_editor_cbs.h \
	anjuta-docman.c \
	anjuta-docman.h \
	lexer.h \
	plugin.c \
	action-callbacks.c\
	action-callbacks.h\
	file_history.c \
	file_history.h \
	search-replace.h \
	search-replace.c \
	search-replace_backend.c \
	search-replace_backend.h \
	search_preferences.c \
	search_preferences.h \
	goto_line.c \
	goto_line.h \
	print.c \
	print.h \
	style-editor.c \
	style-editor.h \
	editor-tooltips.c \
	editor-tooltips.h 

EXTRA_DIST = \
	$(editor_pixmaps_DATA) \
	$(plugin_in_files) \
	$(editor_plugin_DATA) \
	$(editor_ui_DATA) \
	$(editor_glade_DATA)
