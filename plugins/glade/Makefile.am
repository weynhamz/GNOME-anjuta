
if HAVE_PLUGIN_GLADE

# Plugin glade file
glade_gladedir = $(anjuta_glade_dir)
glade_glade_DATA = anjuta-glade.glade

# Plugin UI file
glade_uidir = $(anjuta_ui_dir)
glade_ui_DATA = anjuta-glade.ui

# Plugin Icon file
glade_pixmapsdir = $(anjuta_image_dir)
glade_pixmaps_DATA = \
	anjuta-glade-plugin.svg \
	anjuta-glade-plugin-48.png

# Plugin description file
plugin_in_files = anjuta-glade.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

glade_plugindir = $(anjuta_plugin_dir)
glade_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# Marshallers
marshallers_SOURCES = \
	anjuta-glade-marshallers.c \
	anjuta-glade-marshallers.h

anjuta-glade-marshallers.h: anjuta-glade-marshallers.list
	glib-genmarshal anjuta-glade-marshallers.list --header --prefix=anjuta_glade_marshallers > $@

anjuta-glade-marshallers.c: anjuta-glade-marshallers.list anjuta-glade-marshallers.h
	echo "#include \"anjuta-glade-marshallers.h\"" > $@ && \
	glib-genmarshal anjuta-glade-marshallers.list --body --prefix=anjuta_glade_marshallers >> $@

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(PLUGIN_GLADE_CFLAGS) \
	$(LIBXML_CFLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-glade.la

# Plugin sources
libanjuta_glade_la_SOURCES = \
	$(marshallers_SOURCES) \
	plugin.c plugin.h \
	anjuta-design-document.h \
	anjuta-design-document.c \
	designer-associations.h \
	designer-associations.c \
	designer-associations-item.h \
	designer-associations-item.c

libanjuta_glade_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_glade_la_LIBADD = \
	$(PLUGIN_GLADE_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(LIBXML_LIBS)
endif

EXTRA_DIST = \
	anjuta-glade-marshallers.list \
	$(plugin_in_files) \
	$(glade_plugin_DATA) \
	$(glade_ui_DATA) \
	$(glade_pixmaps_DATA) \
	$(glade_glade_DATA)


-include $(top_srcdir)/git.mk
