CFLAGS=-O0 -g

INCLUDES= \
	$(WARN_CFLAGS) \
	$(PCRE_CFLAGS)\
	$(DEPRECATED_FLAGS)\
	$(LIBANJUTA_CFLAGS)

lib_LTLIBRARIES = libxmltemplate.la

libxmltemplate_la_LIADD = $(LIBANJUTA_LIBS)
libxmltemplate_la_LIBADD = \
	$(top_builddir)/libanjuta/libanjuta.la

libxmltemplate_la_SOURCES = \
	xmltemplate.c\
	xmltemplate.h\
	xmlstrings.c\
	xmlstrings.h\
	text-template.c\
	text-template.h\
	autotools.c\
	autotools.h\
	lang-template.c\
	lang-template.h\
	lib-template.c\
	lib-template.h\
	prj-template.c\
	prj-temlate.h

	
bin_PROGRAMS = template

template_SOURCES = template.c prj-template.c autotools.c \
	xmlstrings.c text-template.c lib-template.c lang-template.c
template_LDADD = \
	$(ANJUTA_LIBS) -lutil\
	$(PCRE_LIBS)\
	$(top_builddir)/scintilla/libscintilla.a\
	$(top_builddir)/tagmanager/libctags.a\
	$(top_builddir)/widgets/libwidgets.a\
	$(top_builddir)/gdl/libgdl.a\
	$(top_builddir)/printing/libanjutaprint.a\
	$(top_builddir)/libegg/treeviewutils/libegg-treeview.a\
	$(top_builddir)/libegg/toolbar/libegg-toolbar.a\
	$(top_builddir)/libegg/menu/libegg-menu.a\
	$(top_builddir)/libegg/util/libeggutil.a\
	$(LEXER_OBJS)
