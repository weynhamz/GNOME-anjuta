[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src

[+(string-downcase (get "Name"))+]docdir = ${prefix}/doc/[+Name+]
[+(string-downcase (get "Name"))+]doc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO

EXTRA_DIST = $([+(string-downcase (get "Name"))+]doc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

