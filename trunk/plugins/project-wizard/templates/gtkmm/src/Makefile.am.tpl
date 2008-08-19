[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/[+NameHLower+]/glade
glade_DATA = [+NameHLower+].glade

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$([+NameCUpper+]_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+NameHLower+]

[+NameCLower+]_SOURCES = \
	main.cc

[+NameCLower+]_LDFLAGS = 

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

EXTRA_DIST = $(glade_DATA)
