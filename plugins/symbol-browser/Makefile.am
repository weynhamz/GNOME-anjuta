
SUBDIRS = images

# Plugin UI file
symbol_browser_uidir = $(anjuta_ui_dir)
symbol_browser_ui_DATA = anjuta-symbol-browser-plugin.ui

# Plugin UI file
symbol_browser_gladedir = $(anjuta_glade_dir)
symbol_browser_glade_DATA = anjuta-symbol-browser-plugin.glade

# Plugin pixmap files
symbol_browser_pixmapsdir = $(anjuta_image_dir)
symbol_browser_pixmaps_DATA = \
	anjuta-symbol-browser-plugin.svg \
	anjuta-symbol-browser-plugin-48.png

# Plugin description file
plugin_in_files = anjuta-symbol-browser.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

symbol_browser_plugindir = $(anjuta_plugin_dir)
symbol_browser_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GDL_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-I$(top_srcdir)/tagmanager/include 

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-symbol-browser.la

# Plugin sources
libanjuta_symbol_browser_la_SOURCES = \
	an_symbol.c \
	an_symbol.h \
	an_symbol_iter.h \
	an_symbol_iter.c \
	an_symbol_info.c \
	an_symbol_info.h \
	an_symbol_view.h \
	an_symbol_view.c \
	an_symbol_search.h \
	an_symbol_search.c \
	an_symbol_prefs.c \
	an_symbol_prefs.h \
	plugin.c \
	plugin.h \
	anjuta-symbol-locals.h \
	anjuta-symbol-locals.c 

libanjuta_symbol_browser_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_symbol_browser_la_LIBADD = \
	$(GDL_LIBS) \
	$(GNOME_UI_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(top_builddir)/tagmanager/libanjuta-ctags.la
	
prefs_glade_files = anjuta-symbol-browser-plugin.glade
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(symbol_browser_plugin_DATA) \
	$(symbol_browser_pixmaps_DATA) \
	$(symbol_browser_glade_DATA) \
	$(symbol_browser_ui_DATA)

noinst_PROGRAMS = \
	test-class-symbol-parsing 

test_class_symbol_parsing_SOURCES = \
	test-class.cpp             \
	test-class.h 




