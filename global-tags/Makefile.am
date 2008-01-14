## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

bindir = $(pkglibdir)
scriptdir = $(anjuta_data_dir)/scripts

script_SCRIPTS = create_global_tags.sh

AM_CPPFLAGS = -I$(top_srcdir)/tagmanager/include $(GLIB_CFLAGS)

bin_PROGRAMS = anjuta-tags

anjuta_tags_SOURCES = tm_global_tags.c

anjuta_tags_LDADD = $(top_builddir)/tagmanager/libanjuta-ctags.la
anjuta_tags_LDFLAGS = $(GLIB_LIBS)

edit = sed \
	-e 's,\@bindir\@,$(bindir),g'

create_global_tags.sh: Makefile create_global_tags.sh.in
	rm -f create_global_tags.sh
	$(edit) $(srcdir)/create_global_tags.sh.in > create_global_tags.sh
	chmod +x create_global_tags.sh

EXTRA_DIST = create_global_tags.sh.in
