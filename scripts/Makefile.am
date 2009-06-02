anjuta_datadir = $(anjuta_data_dir)

anjuta_data_DATA = anjuta_project.template

EXTRA_DIST = $(anjuta_data_DATA)

# Distribute but don't install builder2schema.pl
dist-hook:
	cp $(srcdir)/builder2schema.pl $(distdir)
	chmod +x $(distdir)/builder2schema.pl
-include $(top_srcdir)/git.mk
