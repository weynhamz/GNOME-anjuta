
if BUILD_SVN

# Plugin glade file
subversion_gladedir = $(anjuta_glade_dir)
subversion_glade_DATA = anjuta-subversion.glade

subversion_pixmapsdir = $(anjuta_image_dir)
subversion_pixmaps_DATA = \
	anjuta-subversion.png

subversion_uidir = $(anjuta_ui_dir)
subversion_ui_DATA = anjuta-subversion.ui

plugin_in_files = anjuta-subversion.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

subversion_plugindir = $(anjuta_plugin_dir)
subversion_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

INCLUDES= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(SVN_CFLAGS) \
	$(SVN_INCLUDE)\
	$(PCRE_CFLAGS)

plugindir = $(anjuta_plugin_dir)

plugin_LTLIBRARIES = libanjuta-subversion.la

libanjuta_subversion_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_subversion_la_LIBADD = \
	$(GLADE_LIBS) $(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS) $(PCRE_LIBS) $(SVN_LIB)

libanjuta_subversion_la_SOURCES = \
	svn-backend.c \
	svn-backend.h \
	svn-notify.c \
	svn-notify.h \
	plugin.c \
	plugin.h \
	subversion-actions.c \
	subversion-actions.h \
	subversion-callbacks.c \
	subversion-callbacks.h \
	svn-thread.c \
	svn-thread.h \
	svn-backend-priv.h \
	svn-backend.h \
	svn-auth.c \
	svn-auth.h

endif

EXTRA_DIST = \
	$(plugin_in_files) \
	$(subversion_plugin_DATA) \
	$(subversion_ui_DATA) \
	$(subversion_glade_DATA) \
	$(subversion_pixmaps_DATA)
