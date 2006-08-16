## Process this file with automake to produce Makefile.in
SUBDIRS = include
INCLUDES = \
	$(GLIB_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	-I$(srcdir)/include

lib_LTLIBRARIES = libanjuta-ctags.la
libanjuta_ctags_la_SOURCES =\
        args.c\
        args.h\
        acconfig.h\
        asm.c\
        asp.c\
        awk.c\
        beta.c\
        c.c\
        cobol.c\
        ctags.c\
        ctags.h\
        debug.c\
        debug.h\
        eiffel.c\
        erlang.c\
        entry.c\
        entry.h\
        fortran.c\
        general.h\
        get.c\
        get.h\
        html.c\
        jscript.c\
        keyword.c\
        keyword.h\
        lisp.c\
        lregex.c\
        lua.c\
        main.h\
        make.c\
        options.c\
        options.h\
        parse.c\
        parse.h\
        parsers.h\
        pascal.c\
        perl.c\
        php.c\
        python.c\
        read.c\
        read.h\
        readtags.c\
        readtags.h\
        rexx.c\
        routines.c\
        routines.h\
        ruby.c\
        scheme.c\
        sh.c\
        slang.c\
        sml.c\
        sort.c\
        sort.h\
        sql.c\
        strlist.c\
        strlist.h\
        tcl.c\
        tm_file_entry.c\
        tm_project.c\
        tm_source_file.c\
        tm_symbol.c\
        tm_tag.c\
        tm_tagmanager.c\
        tm_work_object.c\
        tm_workspace.c\
        verilog.c\
        vim.c\
        vstring.c\
        vstring.h\
        yacc.c


libanjuta_ctags_la_LIBADD = $(GLIB_LIBS)

bin_PROGRAMS = \
	test_tm_buffer 
	

test_tm_buffer_SOURCES = \
	test_tm_buffer.c\
	entry.h                        \
	general.h                        \
	parse.h                        \
	read.h                        \
	include/tm_source_file.h                        \
	include/tm_tag.h                        \
	include/tm_work_object.h                        

test_tm_buffer_LDADD = $(GLIB_LIBS)\
	libanjuta-ctags.la

