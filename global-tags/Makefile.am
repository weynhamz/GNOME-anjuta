## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

EXTRA_DIST = create_global_tags.sh

AM_CFLAGS = -I$(top_srcdir)/tagmanager/include -Wall $(GLIB_CFLAGS)

noinst_PROGRAMS = tm_global_tags

tm_global_tags_SOURCES = tm_global_tags.c

LDADD = $(top_builddir)/tagmanager/libctags.a
LDFLAGS = $(GLIB_LIBS)

system.tags: create_global_tags.sh tm_global_tags
	echo "Detecting header files and Generating global symbols ....."
	chmod +x $(srcdir)/create_global_tags.sh
	(cd $(top_builddir)/global-tags; $(srcdir)/create_global_tags.sh)

