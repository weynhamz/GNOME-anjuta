[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

uidir = $(datadir)/[+NameHLower+]/ui
ui_DATA = [+NameHLower+].ui

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
	main.c \
	[+NameLower+].h \
	[+NameLower+].c	

[+NameCLower+]_LDFLAGS = \
	-Wl,--export-dynamic

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/[+NameHLower+]
