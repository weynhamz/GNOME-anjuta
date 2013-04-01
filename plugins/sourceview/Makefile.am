# Plugin glade file
sourceview_xmldir = $(anjuta_glade_dir)
dist_sourceview_xml_DATA = anjuta-editor-sourceview.ui

# Plugin UI file
sourceview_uidir = $(anjuta_ui_dir)
dist_sourceview_ui_DATA = anjuta-sourceview.xml

# Plugin Icon file
sourceview_pixmapsdir = $(anjuta_image_dir)
dist_sourceview_pixmaps_DATA = \
	anjuta-editor-sourceview-plugin-48.png \
	anjuta-linemark-16.png \
	anjuta-pcmark-16.png \
	anjuta-breakpoint-disabled-16.png \
	anjuta-breakpoint-enabled-16.png \
	anjuta-bookmark-16.png \
	anjuta-message-16.png

# Plugin description file
plugin_in_files = sourceview.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

sourceview_plugindir = $(anjuta_plugin_dir)
dist_sourceview_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(GIO_CFLAGS) \
	$(SOURCEVIEW_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DG_LOG_DOMAIN=\"sourceview\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# Marshallers
BUILT_SOURCES = 			\
	anjuta-marshal.c			\
	anjuta-marshal.h              

anjuta-marshal.h: anjuta-marshal.list
	$(AM_V_GEN)glib-genmarshal $< --header --prefix=anjuta_marshal > $@

anjuta-marshal.c: anjuta-marshal.list
	$(AM_V_GEN)echo "#include \"anjuta-marshal.h\"" > $@ && \
	glib-genmarshal $< --body --prefix=anjuta_marshal >> $@

# The plugin
plugin_LTLIBRARIES = libanjuta-sourceview.la

# Plugin sources
libanjuta_sourceview_la_SOURCES = \
	$(BUILT_SOURCES)		\
	plugin.c \
	plugin.h \
	sourceview.c \
	sourceview.h \
	anjuta-view.h \
	anjuta-view.c \
	sourceview-prefs.h \
	sourceview-prefs.c \
	sourceview-private.h \
	sourceview-cell.c \
	sourceview-cell.h \
	sourceview-print.h \
	sourceview-print.c \
	assist-tip.h \
	assist-tip.c \
	sourceview-io.h \
	sourceview-io.c \
	sourceview-provider.h \
	sourceview-provider.c

libanjuta_sourceview_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_sourceview_la_LIBADD = \
	$(GIO_LIBS) \
	$(SOURCEVIEW_LIBS) \
	$(LIBANJUTA_LIBS)

gsettings_in_file = org.gnome.anjuta.plugins.sourceview.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	anjuta-marshal.list \
	$(plugin_in_files) \
        $(gsettings_in_file)

CLEANFILES = \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
