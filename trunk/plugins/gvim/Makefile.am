# Plugin Icon file
vim_pixmapsdir = $(anjuta_image_dir)
vim_pixmaps_DATA = \
	anjuta-vim-plugin.png

# Plugin description file
plugin_in_files = anjuta-vim.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

vim_plugindir = $(anjuta_plugin_dir)
vim_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) 

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-vim.la

# Plugin sources
libanjuta_vim_la_SOURCES = \
	plugin.c \
	plugin.h \
	anjuta-vim.c \
	anjuta-vim.h \
	gtkvim.c \
	gtkvim.h 

# Plugin dependencies
libanjuta_vim_la_LIBADD = \
	$(LIBANJUTA_LIBS) 

EXTRA_DIST = \
	$(plugin_in_files) \
	$(vim_plugin_DATA) \
	$(vim_pixmaps_DATA)
