[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

uidir = $(datadir)/[+NameHLower+]/ui
ui_DATA = ../data/[+NameHLower+].ui

## The main script
bin_SCRIPTS = [+NameHLower+].py

## Directory where .class files will be installed
[+NameCLower+]dir = $(pythondir)/[+NameHLower+]

EXTRA_DIST = $(ui_DATA)

[+NameCLower+]_PYTHON = \
	[+NameHLower+].py

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/[+NameHLower+]
