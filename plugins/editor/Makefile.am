# Plugin glade file
anjuta_gladedir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
anjuta_glade_DATA =  anjuta-document-manager.glade

# Plugin UI file
anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta-document-manager.ui

# Plugin Icon file
plugin_icondir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_icon_DATA = anjuta-document-manager.png

# Plugin Icon file
plugin_in_files = anjuta-document-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; \
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u \ \
		-c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
plugin_LTLIBRARIES = libanjuta-editor.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(PCRE_CFLAGS)\
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-I$(top_builddir)/scintilla/include \
	-I$(top_builddir)/tagmanager/include \
	-I$(top_builddir)/pixmaps

libanjuta_editor_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(top_builddir)/libanjuta/libanjuta.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la \
	$(top_builddir)/scintilla/libanjuta-scintilla.la \
        $(top_builddir)/tagmanager/libanjuta-ctags.la

libanjuta_editor_la_SOURCES= \
	aneditor.cxx \
	aneditor.h \
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
	file.c \
	file.h \
	file_history.c \
	file_history.h \
	search-replace.h \
	search-replace.c \
	search-replace_backend.c \
	search-replace_backend.h \
	search_preferences.c \
	search_preferences.h \
	search_incremental.c \
	search_incremental.h \
	goto_line.c \
	goto_line.h \
	print.c \
	print.h

EXTRA_DIST = \
	$(plugin_icon_DATA) \
        $(plugin_in_files) \
        $(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA)
