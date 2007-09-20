# Plugin UI file
file_manager_uidir = $(anjuta_ui_dir)
file_manager_ui_DATA =  file-manager.ui

# Plugin Icon file
file_manager_pixmapsdir = $(anjuta_image_dir)
file_manager_pixmaps_DATA = file-manager.png

# Plugin description file
plugin_in_files = file-manager.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

file_manager_plugindir = $(anjuta_plugin_dir)
file_manager_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
INCLUDES = \
	$(LIBANJUTA_CFLAGS) \
	$(GDL_CFLAGS) 

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libfile-manager.la

# Plugin sources
libfile_manager_la_SOURCES = \
	plugin.c \
	plugin.h \
	file-view-marshal.h \
	file-view-marshal.c \
	file-view.h \
	file-view.c

# Plugin dependencies
libfile_manager_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(GDL_LIBS) 

file-view-marshal.h: file-view-marshal.list
	@GLIB_GENMARSHAL@ \
	        --prefix=file_view_cclosure_marshal file-view-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

file-view-marshal.c: file-view-marshal.h file-view-marshal.list
	echo "#include \"file-view-marshal.h\"" > xgen-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=file_view_cclosure_marshal file-view-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc	
	
EXTRA_DIST = \
	$(plugin_in_files) \
	$(file_manager_plugin_DATA) \
	$(file_manager_ui_DATA) \
	$(file_manager_pixmaps_DATA)
