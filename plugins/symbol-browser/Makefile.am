# Plugin UI file
symbol_browser_uidir = $(anjuta_ui_dir)
symbol_browser_ui_DATA = anjuta-symbol-browser-plugin.ui

# Plugin pixmap files
symbol_browser_pixmapsdir = $(anjuta_image_dir)
symbol_browser_pixmaps_DATA = \
	anjuta-symbol-browser-plugin.png \
	sv_class.xpm \
	sv_private_var.xpm \
	sv_public_var.xpm \
	sv_unknown.xpm \
	sv_function.xpm \
	sv_protected_fun.xpm \
	sv_static_fun.xpm \
	sv_variable.xpm \
	sv_macro.xpm \
	sv_protected_var.xpm \
	sv_static_var.xpm \
	sv_private_fun.xpm \
	sv_public_fun.xpm \
	sv_struct.xpm

# Plugin description file
plugin_in_files = anjuta-symbol-browser.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

symbol_browser_plugindir = $(anjuta_plugin_dir)
symbol_browser_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
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
	an_symbol_info.c \
	an_symbol_info.h \
	an_symbol_view.h \
	an_symbol_view.c \
	an_symbol_search.h \
	an_symbol_search.c \
	plugin.c 

libanjuta_symbol_browser_la_LDFLAGS = -module


# Plugin dependencies
libanjuta_symbol_browser_la_LIBADD = \
	$(GDL_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(top_builddir)/tagmanager/libanjuta-ctags.la

 
EXTRA_DIST = \
	$(plugin_in_files) \
	$(symbol_browser_plugin_DATA) \
	$(symbol_browser_pixmaps_DATA) \
	$(symbol_browser_ui_DATA)
