if HAVE_PLUGIN_SYMBOL_DB

#SUBDIRS = test

symbol_db_datadir = $(anjuta_data_dir)
symbol_db_data_DATA = tables.sql

# Plugin Glade file
symbol_db_gladedir = $(anjuta_glade_dir)
symbol_db_glade_DATA =  anjuta-symbol-db.glade

# Plugin Icon file
symbol_db_pixmapsdir = $(anjuta_image_dir)
symbol_db_pixmaps_DATA = anjuta-symbol-db-plugin-48.png

# Plugin description file
plugin_in_files = symbol-db.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

symbol_db_plugindir = $(anjuta_plugin_dir)
symbol_db_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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
	$(DEPRECATED_FLAGS) \
	$(GDL_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	-DG_LOG_DOMAIN=\"libanjuta-symbol-db\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-symbol-db.la

# Plugin sources
libanjuta_symbol_db_la_SOURCES = plugin.c plugin.h \
	readtags.c readtags.h \
	symbol-db-engine-iterator.c \
	symbol-db-engine-iterator.h \
	symbol-db-view.h \
	symbol-db-view.c \
	symbol-db-view-locals.c \
	symbol-db-view-locals.h \
	symbol-db-view-search.h \
	symbol-db-view-search.c \
    	symbol-db-engine-iterator-node.h \
	symbol-db-engine-iterator-node.c \
	symbol-db-system.h \
	symbol-db-system.c  symbol-db-prefs.h \
	symbol-db-prefs.c  symbol-db-engine-queries.c \
	symbol-db-engine-queries.h  symbol-db-engine-utils.c \
	symbol-db-engine-utils.h  symbol-db-engine-priv.h \
	symbol-db-engine-core.c  symbol-db-engine-core.h \
	symbol-db-engine.h

libanjuta_symbol_db_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_symbol_db_la_LIBADD = \
	$(GDL_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

prefs_glade_files = anjuta-symbol-db.glade
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(symbol_db_plugin_DATA) \
	$(symbol_db_glade_DATA) \
	$(symbol_db_pixmaps_DATA) \
	$(symbol_db_data_DATA)

endif




