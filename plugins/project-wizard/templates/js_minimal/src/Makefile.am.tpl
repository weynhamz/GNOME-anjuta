[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

[+IF (=(get "HaveBuilderUI") "1")+]
uidir = $(datadir)/[+NameHLower+]/ui
ui_DATA = [+NameHLower+].ui
[+ENDIF+]

jsdir = $(pkgdatadir)
js_DATA = \
	main.js

bin_SCRIPTs = main.js

EXTRA_DIST = $(js_DATA)

[+IF (=(get "HaveBuilderUI") "1")+]
EXTRA_DIST += $(ui_DATA)
[+ENDIF+]

uninstall-local:
	-rm -r $(jsdir)


