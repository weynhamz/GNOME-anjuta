SUBDIRS = . anjuta-tags images benchmark

symbol_db_datadir = $(anjuta_data_dir)
symbol_db_data_DATA = tables.sql

# Plugin UI file
symbol_db_uidir = $(anjuta_ui_dir)
symbol_db_ui_DATA = anjuta-symbol-db-plugin.xml

# Plugin Glade file
symbol_db_gladedir = $(anjuta_glade_dir)
symbol_db_glade_DATA =  anjuta-symbol-db.ui

# Plugin Icon file
symbol_db_pixmapsdir = $(anjuta_image_dir)
symbol_db_pixmaps_DATA = \
	anjuta-symbol-db-plugin.svg \
	anjuta-symbol-db-plugin-48.png \
	anjuta-symbol-db-plugin-16.png

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
	$(GDA_CFLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	-DSYMBOL_DB_SHM=\"$(SYMBOL_DB_SHM)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DG_LOG_DOMAIN=\"libanjuta-symbol-db\"

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-symbol-db.la

# Plugin sources
libanjuta_symbol_db_la_SOURCES = \
	plugin.c \
	plugin.h \
	readtags.c \
	readtags.h \
	symbol-db-system.h \
	symbol-db-system.c \
	symbol-db-engine-utils.c \
	symbol-db-engine-utils.h \
	symbol-db-engine-priv.h \
	symbol-db-engine-core.c \
	symbol-db-engine-core.h \
	symbol-db-engine.h \
	symbol-db-query.h \
	symbol-db-query.c \
	symbol-db-query-result.h \
	symbol-db-query-result.c \
	symbol-db-model.c \
	symbol-db-model.h \
	symbol-db-model-project.c \
	symbol-db-model-project.h \
	symbol-db-model-file.c \
	symbol-db-model-file.h \
	symbol-db-model-search.c \
	symbol-db-model-search.h \
	symbol-db-views.c \
	symbol-db-views.h \
	$(BUILT_SOURCES)

libanjuta_symbol_db_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_symbol_db_la_LIBADD = \
	$(GDA_LIBS) \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

BUILT_SOURCES=symbol-db-marshal.c symbol-db-marshal.h

symbol-db-marshal.h: symbol-db-marshal.list
	$(AM_V_GEN)@GLIB_GENMARSHAL@ \
	        --prefix=symbol_db_cclosure_marshal $(srcdir)/symbol-db-marshal.list --header > xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc

symbol-db-marshal.c: symbol-db-marshal.h symbol-db-marshal.list
	$(AM_V_GEN)echo "#include \"symbol-db-marshal.h\"" > xgenc-gmc \
	&& @GLIB_GENMARSHAL@ \
	        --prefix=symbol_db_cclosure_marshal $(srcdir)/symbol-db-marshal.list --body >> xgenc-gmc \
	&& cp xgenc-gmc $(@F) \
	&& rm -f xgenc-gmc


prefs_ui_files = anjuta-symbol-db.ui
prefs_name = org.gnome.anjuta.symbol-db
include $(top_srcdir)/scripts/build-schemas.mk

EXTRA_DIST = \
	$(plugin_in_files) \
	$(symbol_db_plugin_DATA) \
	$(symbol_db_glade_DATA) \
	$(symbol_db_pixmaps_DATA) \
	$(symbol_db_data_DATA) \
	$(symbol_db_ui_DATA) \
	symbol-db-marshal.list

-include $(top_srcdir)/git.mk
