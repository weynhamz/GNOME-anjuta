# Stolen from scrollekeeper.
# Hacked according to requirements for anjuta.
#
# To use this template:
#     1) Manually change 'docdir' below as necessary
#     2) Define: figs, docname, lang, omffile, sgml_ents
#        although figs, omffile, and sgml_ents may be empty
#     3) Optionally define 'sgml_ents' to hold sgml entities which
#        you would also like installed
#     4) Figures must go under images/ and be in PNG format
#     5) You should only have one document per directory 
#     6) Note that the path  
#
# eg:
#   figs = \
#	fugures/img1.png		\
#	figures/img2.png
#
#   docname = anjuta-manual
#   lang = C
#   omffile=anjuta-manual-C.omf
#   sgml_ents = \
#	fdl.sgml		\
#	fdl.sgml
#
#   include $(top_srcdir)/docs/sgmldocs.make
#   dist-hook: app-dist-hook
#

docdir = $(prefix)/@NO_PREFIX_PACKAGE_HELP_DIR@/$(lang)/$(docname)

doc_DATA = index.html

sgml_files = $(sgml_ents) $(docname).sgml

omf_dir=$(top_srcdir)/omf-install

EXTRA_DIST = $(sgml_files) $(doc_DATA) $(omffile) $(figs)

CLEANFILES = omf_timestamp

all: index.html omf

omf: omf_timestamp

omf_timestamp: $(omffile)
	-for file in $(omffile); do \
	   scrollkeeper-preinstall $(docdir)/$(docname).sgml $$file $(omf_dir)/$$file; \
	done
	touch omf_timestamp

index.html: $(docname)/index.html
	-cp $(docname)/index.html .

$(docname).sgml: $(sgml_ents)
        ourdir=`pwd` && cd $(srcdir) && cp -f $(sgml_ents) $$ourdir || true

# The weird srcdir trick is because the db2html from the Cygnus RPMs
# cannot handle relative filenames
$(docname)/index.html: $(srcdir)/$(docname).sgml $(sgml_ents)
	-if test -e $(docname)/index.html ; then \
		rm -f $(docname)/index.html ; \
	fi
	-srcdir=`cd $(srcdir) && pwd`; \
	if test "$(HAVE_JW)" = 'yes' ; then \
		jw -c /etc/sgml/catalog $$srcdir/$(docname).sgml -o $$srcdir/$(docname); \
	else \
		db2html $$srcdir/$(docname).sgml; \
	fi
	-if test ! -e $(docname)/index.html ; then \
		if [ -e $(docname)/t1.html ]; then \
			cp $(docname)/t1.html $(docname)/index.html; \
		elif [ -e $(docname)/book1.html ]; then \
			cp $(docname)/book1.html $(docname)/index.html; \
		fi \
	fi

$(docname).ps: $(srcdir)/$(docname).sgml $(sgml_ents)
	-if test -e $(docname).ps ; then \
		rm -f $(docname).ps ; \
	fi
	-srcdir=`cd $(srcdir) && pwd`; \
	if test "$(HAVE_JW)" = 'yes' ; then \
		jw -b ps -c /etc/sgml/catalog $$srcdir/$(docname).sgml; \
	else \
		db2ps $$srcdir/$(docname).sgml; \
	fi

$(docname).pdf: $(docname).ps
	ps2pdf $(docname).ps

app-dist-hook: index.html
	-$(mkinstalldirs) $(distdir)/figures
	-cp $(srcdir)/$(docname)/*.html $(distdir)/$(docname)
	-cp $(srcdir)/figures/*.png $(distdir)/figures
	-if [ -e topic.dat ]; then \
		cp $(srcdir)/topic.dat $(distdir); \
	 fi

install-data-am: index.html omf
	-$(mkinstalldirs) $(DESTDIR)$(docdir)/figures
	-cp $(srcdir)/$(sgml_files) $(DESTDIR)$(docdir)
	-for file in $(srcdir)/$(docname)/*.html ; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/$$basefile; \
	done
	-for file in $(srcdir)/figures/*.png; do \
	  basefile=`echo $$file | sed -e  's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/figures/$$basefile; \
	done

uninstall-local:
	-for file in $(srcdir)/figures/*.png; do \
	  basefile=`echo $$file | sed -e  's,^.*/,,'`; \
	  rm -f $(docdir)/figures/$$basefile; \
	done
	-for file in $(srcdir)/$(docname)/*.html ; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  rm -f $(DESTDIR)$(docdir)/$$basefile; \
	done
	-for file in $(sgml_files); do \
	  rm -f $(DESTDIR)$(docdir)/$$file; \
	done
	-rmdir $(DESTDIR)$(docdir)/figures
	-rmdir $(DESTDIR)$(docdir)
