# Plugin glade file
message_view_gladedir = $(anjuta_glade_dir)
message_view_glade_DATA = anjuta-message-manager-plugin.glade

message_view_pixmapsdir = $(anjuta_image_dir)
message_view_pixmaps_DATA = \
	anjuta-message-view.png \
	preferences-messages.png \
	error-next.png \
	error-prev.png \
	error-next-16.png \
	error-prev-16.png \
	messages.xpm

message_view_uidir = $(anjuta_ui_dir)
message_view_ui_DATA = anjuta-message-manager.ui

plugin_in_files = anjuta-message-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

message_view_plugindir = $(anjuta_plugin_dir)
message_view_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-message-view.la

libanjuta_message_view_la_LDFLAGS = -module
libanjuta_message_view_la_LIBADD = \
	$(LIBANJUTA_LIBS)

libanjuta_message_view_la_SOURCES = \
	plugin.c\
	plugin.h\
	anjuta-msgman.c\
	anjuta-msgman.h\
	message-view.c\
	message-view.h

EXTRA_DIST = \
	$(plugin_in_files) \
	$(message_view_pixmaps_DATA) \
	$(message_view_plugin_DATA) \
	$(message_view_ui_DATA) \
	$(message_view_glade_DATA)
