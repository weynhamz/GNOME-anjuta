## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

anjuta_datadir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@

anjuta_data_DATA = \
	anjuta.properties\
	gdb.init\
	README.stock_libs\
	header.c\
	internal.properties\
	stock_libs.anj\
	welcome.txt\
	autogen.sh.gnome\
	autogen.sh.gtk\
	autogen.sh.generic\
	linux-gnome-c.api\
	tags2api.py\
	autogen.sh.gtkmm\
	autogen.sh.gnomemm\
	system.tags

EXTRA_DIST = $(anjuta_data_DATA)

MACROS = \
	ChangeLog \
	Makefile.am \
	autogen.sh \
	*.m4

MACROS_dir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/gnome

install-data-local:
	mkdir -p $(MACROS_dir)
	$(INSTALL_DATA) $(MACROS:%=$(top_srcdir)/macros/%) $(MACROS_dir)

uninstall-local:
	@rm -fv $(MACROS:%=$(MACROS_dir)/%)
	if test -d $(MACROS_dir); then \
	  rmdir $(MACROS_dir); \
	  \
	  if test -d $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@; then \
	    rmdir $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@; \
	  fi; \
	fi
