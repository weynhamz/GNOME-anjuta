## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

bindir = $(anjuta_data_dir)/scripts
dist_bin_SCRIPTS = create_global_tags.sh

INCLUDES = -I$(top_srcdir)/tagmanager/include $(GLIB_CFLAGS)

bin_PROGRAMS = anjuta-tags

anjuta_tags_SOURCES = tm_global_tags.c

anjuta_tags_LDADD = $(top_builddir)/tagmanager/libanjuta-ctags.la
anjuta_tags_LDFLAGS = $(GLIB_LIBS)

edit = sed \
	-e 's,@scriptdir\@,$(bindir),g'

create_global_tags.sh: Makefile create_global_tags.sh.in
	rm -f create_global_tags.sh
	$(edit) $(srcdir)/create_global_tags.sh.in > create_global_tags.sh
	chmod +x create_global_tags.sh

EXTRA_DIST = $(dist_bin_SCRIPTS) create_global_tags.sh.in
