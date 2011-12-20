[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" [+IF (=(get "HavePackage") "1")+]$([+NameCUpper+]_CFLAGS)[+ENDIF+]

AM_CFLAGS =\
	 -Wall\
	 -g

lib_LTLIBRARIES = lib[+NameHLower+].la


lib[+NameHLower+]_la_SOURCES = \
	lib.c

lib[+NameCLower+]_la_LDFLAGS = 

lib[+NameCLower+]_la_LIBADD = [+IF (=(get "HavePackage") "1")+]$([+NameCUpper+]_LIBS)[+ENDIF+]

include_HEADERS = \
	[+NameHLower+].h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lib[+NameHLower+]-[+Version+].pc

EXTRA_DIST = \
	lib[+NameHLower+]-[+Version+].pc.in
