[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src [+IF (=(get "HaveI18n") "1") +]po[+ENDIF+]

ACLOCAL_AMFLAGS = -I m4

[+NameCLower+]docdir = ${prefix}/doc/[+NameHLower+]
[+NameCLower+]doc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

[+IF (=(get "HaveI18n") "1") +]
INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = $([+NameCLower+]doc_DATA) \
	$(INTLTOOL_FILES)

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache
[+ELSE+]
EXTRA_DIST = $([+NameCLower+]doc_DATA)
[+ENDIF+]

