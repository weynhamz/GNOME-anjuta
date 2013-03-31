
SUBDIRS = src po

wizard_filesdir = $(anjuta_data_dir)/templates/terminal
wizard_files_DATA = \
	AUTHORS \
	ChangeLog \
	autogen.sh \
	configure.ac.tpl \
	Makefile.am.tpl \
	NEWS \
	project.anjuta \
	README

EXTRA_DIST = $(wizard_files_DATA)

-include $(top_srcdir)/git.mk
