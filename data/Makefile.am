## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = properties

anjuta_datadir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@

anjuta_data_DATA = \
	gdb.init\
	welcome.txt

EXTRA_DIST = $(anjuta_data_DATA)

## $(anjuta_mime_DATA) $(anjuta_mimelnk_DATA)

install-data-local: system.tags
	$(INSTALL_DATA) system.tags $(DESTDIR)$(anjuta_datadir)

uninstall-local:
	@rm -fv $(anjuta_datadir)/system.tags
	-if test -d $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/properties; then \
	    rmdir $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/properties; fi

clean-local:
	@rm -f system.tags

system.tags:
	cd $(top_builddir)/tagmanager && $(GMAKE) libanjuta-ctags.la
	cd $(top_builddir)/global-tags && $(GMAKE) system.tags
	mv $(top_builddir)/global-tags/system.tags .
