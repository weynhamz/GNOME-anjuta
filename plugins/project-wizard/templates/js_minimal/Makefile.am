SUBDIRS = src

wizard_filesdir = $(anjuta_data_dir)/templates/js_minimal
wizard_files_DATA = \
	autogen.sh \
	configure.ac.tpl \
	Makefile.am.tpl \
	project.anjuta

EXTRA_DIST = $(wizard_files_DATA)

-include $(top_srcdir)/git.mk
