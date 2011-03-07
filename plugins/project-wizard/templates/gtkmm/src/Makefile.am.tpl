[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

[+IF (=(get "HaveBuilderUI") "1")+]
uidir = $(datadir)/[+NameHLower+]/ui
ui_DATA = [+NameHLower+].ui
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
	main.cc

[+NameCLower+]_LDFLAGS = 

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

[+IF (=(get "HaveBuilderUI") "1")+]
EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/[+NameHLower+]
[+ENDIF+]
