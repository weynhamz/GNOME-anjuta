## Process this file with automake to produce Makefile.in

SUBDIRS = include

noinst_LIBRARIES = libscintilla.a

INCLUDES = -I$(srcdir)/include

CXXFLAGS = -DGTK -DSCI_LEXER -DUSE_XIM -W -Wall -g -O

libscintilla_a_SOURCES =\
	AutoComplete.cxx \
	CallTip.cxx \
	CellBuffer.cxx \
	ContractionState.cxx \
	Document.cxx \
	DocumentAccessor.cxx \
	Editor.cxx \
	Indicator.cxx \
	KeyMap.cxx \
	KeyWords.cxx \
	LexAda.cxx \
	LexAVE.cxx \
	LexCPP.cxx \
	LexConf.cxx \
	LexHTML.cxx \
	LexLua.cxx \
	LexOthers.cxx \
	LexPascal.cxx \
	LexPerl.cxx \
	LexPython.cxx \
	LexSQL.cxx \
	LexVB.cxx \
	LineMarker.cxx \
	PlatGTK.cxx \
	PropSet.cxx \
	RESearch.cxx \
	ScintillaBase.cxx \
	ScintillaGTK.cxx \
	Style.cxx \
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
	ViewStyle.h

EXTRA_DIST = PosRegExp.cxx
