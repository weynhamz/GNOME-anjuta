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
	autogen.sh.gnomemm

EXTRA_DIST = $(anjuta_data_DATA)

MACROS = \
	ChangeLog \
	Makefile.am \
	aclocal-include.m4 \
	autogen.sh \
	compiler-flags.m4 \
	curses.m4 \
	gnome-bonobo-check.m4 \
	gnome-common.m4 \
	gnome-cxx-check.m4 \
	gnome-fileutils.m4 \
	gnome-gettext.m4 \
	gnome-ghttp-check.m4 \
	gnome-gnorba-check.m4 \
	gnome-guile-checks.m4 \
	gnome-libgtop-check.m4 \
	gnome-objc-checks.m4 \
	gnome-orbit-check.m4 \
	gnome-pilot.m4 \
	gnome-print-check.m4 \
	gnome-pthread-check.m4 \
	gnome-support.m4 \
	gnome-undelfs.m4 \
	gnome-vfs.m4 \
	gnome-x-checks.m4 \
	gnome-xml-check.m4 \
	gnome.m4 \
	gperf-check.m4 \
	linger.m4 \
	need-declaration.m4 \
	psiconv.m4

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
