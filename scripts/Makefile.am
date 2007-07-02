anjuta_datadir = $(anjuta_data_dir)

bin_SCRIPTS = anjuta_import.sh

anjuta_data_DATA = anjuta_project.template

EXTRA_DIST = $(anjuta_data_DATA)

# Distribute but don't install glade2schema.pl
dist-hook:
	cp $(srcdir)/glade2schema.pl $(distdir)
	chmod +x $(distdir)/glade2schema.pl
