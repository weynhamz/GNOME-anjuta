[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

jsdir = $(pkgdatadir)
js_DATA = \
	main.js

bin_SCRIPTs = main.js

EXTRA_DIST = $(js_DATA)

uninstall-local:
	-rm -r $(jsdir)

