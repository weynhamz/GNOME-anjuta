[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

INCLUDES = -DPACKAGE_DATA_DIR=\"$(datadir)\" [+IF (=(get "HavePackage") "1")+]$(PACKAGE_CFLAGS)[+ENDIF+]

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+(string-downcase (get "Name"))+]

[+(string->c-name! (string-downcase (get "Name")))+]_SOURCES = \
	main.c

[+(string->c-name! (string-downcase (get "Name")))+]_LDFLAGS = 

[+(string->c-name! (string-downcase (get "Name")))+]_LDADD = [+IF (=(get "HavePackage") "1")+]$(PACKAGE_LIBS)[+ENDIF+]

