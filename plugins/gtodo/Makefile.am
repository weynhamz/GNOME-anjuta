# Plugin UI file
anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta-gtodo.ui

tododir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/gtodo
todoDATA = gtodo.css gtodo.xsl

# Plugin pixmaps
plugin_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_pixmaps_DATA = \
	anjuta-gtodo-plugin.png\
	gtodo.png\
	gtodo_tray.png\
	gtodo-about.png\
	gtodo-edit.png

# Plugin Icon file
plugin_in_files = anjuta-gtodo.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
plugin_LTLIBRARIES = libanjuta-gtodo.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(LIBXSLT_CFLAGS)\
	-DDATADIR="\"$(PACKAGE_DATA_DIR)\"" \
	-DPIXMAP_PATH="\"$(PACKAGE_PIXMAPS_DIR)\""

##	$(PLUGIN_GTODO_CFLAGS)

libanjuta_gtodo_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(LIBXSLT_LIBS)\
	$(top_builddir)/libanjuta/libanjuta.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

##	$(PLUGIN_GTODO_LIBS)

libanjuta_gtodo_la_SOURCES=\
	plugin.c\
	plugin.h\
	add_edit_item.c\
	callback.c\
	debug_printf.c\
	debug_printf.h\
	egg-datetime.c\
	egg-datetime.h\
	eggtrayicon.c\
	eggtrayicon.h\
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
	todo_db.c\
	tray-icon.c\
	tray-icon.h

#	main.c\
#	gtodo.c

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA)
