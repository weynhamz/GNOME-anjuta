# Plugin glade file
anjuta_gladedir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
anjuta_glade_DATA =  anjuta-message-manager-plugin.glade

plugin_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_pixmaps_DATA = \
	anjuta-message-view.png \
	preferences-messages.png \
	error-next.png \
    error-prev.png \
	error-next-16.png \
	error-prev-16.png \
	messages.xpm

anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta-message-manager.ui

plugin_in_files = anjuta-message-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; \
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u \ \
		-c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(PCRE_CFLAGS)\
	$(DEPRECATED_FLAGS)\
	$(LIBANJUTA_CFLAGS)

plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
plugin_LTLIBRARIES = libanjuta-message-view.la

libanjuta_message_view_la_LIADD = $(LIBANJUTA_LIBS)
libanjuta_message_view_la_LIBADD = \
	$(top_builddir)/libanjuta/libanjuta.la \
	$(top_builddir)/libanjuta/interfaces/libanjuta-interfaces.la

libanjuta_message_view_la_SOURCES = \
	plugin.c\
	plugin.h\
	anjuta-msgman.c\
	anjuta-msgman.h\
	message-view.c\
	message-view.h

EXTRA_DIST = \
	$(plugin_pixmaps_DATA) \
	$(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA)
