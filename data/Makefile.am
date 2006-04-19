## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = properties

anjuta_datadir = $(anjuta_data_dir)

anjuta_data_DATA = \
	gdb.init\
	welcome.txt

EXTRA_DIST = $(anjuta_data_DATA)

uninstall-local:
	@rm -fv $(anjuta_datadir)/system.tags
	-if test -d $(DESTDIR)$(anjuta_data_dir)/properties; then \
	    rmdir $(DESTDIR)$(anjuta_data_dir)/properties; fi

clean-local:
	@rm -f system.tags

system.tags:
	cd $(top_builddir)/tagmanager && $(GMAKE) libanjuta-ctags.la
	cd $(top_builddir)/global-tags && $(GMAKE) system.tags
	mv $(top_builddir)/global-tags/system.tags .
