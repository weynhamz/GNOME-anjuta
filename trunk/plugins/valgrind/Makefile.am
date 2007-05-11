if HAVE_PLUGIN_VALGRIND

# Plugin UI file
valgrind_uidir = $(anjuta_ui_dir)
valgrind_ui_DATA = anjuta-valgrind.ui


# Plugin glade file
valgrind_gladedir = $(anjuta_glade_dir)
valgrind_glade_DATA = anjuta-valgrind.glade

# Plugin Icon file
valgrind_pixmapsdir = $(anjuta_image_dir)
valgrind_pixmaps_DATA = anjuta-valgrind.png \
						anjuta-valgrind-knight.png

# Plugin description file
plugin_in_files = anjuta-valgrind.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

valgrind_plugindir = $(anjuta_plugin_dir)
valgrind_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-valgrind.la

# Plugin dependencies
libanjuta_valgrind_la_LIBADD = \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_VALGRIND_LIBS)
	

# Plugin sources
libanjuta_valgrind_la_SOURCES = \
	plugin.c \
	plugin.h \
	vggeneralprefs.c \
	vggeneralprefs.h \
	vgtoolprefs.h \
	vgtoolprefs.c \
	vgdefaultview.c \
	vgdefaultview.h \
	vgrulepattern.c \
	vgrulepattern.h \
	vgsearchbar.c \
	vgsearchbar.h \
	vgmarshal.h \
	vgmarshal.c \
	vgio.c \
	vgio.h \
	vgrule-list.h \
	vgrule-list.c \
	vgrule-editor.h \
	vgrule-editor.c \
	vgrule.c \
	vgrule.h \
	vgstrpool.c \
	vgstrpool.h \
	parser.c \
	parser.h \
	vgtoolview.c \
	vgtoolview.h \
	symtab.c \
	symtab.h \
	ldd.c \
	ldd.h \
	process.c \
	process.h \
	vgerror.c \
	vgerror.h \
	list.h \
	list.c \
	vgmemcheckprefs.c \
	vgmemcheckprefs.h \
	preferences.c \
	preferences.h \
	vgcachegrindprefs.c \
	vgcachegrindprefs.h \
	vghelgrindprefs.c \
	vghelgrindprefs.h \
	vgactions.c \
	vgactions.h \
	menu-utils.h \
	menu-utils.c 

vgmarshal.h: vgmarshal.list
        ( @GLIB_GENMARSHAL@ --prefix=vg_marshal $(srcdir)/vgmarshal.list --header > vgmarshal.h.tmp \
        && mv vgmarshal.h.tmp vgmarshal.h ) \
        || ( rm -f vgmarshal.h.tmp && exit 1 )

vgmarshal.c: vgmarshal.h
        ( @GLIB_GENMARSHAL@ --prefix=vg_marshal $(srcdir)/vgmarshal.list --body > vgmarshal.c.tmp \
        && mv vgmarshal.c.tmp vgmarshal.c ) \
        || ( rm -f vgmarshal.c.tmp && exit 1 )

MARSHAL_GENERATED = \
		vgmarshal.c \
		vgmarshal.h


schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = anjuta-valgrind.schemas


EXTRA_DIST = \
	$(plugin_in_files) \
	$(valgrind_plugin_DATA) \
	$(valgrind_ui_DATA) \
	$(valgrind_glade_DATA) \
	$(valgrind_pixmaps_DATA) \
	$(valgrind_glade_DATA) \
	$(schema_DATA) \
	vgmarshal.list

install-data-local:
        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
	
noinst_PROGRAMS = \
	bugged-prg-test 

bugged_prg_test_SOURCES = \
	bugged-prg-test.c 

endif


