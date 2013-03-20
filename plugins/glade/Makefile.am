SUBDIRS = images

plugin_in_files = anjuta-glade.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

if HAVE_PLUGIN_GLADE

# Plugin glade file
glade_gladedir = $(anjuta_glade_dir)
dist_glade_glade_DATA = anjuta-glade.ui

# Plugin UI file
glade_uidir = $(anjuta_ui_dir)
dist_glade_ui_DATA = anjuta-glade.xml

# Plugin description file
glade_plugindir = $(anjuta_plugin_dir)
dist_glade_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PLUGIN_GLADE_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-glade.la

# Plugin sources
libanjuta_glade_la_SOURCES = plugin.c plugin.h \
	anjuta-design-document.h                      \
	anjuta-design-document.c 

libanjuta_glade_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_glade_la_LIBADD = \
	$(PLUGIN_GLADE_LIBS) \
	$(LIBANJUTA_LIBS)
endif

EXTRA_DIST = \
	$(plugin_in_files)


-include $(top_srcdir)/git.mk
