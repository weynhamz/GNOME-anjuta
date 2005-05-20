
appdir = $(datadir)/application-registry
app_DATA = anjuta.applications

mimedir = $(datadir)/mime-info
mime_DATA = anjuta.keys anjuta.mime

mimexmldir = $(datadir)/mime/packages
mimexml_DATA = anjuta.xml

mimeicondir = $(datadir)/icons/gnome/48x48/mimetypes
mimeicon_DATA = gnome-mime-application-x-anjuta.png

EXTRA_DIST = \
	$(app_DATA) \
	$(mime_DATA) \
	$(mimexml_DATA) \
	$(mimeicon_DATA)

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
