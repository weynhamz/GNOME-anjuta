## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = properties

anjuta_datadir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@

anjuta_data_DATA = \
	gdb.init\
	README.stock_libs\
	header.c\
	stock_libs.anj\
	welcome.txt\
	autogen.sh


anjuta_mimedir = $(prefix)/share/mime-info
anjuta_mime_DATA = anjuta.mime

anjuta_mimelnkdir = $(prefix)/share/mimelnk/application
anjuta_mimelnk_DATA = x-anjuta-project.desktop

EXTRA_DIST = $(anjuta_data_DATA) $(anjuta_mime_DATA) $(anjuta_mimelnk_DATA)

install-data-local: system.tags
	$(INSTALL_DATA) system.tags $(DESTDIR)$(anjuta_datadir)
	$(INSTALL_DATA) $(top_srcdir)/setup-gettext $(DESTDIR)$(anjuta_datadir)

uninstall-local:
	@rm -fv $(anjuta_datadir)/system.tags
	@rm -fv $(anjuta_datadir)/setup-gettext
	mv $(MACROS_dir)/Makefile.am $(MACROS_dir)/Makefile.am.project
	mv $(MACROS2_dir)/Makefile.am $(MACROS2_dir)/Makefile.am.project
	@rm -fv $(MACROS:%=$(MACROS_dir)/%)
	@rm -fv $(MACROS2:%=$(MACROS2_dir)/%)
	-if test -d $(MACROS_dir); then rmdir $(MACROS_dir); fi
	-if test -d $(MACROS2_dir); then rmdir $(MACROS2_dir); fi
	-if test -d $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/properties; then \
	    rmdir $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/properties; fi

clean-local:
	@rm -f system.tags

system.tags:
	cd $(top_builddir)/tagmanager && $(GMAKE) libctags.a
	cd $(top_builddir)/global-tags && $(GMAKE) system.tags
	mv $(top_builddir)/global-tags/system.tags .
