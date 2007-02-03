
# Plugin UI file
profiler_uidir = $(anjuta_ui_dir)
profiler_ui_DATA =  profiler.ui

# Plugin glade file
profiler_gladedir = $(anjuta_glade_dir)
profiler_glade_DATA = profiler.glade \
					  profiler-flat-profile.glade \
					  profiler-call-graph.glade \
					  profiler-function-call-tree.glade

# Plugin Icon file
profiler_pixmapsdir = $(anjuta_image_dir)
profiler_pixmaps_DATA = profiler.png

# Plugin description file
plugin_in_files = profiler.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

profiler_plugindir = $(anjuta_plugin_dir)
profiler_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
INCLUDES = \
		   $(LIBANJUTA_CFLAGS) \
		   $(WARN_CFLAGS) \
		   $(GDL_CFLAGS) \
		   $(DEPRECATED_FLAGS)
		   
if HAVE_GRAPHVIZ
INCLUDES += $(GRAPHVIZ_CFLAGS)
endif

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-profiler.la

# Plugin sources
libanjuta_profiler_la_SOURCES = \
	gprof-call-graph.c \
	gprof-call-graph.h \
	gprof-call-graph-block.c \
	gprof-call-graph-block.h \
	gprof-call-graph-block-entry.c \
	gprof-call-graph-block-entry.h \
	gprof-call-graph-view.c \
	gprof-call-graph-view.h \
	gprof-flat-profile.c \
	gprof-flat-profile.h \
	gprof-flat-profile-entry.c \
	gprof-flat-profile-entry.h \
	gprof-flat-profile-view.c \
	gprof-flat-profile-view.h \
	gprof-function-call-tree-view.c \
	gprof-function-call-tree-view.h \
	gprof-profile-data.c \
	gprof-profile-data.h \
	gprof-view.c \
	gprof-view.h \
	gprof-view-manager.c \
	gprof-view-manager.h \
	string-utils.c \
	string-utils.h \
	plugin.c \
	plugin.h

if HAVE_GRAPHVIZ
libanjuta_profiler_la_SOURCES += \
						  		 gprof-function-call-chart-view.c \
						  		 gprof-function-call-chart-view.h 
endif

# Plugin dependencies
libanjuta_profiler_la_LIBADD = $(LIBANJUTA_LIBS)

if HAVE_GRAPHVIZ
libanjuta_profiler_la_LIBADD += $(GRAPHVIZ_LIBS)
endif

EXTRA_DIST = \
			 $(plugin_in_files) \
			 $(profiler_plugin_DATA) \
			 $(profiler_ui_DATA) \
			 $(profiler_pixmaps_DATA) \
			 $(profiler_glade_DATA)

