# Plugin glade file
clsgen_gladedir = $(anjuta_glade_dir)
clsgen_glade_DATA = anjuta-class-gen-plugin.glade

# Templates
clsgen_appdatadir = "$(datadir)/anjuta/class-templates/"
clsgen_appdata_DATA = \
	goc_template_header \
	goc_template_source 

# Plugin Icon file
plugin_in_files = anjuta-class-gen.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache


# Plugin pixmaps
clsgen_pixmapsdir = $(anjuta_image_dir)
clsgen_pixmaps_DATA = \
	class_logo.xpm


clsgen_plugindir = $(anjuta_plugin_dir)
clsgen_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-class-gen.la

INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)


libanjuta_class_gen_la_LIBADD = \
	$(LIBANJUTA_LIBS)

libanjuta_class_gen_la_SOURCES= \
	plugin.c \
	plugin.h \
	class_gen.c \
	class_gen.h \
	action-callbacks.c \
	action-callbacks.h 
	
EXTRA_DIST = \
	$(clsgen_pixmaps_DATA) \
	$(clsgen_glade_DATA) \
	$(clsgen_appdata_DATA) \
	$(clsgen_plugin_DATA)
