
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

install-data-local: install-mimexmlDATA
	update-mime-database $(DESTDIR)$(datadir)/mime

uninstall-local:
	update-mime-database $(DESTDIR)$(datadir)/mime
