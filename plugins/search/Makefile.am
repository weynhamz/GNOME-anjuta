# Plugin glade file
search_gladedir = $(anjuta_glade_dir)
search_glade_DATA = anjuta-search.glade

# Plugin Icon file
search_pixmapsdir = $(anjuta_image_dir)
search_pixmaps_DATA = \
	anjuta-search-plugin.svg \
	anjuta-search-plugin-48.png

plugin_in_files = anjuta-search.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

search_plugindir = $(anjuta_plugin_dir)
search_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-search.la

# Plugin UI file
search_uidir = $(anjuta_ui_dir)
search_ui_DATA = anjuta-search.ui

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GNOME_PRINT_UI_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-I$(top_srcdir)/tagmanager/include

libanjuta_search_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_search_la_LIBADD = \
	$(GNOME_PRINT_UI_LIBS) \
	$(GNOME_UI_LIBS) \
	$(GLADE_LIBS) \
	$(LIBANJUTA_LIBS)

libanjuta_search_la_SOURCES= \
	plugin.c \
	plugin.h \
	search-replace.c \
	search-replace.h \
	search-replace_backend.c \
	search-replace_backend.h \
	search_preferences.c \
	search_preferences.h

EXTRA_DIST = \
	$(plugin_in_files) \
	$(search_plugin_DATA) \
	$(search_ui_DATA) \
	$(search_pixmaps_DATA) \
	$(search_glade_DATA)
