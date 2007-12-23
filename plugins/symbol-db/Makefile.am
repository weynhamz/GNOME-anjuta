if HAVE_PLUGIN_SYMBOL_DB

#SUBDIRS = test

# Plugin UI file
symbol_db_uidir = $(anjuta_ui_dir)
symbol_db_ui_DATA =  symbol-db.ui

symbol_db_datadir = $(anjuta_data_dir)
symbol_db_data_DATA = tables.sql


# Plugin Glade file
symbol_db_gladedir = $(anjuta_glade_dir)
symbol_db_glade_DATA =  symbol-db.glade

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
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DANJUTA_DATA_DIR=\"$(anjuta_data_dir)\" \
	-DANJUTA_PLUGIN_DIR=\"$(anjuta_plugin_dir)\" \
	-DANJUTA_IMAGE_DIR=\"$(anjuta_image_dir)\" \
	-DANJUTA_GLADE_DIR=\"$(anjuta_glade_dir)\" \
	-DANJUTA_UI_DIR=\"$(anjuta_ui_dir)\" \
	-DPACKAGE_SRC_DIR=\"$(srcdir)\" \
	$(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	$(GDL_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-symbol-db.la

libanjuta_symbol_db_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin sources
libanjuta_symbol_db_la_SOURCES = plugin.c plugin.h \
	readtags.c readtags.h \
	symbol-db-engine-iterator.c \
	symbol-db-engine-iterator.h \
	symbol-db-engine.c \
	symbol-db-engine.h symbol-db-view.h \
	symbol-db-view.c \
	symbol-db-view-locals.c \
	symbol-db-view-locals.h \
	symbol-db-view-search.h \
	symbol-db-view-search.c \
    symbol-db-engine-iterator-node.h \
	symbol-db-engine-iterator-node.c

# Plugin dependencies
libanjuta_symbol_db_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

endif

EXTRA_DIST = \
	$(plugin_in_files) \
	$(symbol_db_plugin_DATA) \
	$(symbol_db_ui_DATA) \
	$(symbol_db_glade_DATA) \
	$(symbol_db_pixmaps_DATA)













