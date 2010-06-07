if ENABLE_VALA

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# Plugin description file
plugin_in_files = anjuta-vala.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DANJUTA_DATA_DIR=\"$(anjuta_data_dir)\" \
	-DANJUTA_PLUGIN_DIR=\"$(anjuta_plugin_dir)\" \
	-DANJUTA_IMAGE_DIR=\"$(anjuta_image_dir)\" \
	-DANJUTA_GLADE_DIR=\"$(anjuta_glade_dir)\" \
	-DANJUTA_UI_DIR=\"$(anjuta_ui_dir)\" \
	$(LIBANJUTA_CFLAGS) \
	$(VALA_CFLAGS) \
	-DPACKAGE_SRC_DIR=\"$(srcdir)\" \
	-DG_LOG_DOMAIN=\"language-support-vala\" \
	--no-warn

# The plugin
plugin_LTLIBRARIES = libanjuta-language-vala.la

# Plugin sources
libanjuta_language_vala_la_VALASOURCES = locator.vala plugin.vala provider.vala report.vala
libanjuta_language_vala_la_SOURCES = plugin.vala.stamp $(libanjuta_language_vala_la_VALASOURCES:.vala=.c)


plugin.vala.stamp: $(libanjuta_vala_la_VALASOURCES)
	$(VALAC) --disable-warnings --vapidir $(srcdir) --pkg vala-1.0 --pkg libanjuta-1.0 -C $^
	@touch $@


libanjuta_language_vala_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_language_vala_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(VALA_LIBS)

BUILT_SOURCES = plugin.vala.stamp
endif # ENABLE_VALA

EXTRA_DIST = \
	$(plugin_in_files) \
	$(libanjuta_language_vala_la_VALASOURCES:.vala=.c)

-include $(top_srcdir)/git.mk

