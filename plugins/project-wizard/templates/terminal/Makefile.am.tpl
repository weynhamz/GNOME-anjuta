[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src [+IF (=(get "HaveI18n") "1") +]po[+ENDIF+]

dist_doc_DATA = \
	README \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS

[+IF (=(get "HaveI18n") "1") +]
INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = \
	$(INTLTOOL_FILES)

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache
[+ENDIF+]

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(docdir)
