## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 

AM_CFLAGS =\
	 -Wall \
	 $(GLIB_CFLAGS) \
	 $(LIBXML_CFLAGS)

bin_PROGRAMS = anjuta-tags

YACCFLAGS = -d -o ./js_parser/y.tab.c
YACCFILE = ./js_parser/Grammar.y
PARSER = bison

js_parser/y.tab.c : ${YACCFILE}
	${PARSER} ${YACCFLAGS} ${YACCFILE}

LEXER = flex
LEXFILE = ./js_parser/lex.l
LEXCCFLAGS = -Cr -o js_parser/lex.yy.c --yylineno --bison-bridge

js_parser/lex.yy.c : ${LEXFILE}
	${LEXER} ${LEXCCFLAGS} ${LEXFILE}

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
js_parser/y.tab.c	\
js_parser/lex.yy.c   \
js_parser/jsparse.c   \
js_parser/js-context.c   \
js_parser/js-node.c


anjuta_tags_LDFLAGS = \
		$(GLIB_LDFLAGS) \
		$(LIBXML_LDFLAGS)

anjuta_tags_LDADD = \
		$(GLIB_LIBS) \
		$(LIBXML_LIBS)

