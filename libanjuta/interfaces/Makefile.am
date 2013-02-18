include Makefile.am.iface

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I . \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(ANJUTA_CFLAGS) \
	$(XML_CFLAGS) \
	$(GDL_CFLAGS) \
	-DDATADIR="\"$(datadir)\"" \
	-DPACKAGE_PIXMAPS_DIR="\"$(datadir)/pixmaps/$(PACKAGE)\"" \
	-DPACKAGE_DATA_DIR="\"$(datadir)/$(PACKAGE)\"" \
	-DPACKAGE_HELP_DIR="\"$(datadir)/gnome/help/$(PACKAGE)\"" \
	-DPACKAGE_DOC_DIR="\"$(docdir)\"" \
	-DG_LOG_DOMAIN=\"libanjuta-interfaces\"

# interfaces
noinst_LTLIBRARIES = libanjuta-interfaces.la
libanjuta_interfaces_la_LIBADD = $(ANJUTA_LIBS) $(XML_LIBS)

libanjuta_interfaces_includedir = $(includedir)/libanjuta-3.0/libanjuta/interfaces
libanjuta_interfaces_include_HEADERS =  $(libanjuta_interfaces_include)

BUILT_SOURCES = iface-built.stamp

libanjuta-interfaces.h: iface-built.stamp

iface-built.stamp: libanjuta.idl anjuta-idl-compiler.pl
	savedir=`pwd` && \
	cd $(srcdir) && perl anjuta-idl-compiler.pl libanjuta && \
	echo 'GInterface' > hierarchy.txt && grep -e '^[[:space:]|\t]*interface' \
	libanjuta.idl  | sed -e 's/\binterface\b\s*/\t/' | sed -e 's/\t/    /g' \
	>> hierarchy.txt && \
	touch iface-built.stamp && \
	echo "changing to $$savedir" && \
	cd $$savedir && \
	make

files_idl = libanjuta.idl
idldir = $(libanjuta_interfaces_includedir)
idl_HEADERS = $(files_idl)

anjuta-marshal.h: interfaces/anjuta-marshal.list
	$(AM_V_GEN)@GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)/anjuta-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

anjuta-marshal.c: interfaces/anjuta-marshal.h interfaces/anjuta-marshal.list
	$(AM_V_GEN)echo "#include \"ianjuta-marshal.h\"" > xgenc-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=anjuta_cclosure_marshal $(srcdir)anjuta-marshal.list --body >> xgenc-gmc \
	&& cp xgenc-gmc $(@F) \
	&& rm -f xgenc-gmc

EXTRA_DIST = \
	iface-built.stamp \
	$(libanjuta_interfaces_include) \
	$(files_idl) \
	Makefile.am.iface \
	anjuta-idl-compiler.pl


CLEANFILES = libanjuta-interfaces.h iface-built.stamp

-include $(top_srcdir)/git.mk
