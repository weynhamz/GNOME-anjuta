
if HAVE_GRAPHVIZ

# Plugin Icon file
cls_inheritance_pixmapsdir = $(anjuta_image_dir)
cls_inheritance_pixmaps_DATA = \
	anjuta-class-inheritance-plugin.svg \
	anjuta-class-inheritance-plugin-48.png

# Plugin description file
plugin_in_files = anjuta-class-inheritance.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

cls_inheritance_plugindir = $(anjuta_plugin_dir)
cls_inheritance_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
AM_CPPFLAGS = \
	$(LIBANJUTA_CFLAGS) \
	$(WARN_CFLAGS) \
	$(GDL_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GRAPHVIZ_CFLAGS) \
	-DG_LOG_DOMAIN=\"libanjuta-class-inheritance\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-class-inheritance.la

libanjuta_class_inheritance_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_class_inheritance_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(GNOME_UI_LIBS) \
	$(GRAPHVIZ_LIBS)

# Plugin sources
libanjuta_class_inheritance_la_SOURCES = \
	plugin.c \
	plugin.h \
	class-inherit.c \
	class-inherit.h \
	class-callbacks.c \
	class-callbacks.h 

endif

EXTRA_DIST = \
	$(plugin_in_files) \
	$(cls_inheritance_plugin_DATA) \
	$(cls_inheritance_ui_DATA) \
	$(cls_inheritance_glade_DATA) \
	$(cls_inheritance_pixmaps_DATA)
