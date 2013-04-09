[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

[+IF (=(get "HaveBuilderUI") "1")+]
uidir = $(pkgdatadir)/ui
ui_DATA = [+NameHLower+].ui
[+ENDIF+]

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"[+IF (=(get "HavePackage") "1")+] \
	$([+NameCUpper+]_CFLAGS)[+ENDIF+]

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+NameHLower+]

[+NameCLower+]_SOURCES = \
	main.cc

[+NameCLower+]_LDFLAGS = 

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

[+IF (=(get "HaveWindowsSupport") "1")+]
if NATIVE_WIN32
[+NameCLower+]_LDFLAGS += -mwindows
endif
[+ENDIF+]

[+IF (=(get "HaveBuilderUI") "1")+]
EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)
[+ENDIF+]
