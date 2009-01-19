# Plugin UI file
gtodo_uidir = $(anjuta_ui_dir)
gtodo_ui_DATA = anjuta-gtodo.ui

gtodo_datadir = $(anjuta_data_dir)/gtodo
gtodo_data_DATA = gtodo.css \
				  gtodo.xsl \
				  gtodo-plain.xsl

# Plugin pixmaps
gtodo_pixmapsdir = $(anjuta_image_dir)
gtodo_pixmaps_DATA = \
	anjuta-gtodo-plugin.svg \
	anjuta-gtodo-plugin-48.png \
	gtodo-about.png \
	gtodo-edit.png

# Plugin Icon file
plugin_in_files = anjuta-gtodo.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

gtodo_plugindir = $(anjuta_plugin_dir)
gtodo_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-gtodo.la

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GTK_CFLAGS) \
	$(GIO_CFLAGS) \
	$(LIBXSLT_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DDATADIR="\"$(anjuta_data_dir)\"" \
	-DPIXMAP_PATH="\"$(anjuta_image_dir)\""

libanjuta_gtodo_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_gtodo_la_LIBADD = \
	$(GTK_LIBS) \
	$(GIO_LIBS) \
	$(LIBXSLT_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_gtodo_la_SOURCES=\
	plugin.c\
	plugin.h\
	add_edit_item.c\
	callback.c\
	debug_printf.c\
	debug_printf.h\
	egg-datetime.c\
	egg-datetime.h\
	export.c\
	interface.c\
	libgtodo.c\
	libgtodo.h\
	list_tooltip.c\
	mcategory.c\
	main.h\
	notification.c\
	preferences.c\
	tasklists.c\
	todo_db.c

EXTRA_DIST = \
	$(gtodo_pixmaps_DATA) \
	$(plugin_in_files) \
	$(gtodo_plugin_DATA) \
	$(gtodo_ui_DATA) \
	$(gtodo_data_DATA)
