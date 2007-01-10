mimexmldir = $(datadir)/mime/packages
mimexml_DATA = anjuta.xml

mimepngicondir = $(datadir)/icons/gnome/48x48/mimetypes
mimepngicon_DATA = gnome-mime-application-x-anjuta.png

mimesvgicondir = $(datadir)/icons/gnome/scalable/mimetypes
mimesvgicon_DATA = gnome-mime-application-x-anjuta.svg

EXTRA_DIST = \
	$(mimexml_DATA) \
	$(mimepngicon_DATA) \
	$(mimesvgicon_DATA)

## Update mime only when system wide installation takes place

install-data-local: install-mimexmlDATA
	if [ -f $(DESTDIR)$(datadir)/mime/packages/freedesktop.org.xml ] ; then \
	  if which update-mime-database>/dev/null 2>&1; then \
	    update-mime-database $(DESTDIR)$(datadir)/mime; \
	  fi \
	fi

uninstall-local:
	if [ -f $(DESTDIR)$(datadir)/mime/packages/freedesktop.org.xml ] ; then \
	  if which update-mime-database>/dev/null 2>&1; then \
	    update-mime-database $(DESTDIR)$(datadir)/mime; \
	  fi \
	fi
