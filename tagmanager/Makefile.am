## Process this file with automake to produce Makefile.in
SUBDIRS = include
INCLUDES = \
	$(GLIB_CFLAGS) \
	-I$(srcdir)/include
#	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
#	-DGDK_PIXBUF_DEPRECATED \
#	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED

lib_LTLIBRARIES = libanjuta-ctags.la
libanjuta_ctags_la_SOURCES =\
	debug.h\
	general.h\
	keyword.h\
	options.h\
	parsers.h\
	sort.h\
	vstring.h\
	args.h\
	ctags.h\
	entry.h\
	get.h\
	main.h\
	parse.h\
	read.h\
	strlist.h\
	args.c\
	args.h\
	c.c\
	asm.c\
	asp.c\
	awk.c\
	beta.c\
	cobol.c\
	eiffel.c\
	fortran.c\
	lisp.c\
	lua.c\
	make.c\
	pascal.c\
	perl.c\
	php.c\
	python.c\
	regex.c\
	rexx.c\
	ruby.c\
	scheme.c\
	sh.c\
	slang.c\
	tcl.c\
	vim.c\
	yacc.c\
	ctags.c\
	debug.c\
	entry.c\
	get.c\
	keyword.c\
	options.c\
	parse.c\
	read.c\
	sort.c\
	strlist.c\
	vstring.c\
	tm_workspace.c\
	tm_work_object.c\
	tm_source_file.c\
	tm_project.c\
	tm_tag.c\
	tm_symbol.c\
	tm_file_entry.c\
	tm_tagmanager.c

libanjuta_ctags_la_LIBADD = $(GLIB_LIBS)
