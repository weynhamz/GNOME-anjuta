[+ autogen5 template +]
[+
(define prefix_if_missing
        (lambda
                (name prefix)
                (string-append
                         (if
                                (==* (get name) prefix)
                                ""
                                prefix
                        )
                        (get name)
                )
        )
)
+]## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"[+IF (=(get "HavePackage") "1")+] \
	$([+NameCUpper+]_CFLAGS)[+ENDIF+]

AM_CFLAGS =\
	 -Wall\
	 -g

lib_LTLIBRARIES = [+(prefix_if_missing "NameHLower" "lib")+].la


[+(prefix_if_missing "NameCLower" "lib")+]_la_SOURCES = \
	lib.c

[+(prefix_if_missing "NameCLower" "lib")+]_la_LDFLAGS = 

[+(prefix_if_missing "NameCLower" "lib")+]_la_LIBADD = [+IF (=(get "HavePackage") "1")+]$([+NameCUpper+]_LIBS)[+ENDIF+]

include_HEADERS = \
	[+NameHLower+].h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = [+NameHLower+]-[+Version+].pc

EXTRA_DIST = \
	[+NameHLower+]-[+Version+].pc.in
