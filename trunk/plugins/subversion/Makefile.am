
if BUILD_SVN

# Plugin glade file
subversion_gladedir = $(anjuta_glade_dir)
subversion_glade_DATA = anjuta-subversion.glade

subversion_pixmapsdir = $(anjuta_image_dir)
subversion_pixmaps_DATA = \
	anjuta-subversion-plugin.svg \
	anjuta-subversion-plugin-48.png

subversion_uidir = $(anjuta_ui_dir)
subversion_ui_DATA = anjuta-subversion.ui

plugin_in_files = anjuta-subversion.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

subversion_plugindir = $(anjuta_plugin_dir)
subversion_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

AM_CPPFLAGS= \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	$(SVN_CFLAGS) \
	$(SVN_INCLUDE)

plugindir = $(anjuta_plugin_dir)

plugin_LTLIBRARIES = libanjuta-subversion.la

libanjuta_subversion_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_subversion_la_LIBADD = \
	$(GLADE_LIBS) $(GNOME_VFS_LIBS) \
	$(LIBANJUTA_LIBS) $(SVN_LIB)

libanjuta_subversion_la_SOURCES = \
	plugin.c \
	plugin.h \
	svn-diff-command.h \
	svn-diff-command.c \
	svn-command.h \
	svn-command.c \
	svn-update-command.h \
	svn-update-command.c \
	svn-remove-command.h \
	svn-remove-command.c \
	svn-add-command.h \
	svn-add-command.c \
	svn-commit-command.h \
	svn-commit-command.c \
	svn-status.h \
	svn-status.c \
	svn-status-command.h \
	svn-status-command.c \
	subversion-ui-utils.h \
	subversion-ui-utils.c \
	subversion-revert-dialog.c \
	svn-revert-command.h \
	svn-revert-command.c \
	subversion-revert-dialog.h \
	subversion-commit-dialog.h \
	subversion-commit-dialog.c \
	subversion-update-dialog.h \
	subversion-update-dialog.c \
	subversion-diff-dialog.h \
	subversion-diff-dialog.c \
	subversion-add-dialog.h \
	subversion-add-dialog.c \
	subversion-remove-dialog.h \
	subversion-remove-dialog.c \
	svn-log-entry.h \
	svn-log-entry.c \
	svn-log-command.h \
	svn-log-command.c \
	subversion-log-dialog.h \
	subversion-log-dialog.c \
	svn-cat-command.h \
	svn-cat-command.c \
	svn-copy-command.h \
	svn-copy-command.c \
	subversion-copy-dialog.c \
	subversion-copy-dialog.h \
	svn-switch-command.h \
	svn-switch-command.c \
	subversion-switch-dialog.c \
	subversion-switch-dialog.h \
	svn-merge-command.h \
	svn-merge-command.c \
	subversion-merge-dialog.c \
	subversion-merge-dialog.h \
	subversion-resolve-dialog.c \
	subversion-resolve-dialog.h \
	svn-resolve-command.h \
	svn-resolve-command.c

endif

EXTRA_DIST = \
	$(plugin_in_files) \
	$(subversion_plugin_DATA) \
	$(subversion_ui_DATA) \
	$(subversion_glade_DATA) \
	$(subversion_pixmaps_DATA) 
