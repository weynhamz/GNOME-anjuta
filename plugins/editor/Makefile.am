INCLUDES = \
	$(WARN_CFLAGS) \
	$(PCRE_CFLAGS)\
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-I$(top_builddir)/scintilla/include \
	-I$(top_builddir)/tagmanager/include \
	-I$(top_builddir)/pixmaps

lib_LTLIBRARIES=libanjuta-editor.la

libanjuta_editor_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_editor_la_LIBADD = \
	$(top_builddir)/libanjuta/libanjuta.la

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
	file.c \
	file.h \
	file_history.c \
	file_history.h \
	search-replace.h \
	search-replace.c \
	search-replace_backend.c \
	search-replace_backend.h \
	search_incremental.c \
	search_incremental.h \
	goto_line.c \
	goto_line.h
