# Sample Makefile for a anjuta plugin.

# Plugin UI file
anjuta_uidir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/ui
anjuta_ui_DATA =  anjuta-default-profile.ui

# Plugin Icon file
plugin_icondir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@
plugin_icon_DATA = anjuta-default-profile-plugin.png

# Plugin description file
plugin_in_files = anjuta-default-profile.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; \
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u \
		-c $(top_builddir)/po/.intltool-merge-cache

anjuta_plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@
anjuta_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
INCLUDES = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@

# The plugin
plugin_LTLIBRARIES = libanjuta-default-profile.la

# Plugin sources
libanjuta_default_profile_la_SOURCES = plugin.c

# Plugin dependencies

EXTRA_DIST = \
	(plugin_in_files) \
	$(anjuta_plugin_DATA) \
	$(anjuta_ui_DATA)
