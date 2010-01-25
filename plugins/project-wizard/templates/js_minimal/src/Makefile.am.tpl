[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

jsdir = $(pkgdatadir)
js_DATA = \
	main.js

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DTYPELIBDIR=\"$(TYPELIBDIR)\"

AM_CFLAGS =\
	 -Wall\
	 -g \
	 $(GJS_CFLAGS)\
	 -DPKGLIBDIR=\"$(pkglibdir)\" \
	 -DJSDIR=\"$(pkgdatadir)\"

bin_PROGRAMS = [+NameHLower+]

[+NameCLower+]_SOURCES = \
	main.c \
	debug.c debug.h

[+NameCLower+]_LDFLAGS = \
	$(GJS_LIBS) \
	-R $(FIREFOX_JS_LIBDIR) -rdynamic

[+NameCLower+]_LDADD = \
	$(GJS_LIBS)

EXTRA_DIST = $(js_DATA)

uninstall-local:
	-rm -r $(jsdir)

