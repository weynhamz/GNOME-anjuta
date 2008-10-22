[+ autogen5 template +]
## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src [+IF (=(get "HaveI18n") "1") +]po[+ENDIF+]

[+NameCLower+]docdir = ${prefix}/doc/[+NameHLower+]
[+NameCLower+]doc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

EXTRA_DIST = $([+NameCLower+]doc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $([+NameCLower+]docdir)
