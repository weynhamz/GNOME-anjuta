## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir) -I$(includedir)\
	$(GNOME_INCLUDEDIR) -DCCVIEW_DEBUG\
	-I/usr/local/gcc-3.0/include/g++-v3/ext\
	-I/usr/gcc-3.0/include/g++-v3/ext

noinst_LIBRARIES = libccview.a

libccview_a_SOURCES = \
	ccview_main.cc\
	cccmd.cc\
	cccproj.cc\
	ccview.cc\
	panes.cc\
	func_dialog.cc\
	gtk_help.cc\
	support.c

noinst_HEADERS = \
	ccview_main.h\
	ccview.h\
	cccmd.h\
	cregex.hh\
	ccview.h\
	cccproj.hh\
	gtk_help.h\
	func_dialog.h\
	tree_coll.h\
	hash_str.h\
	support.h

EXTRA_DIST = \
	ChangeLog\
	class.xpm\
	struct.xpm\
	ofolder.xpm\
	cfolder.xpm\
	file.xpm\
	private.xpm\
	protected.xpm\
	public.xpm\
	scanner.xpm
