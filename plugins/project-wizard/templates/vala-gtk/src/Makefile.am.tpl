[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

[+IF (=(get "HaveBuilderUI") "1")+]
uidir = $(datadir)/[+NameHLower+]/ui
ui_DATA = [+NameHLower+].ui
[+ENDIF+]

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$([+NameCUpper+]_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+NameHLower+]

[+NameCLower+]_SOURCES = \
	[+NameHLower+].vala config.vapi

[+NameCLower+]_VALAFLAGS = [+IF (not (= (get "PackageModule2") ""))+] --pkg [+(string-substitute (get "PackageModule2") " " " --pkg ")+] [+ENDIF+] \
	--pkg gtk+-3.0

[+NameCLower+]_LDFLAGS = \
	-Wl,--export-dynamic

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

[+IF (=(get "HaveBuilderUI") "1")+]
EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/[+NameHLower+]
[+ENDIF+]
