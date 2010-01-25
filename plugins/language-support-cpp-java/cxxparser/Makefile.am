## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) 

noinst_LTLIBRARIES = libcxxparser.la

libcxxparser_la_SOURCES = \
        expression-lexer.cpp \
        expression-parser.cpp \
        expression-result.cpp \
        expression-result.h \
        flex-lexer-klass.cpp \
        flex-lexer-klass.h \
        flex-lexer-klass-tab.h \
        flex-lexer-plain.cpp \
        flex-lexer-plain-tab.h \
        variable-parser.cpp \
        variable-result.cpp \
        variable-result.h \
        scope-parser.cpp \
        function-result.cpp \
        function-result.h \
        function-parser.cpp \
        cpp-flex-tokenizer.cpp \
        cpp-flex-tokenizer.h \
        expression-parser.h \
        engine-parser.cpp \
        engine-parser-priv.h \
        engine-parser.h \
        scope-parser.h \
        variable-parser.h \
        function-parser.h
 
libcxxparser_la_LIBADD = $(LIBANJUTA_LIBS) \
	$(GTHREAD_LIBS)
libcxxparser_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)
