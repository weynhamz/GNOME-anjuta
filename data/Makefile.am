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
	autogen.sh.gnome\
	autogen.sh.gnome2\
	autogen.sh.gtk\
	autogen.sh.generic\
	autogen.sh.gtkmm\
	autogen.sh.gnomemm\
	autogen.sh.wxwin


anjuta_mimedir = $(prefix)/share/mime-info

anjuta_mime_DATA = anjuta.mime

MACROS = \
	ChangeLog \
	Makefile.am \
	autogen.sh \
	*.m4

MACROS_dir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/gnome

EXTRA_DIST = $(anjuta_data_DATA) $(anjuta_mime_DATA)

install-data-local: system.tags
	mkdir -p $(MACROS_dir)
	$(INSTALL_DATA) $(MACROS:%=$(top_srcdir)/macros/%) $(MACROS_dir)
	$(INSTALL_DATA) system.tags $(anjuta_datadir)
	$(INSTALL_DATA) $(top_srcdir)/setup-gettext $(anjuta_datadir)

uninstall-local:
	@rm -fv $(MACROS:%=$(MACROS_dir)/%)
	@rm -fv $(anjuta_datadir)/system.tags
	if test -d $(MACROS_dir); then \
	  rmdir $(MACROS_dir); \
	  \
	  if test -d $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@; then \
	    rmdir $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@; \
	  fi; \
	fi

clean-local:
	@rm -f system.tags

system.tags:
	cd $(top_builddir)/tagmanager && make libctags.a
	cd $(top_builddir)/global-tags && make system.tags
	mv $(top_builddir)/global-tags/system.tags .

