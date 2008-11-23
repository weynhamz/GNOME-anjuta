## Process this file with automake to produce Makefile.in

SUBDIRS = include

noinst_LTLIBRARIES = libanjuta-scintilla.la

AM_CPPFLAGS = \
	$(GTK_CFLAGS) \
	-I$(srcdir)/include \
  -DGTK -DSCI_LEXER -DUSE_XIM
AM_CXXFLAGS = -DGTK -DSCI_LEXER -DUSE_XIM

include $(srcdir)/lexers.make

libanjuta_scintilla_la_SOURCES =\
	AutoComplete.cxx \
	CallTip.cxx \
	CellBuffer.cxx \
	ContractionState.cxx \
	Converter.h \
	CharClassify.h \
	CharClassify.cxx \
	ExternalLexer.cxx \
	ExternalLexer.h \
	Document.cxx \
	DocumentAccessor.cxx \
	Editor.cxx \
	Indicator.cxx \
	KeyMap.cxx \
	KeyWords.cxx \
	LineMarker.cxx \
	PlatGTK.cxx \
	PropSet.cxx \
	RESearch.cxx \
	ScintillaBase.cxx \
	ScintillaGTK.cxx \
	Style.cxx \
	StyleContext.cxx\
	UniConversion.cxx \
	ViewStyle.cxx \
	WindowAccessor.cxx \
	AutoComplete.h \
	CallTip.h \
	CellBuffer.h \
	ContractionState.h \
	Document.h \
	DocumentAccessor.h \
	Editor.h \
	Indicator.h \
	KeyMap.h \
	LineMarker.h \
	RESearch.h \
	SVector.h \
	ScintillaBase.h \
	Style.h \
	UniConversion.h \
	ViewStyle.h\
	StyleContext.h\
	XPM.h\
	XPM.cxx\
	Decoration.h \
	Decoration.cxx \
	PositionCache.h \
	PositionCache.cxx \
	RunStyles.cxx \
	RunStyles.h \
	SplitVector.h \
	Partitioning.h \
	CharacterSet.h \
	scintilla-marshal.h\
	scintilla-marshal.c\
	$(LEXER_SRCS)

libanjuta_scintilla_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

noinst_PROGRAMS = test-scintilla

test_scintilla_CXXFLAGS = -DGTK
test_scintilla_SOURCES = test-scintilla.cxx
test_scintilla_LDADD = libanjuta-scintilla.la

update-scintilla:
	cp $(srcdir)/scintilla/gtk/*.cxx $(srcdir)/
	cp $(srcdir)/scintilla/gtk/*.h $(srcdir)/
	cp $(srcdir)/scintilla/src/*.cxx $(srcdir)/
	cp $(srcdir)/scintilla/src/*.h $(srcdir)/
	cp $(srcdir)/scintilla/include/*.h $(srcdir)/include/
	cp $(srcdir)/scintilla/include/*.py $(srcdir)/include/
	cp $(srcdir)/scintilla/include/*.iface $(srcdir)/include/
	echo "## Lexers make file" > lexers.make
	echo 'LEXER_OBJS = \' >> lexers.make;
	echo -n '	$$(top_srcdir)/plugins/editor/scintilla/StyleContext.o' >> lexers.make
	for lex in Lex*.cxx; do \
		lex=`echo $$lex | sed -e "s/cxx$$/o/"`; \
		echo '\' >> lexers.make; \
		echo -n '	$$(top_srcdir)/plugins/editor/scintilla/' >> lexers.make; \
		echo -n "$$lex" >> lexers.make; \
	done
	echo "" >> lexers.make
	echo "" >> lexers.make
	echo -n 'LEXER_SRCS = ' >> lexers.make
	for lex in Lex*.cxx; do \
		echo '\' >> lexers.make; \
		echo -n "	$$lex" >> lexers.make; \
	done
	echo "" >> lexers.make
	echo "Patching files ..."
	for patchfile in patches/*.diff; do \
		patch -p1 < $$patchfile; \
	done
	echo "Updating properties file..."
	./update-properties.pl $(srcdir)/../properties/styles.properties $(srcdir)/scite/src

marshallers: scintilla-marshal.list
	glib-genmarshal --prefix scintilla_marshal scintilla-marshal.list --header > scintilla-marshal.h
	glib-genmarshal --prefix scintilla_marshal scintilla-marshal.list --body > scintilla-marshal.c 

EXTRA_DIST=lexers.make
