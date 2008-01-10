[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

[+IF (=(get "HaveGlade") "1")+]
gladedir = $(datadir)/[+NameLower+]/glade
glade_DATA = [+NameLower+].glade
[+ENDIF+]

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$([+NameCUpper+]_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+NameLower+]

[+NameCLower+]_SOURCES = \
	callbacks.c \
	callbacks.h \
	main.c

[+NameCLower+]_LDFLAGS = 

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

EXTRA_DIST = $(glade_DATA)
