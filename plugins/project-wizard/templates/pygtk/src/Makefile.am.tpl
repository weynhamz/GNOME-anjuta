[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

[+IF (=(get "HaveBuilderUI") "1")+]
uidir = $(pkgdatadir)/ui
ui_DATA = [+NameHLower+].ui
[+ENDIF+]

## The main script
bin_SCRIPTS = [+NameHLower+].py

## Directory where .class files will be installed
[+NameCLower+]dir = $(pythondir)/[+NameHLower+]


[+NameCLower+]_PYTHON = \
	[+NameHLower+].py

[+IF (=(get "HaveBuilderUI") "1")+]
EXTRA_DIST = $(ui_DATA)
[+ENDIF+]

# Remove ui directory on uninstall
uninstall-local:
[+IF (=(get "HaveBuilderUI") "1")+]
	-rm -r $(uidir)
[+ENDIF+]
	-rm -r $(pkgdatadir)
