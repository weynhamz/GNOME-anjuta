[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/[+NameLower+]/glade
glade_DATA = [+NameLower+].glade

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(PACKAGE_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+NameLower+]

[+NameCLower+]_SOURCES = \
	main.cc

[+NameCLower+]_LDFLAGS = 

[+NameCLower+]_LDADD = $(PACKAGE_LIBS)

EXTRA_DIST = $(glade_DATA)
