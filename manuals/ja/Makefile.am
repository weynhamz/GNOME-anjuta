########################################################
# Add a new language module for _THIS_ document module #
# as a subdir here.                                    #
########################################################

lang = ja
langdir = $(prefix)/@NO_PREFIX_PACKAGE_HELP_DIR@/$(lang)

SUBDIRS = anjuta-manual anjuta-faqs

EXTRA_DIST = topic.dat

install-data-am:
	-if [ -e $(srcdir)/topic.dat ]; then \
		$(INSTALL_DATA) $(srcdir)/topic.dat $(DESTDIR)$(langdir); \
	 fi

