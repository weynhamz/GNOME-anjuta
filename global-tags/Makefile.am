## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

EXTRA_DIST = create_global_tags.sh

INCLUDES = -I$(top_srcdir)/tagmanager/include -Wall $(GLIB_CFLAGS)

bin_PROGRAMS = anjuta-tags

anjuta_tags_SOURCES = tm_global_tags.c

anjuta_tags_LDADD = $(top_builddir)/tagmanager/libanjuta-ctags.la
anjuta_tags_LDFLAGS = $(GLIB_LIBS)

system.tags: create_global_tags.sh anjuta-tags
	echo "Detecting header files and Generating global symbols ....."
	chmod +x $(srcdir)/create_global_tags.sh
	(cd $(top_builddir)/global-tags && $(srcdir)/create_global_tags.sh && echo "This is only a timestamp" > system.tags)

all: system.tags

install-data-local: system.tags
	mkdir -p "$(DESTDIR)$(datadir)/anjuta/tags";
	for tagsfile in tags/*; do \
		$(INSTALL_DATA) $$tagsfile $(DESTDIR)$(datadir)/anjuta/tags/; \
	done

uninstall-local:
	rm -fr $(anjuta_datadir)/tags

clean-local:
	rm -fr system.tags tags
