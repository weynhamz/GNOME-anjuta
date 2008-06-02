if HAVE_PLUGIN_SCINTILLA

SUBDIRS = scintilla properties

# Plugin glade file
editor_gladedir = $(anjuta_glade_dir)
editor_glade_DATA = anjuta-editor-scintilla.glade

# Plugin Icon file
editor_pixmapsdir = $(anjuta_image_dir)
editor_pixmaps_DATA = \
	anjuta-editor-scintilla-plugin.svg \
	anjuta-editor-scintilla-plugin-48.png

plugin_in_files = anjuta-editor.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

editor_plugindir = $(anjuta_plugin_dir)
editor_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-editor.la

AM_CPPFLAGS = \
	$(GNOME_PRINT_UI_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SCINTILLA_CFLAGS) \
	-I$(srcdir)/scintilla/include \
	-I$(top_srcdir)/tagmanager/include

libanjuta_editor_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_editor_la_LIBADD = \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_SCINTILLA_LIBS) \
	$(GLADE_LIBS) \
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
	anjuta-bookmark-16.xpm \
	anjuta-breakpoint-disabled-16.xpm \
	anjuta-breakpoint-enabled-16.xpm \
	anjuta-pcmark-16.xpm \
	anjuta-linemark-16.xpm

prefs_glade_files = anjuta-editor-scintilla.glade
include $(top_srcdir)/scripts/build-schemas.mk

endif

EXTRA_DIST = \
	$(editor_pixmaps_DATA) \
	$(plugin_in_files) \
	$(editor_plugin_DATA) \
	$(editor_ui_DATA) \
	$(editor_glade_DATA)
