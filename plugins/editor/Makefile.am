
SUBDIRS = scintilla

if HAVE_PLUGIN_SCINTILLA

# Plugin glade file
editor_gladedir = $(anjuta_glade_dir)
editor_glade_DATA = editor.glade

# Plugin Icon file
editor_pixmapsdir = $(anjuta_image_dir)
editor_pixmaps_DATA = anjuta-editor.png

plugin_in_files = anjuta-editor.plugin.in
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
	-I$(srcdir)/scintilla/include \
	-I$(top_srcdir)/tagmanager/include

libanjuta_editor_la_LIBADD = \
	$(PCRE_LIBS) \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS) \
	scintilla/libanjuta-scintilla.la \
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
	text-editor-iterable.c \
	text-editor-iterable.h \
	lexer.h \
	plugin.c \
	plugin.h \
	style-editor.c \
	style-editor.h \
	print.c \
	print.h \
	bookmark.xpm \
	breakpoint-disabled.xpm \
	breakpoint-enabled.xpm \
	program-counter.xpm \
	linemarker.xpm

endif

EXTRA_DIST = \
	$(editor_pixmaps_DATA) \
	$(plugin_in_files) \
	$(editor_plugin_DATA) \
	$(editor_ui_DATA) \
	$(editor_glade_DATA)
