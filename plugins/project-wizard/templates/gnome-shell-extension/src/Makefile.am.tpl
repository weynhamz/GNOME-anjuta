[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

jsdir = $(prefix)/[+UUID+]
js_DATA = \
	metadata.json \
	stylesheet.css

js_SCRIPTS = extension.js

EXTRA_DIST = $(js_DATA)

uninstall-local:
	-rm -r $(jsdir)


