## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

EXTRA_DIST = create_global_tags.sh

AM_CFLAGS = -I../tagmanager/include -Wall $(GLIB_CFLAGS)

noinst_PROGRAMS = tm_global_tags

tm_global_tags_SOURCES = tm_global_tags.c

LDADD = -L../tagmanager $(GLIB_LIBS) -lctags

all-local: system.tags

system.tags: create_global_tags.sh tm_global_tags
	echo "Detecting header files and Generating global symbols ....."
	$(srcdir)/create_global_tags.sh 2> /dev/null ; echo "....... Done"
	cp $(srcdir)/system.tags $(top_srcdir)/data
