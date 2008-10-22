[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

[+IF (=(get "HaveGlade") "1")+]
gladedir = $(datadir)/[+NameHLower+]/glade
glade_DATA = [+NameHLower+].glade
[+ENDIF+]

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
	callbacks.c \
	callbacks.h \
	main.c

[+NameCLower+]_LDFLAGS = \
	-Wl,--export-dynamic

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

EXTRA_DIST = $(glade_DATA)

[+IF (=(get "HaveGlade") "1")+]
# Remove glade directory on uninstall
uninstall-local:
	-rm -r $(gladedir)
	-rm -r $(datadir)/[+NameHLower+]
[+ENDIF+]
