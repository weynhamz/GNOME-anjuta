## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 

AM_CFLAGS =\
	 -I$(top_srcdir)/plugins/symbol-db/anjuta-tags/js-parser \
	 $(WARN_CFLAGS) \
	 $(ANJUTA_CFLAGS) \
	 $(VALA_CFLAGS) \
	 $(XML_CFLAGS)

bin_PROGRAMS = anjuta-tags

YACCFLAGS = -d -o ./js-parser/js-parser-y-tab.c
YACCFILE = ./js-parser/Grammar.y
PARSER = bison

js-parser/js-parser-y-tab.c : ${YACCFILE}
	${PARSER} ${YACCFLAGS} ${YACCFILE}

LEXER = flex
LEXFILE = ./js-parser/lex.l
LEXCCFLAGS = -Cr -o js-parser/lex.yy.c --yylineno --bison-bridge

js-parser/lex.yy.c : ${LEXFILE}
	${LEXER} ${LEXCCFLAGS} ${LEXFILE}

ctags-visitor.c: ctags-visitor.vala
	$(VALAC) -g -h ctags-vala.h -C --pkg vala-0.10 $^

anjuta_tags_SOURCES = \
	main.c        \
	acconfig.h        \
	ant.c        \
	args.c        \
	args.h        \
	asm.c        \
	asp.c        \
	awk.c        \
	basic.c        \
	beta.c        \
	c.c        \
	cobol.c        \
	config.h        \
	ctags.h        \
	debug.c        \
	debug.h        \
	dosbatch.c        \
	e_amiga.h        \
	e_djgpp.h        \
	eiffel.c        \
	e_mac.h        \
	e_msoft.h        \
	entry.c        \
	entry.h        \
	e_os2.h        \
	e_qdos.h        \
	e_riscos.h        \
	erlang.c        \
	e_vms.h        \
	flex.c        \
	fortran.c        \
	general.h        \
	get.c        \
	get.h        \
	gir.c        \
	html.c        \
	jscript.c        \
	keyword.c        \
	keyword.h        \
	lisp.c        \
	lregex.c        \
	lua.c        \
	main.h        \
	make.c        \
	Makefile.am        \
	matlab.c        \
	ocaml.c        \
	options.c        \
	options.h        \
	parse.c        \
	parse.h        \
	parsers.h        \
	pascal.c        \
	perl.c        \
	php.c        \
	python.c        \
	read.c        \
	read.h        \
	rexx.c        \
	routines.c        \
	routines.h        \
	ruby.c        \
	scheme.c        \
	sh.c        \
	slang.c        \
	sml.c        \
	sort.c        \
	sort.h        \
	sql.c        \
	strlist.c        \
	strlist.h        \
	tcl.c        \
	tex.c        \
	verilog.c        \
	vhdl.c        \
	vim.c        \
	vstring.c        \
	vstring.h        \
	yacc.c	\
	js-parser/js-parser-y-tab.c \
	js-parser/js-parser-y-tab.h	\
	js-parser/lex.yy.c   \
	js-parser/lex.yy.h	\
	js-parser/jsparse.c   \
	js-parser/jsparse.h	\
	js-parser/js-context.c   \
	js-parser/js-context.h	\
	js-parser/js-node.c	\
	js-parser/js-node.h	\
	js-parser/jstypes.h     \
	ctags-utils.c     \
	ctags-utils.h

anjuta_tags_LDADD = \
		$(ANJUTA_LIBS) \
		$(VALA_LIBS) \
		$(XML_LIBS)

EXTRA_DIST = ${YACCFILE} ${LEXFILE} ctags-visitor.vala ctags-visitor.c ctags-vala.h

if ENABLE_VALA
anjuta_tags_SOURCES += vala.c ctags-visitor.c
AM_CPPFLAGS += -DENABLE_VALA
BUILT_SOURCES = ctags-visitor.c
DISTCLEANFILES = ctags-visitor.c ctags-vala.h
endif

