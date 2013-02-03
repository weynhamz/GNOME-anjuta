# Templates
template_datadir = $(anjuta_data_dir)/templates
template_data_DATA = \
	indent.tpl

licenses_datadir = $(template_datadir)/licenses
licenses_data_DATA = \
	licenses/gpl.txt \
	licenses/gpl.tpl \
	licenses/lgpl.txt \
	licenses/lgpl.tpl \
	licenses/bsd.txt \
	licenses/bsd.tpl \
	licenses/none.txt \
	licenses/none.tpl

EXTRA_DIST = \
	$(template_data_DATA) \
	$(licenses_data_DATA)

-include $(top_srcdir)/git.mk
