
appdir = $(datadir)/application-registry
app_DATA = anjuta.applications

mimedir = $(datadir)/mime-info
mime_DATA = anjuta.keys anjuta.mime

mimexmldir = $(datadir)/mime/packages
mimexml_DATA = anjuta.xml

EXTRA_DIST = \
	$(app_DATA) \
	$(mime_DATA) \
	$(mimexml_DATA)

install-data-local:
	update-mime-database $(datadir)/mime

uninstall-local:
	update-mime-database $(datadir)/mime
