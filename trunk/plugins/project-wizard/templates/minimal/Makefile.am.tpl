[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" [+IF (=(get "HavePackage") "1")+]$([+NameCUpper+]_CFLAGS)[+ENDIF+]

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+NameLower+]

[+NameCLower+]_SOURCES = \
	main.c

[+NameCLower+]_LDFLAGS = 

[+NameCLower+]_LDADD = [+IF (=(get "HavePackage") "1")+]$([+NameCUpper+]_LIBS)[+ENDIF+]

