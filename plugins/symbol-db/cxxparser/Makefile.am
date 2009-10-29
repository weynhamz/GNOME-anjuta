## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
  	-lc \
	-DDEBUG \
	-DCURR_DIR=\"`pwd`\"

AM_CFLAGS =\
	 -Wall\
	 -g

noinst_PROGRAMS = anjuta-cxxparser

anjuta_cxxparser_SOURCES = \
	main.c \
       ../readtags.c       \
        ../readtags.h       \
        ../symbol-db-engine-core.c       \
        ../symbol-db-engine-queries.c       \
        ../symbol-db-engine-iterator.c       \
        ../symbol-db-engine-utils.c \
        ../symbol-db-engine-iterator-node.c \
        ../symbol-db-engine-iterator-node.h \
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
		../libgda-extra/gda-data-model-concat.h \
		../libgda-extra/gda-data-model-concat.c \
		../symbol-db-engine-iterator.h \
		../symbol-db-engine-queries.h \
		../symbol-db-engine.h \
		../symbol-db-engine-utils.h \
		../symbol-db-engine-core.h
 
anjuta_cxxparser_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

anjuta_cxxparser_LDADD =   $(LIBANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

