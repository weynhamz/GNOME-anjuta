# Sample Makefile for a anjuta plugin.

# Plugin glade file
git_gladedir = $(anjuta_glade_dir)
git_glade_DATA = anjuta-git.glade

# Plugin UI file
git_uidir = $(anjuta_ui_dir)
git_ui_DATA =  anjuta-git.ui

# Plugin Icon file
git_pixmapsdir = $(anjuta_image_dir)
git_pixmaps_DATA = \
	anjuta-git-plugin-48.png

# Plugin description file
plugin_in_files = git.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

git_plugindir = $(anjuta_plugin_dir)
git_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

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
	$(LIBANJUTA_CFLAGS)

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = libanjuta-git.la

# Plugin sources
libanjuta_git_la_SOURCES = \
	plugin.c \
	plugin.h \
	git-command.c \
	git-command.h \
	git-ui-utils.c \
	git-ui-utils.h \
	git-diff-command.c \
	git-diff-command.h \
	git-diff-dialog.c \
	git-diff-dialog.h \
	git-status.c \
	git-status.h \
	git-status-command.c \
	git-status-command.h \
	git-commit-command.h \
	git-commit-command.c \
	git-commit-dialog.c \
	git-commit-dialog.h \
	git-add-command.h \
	git-add-command.c \
	git-add-dialog.c \
	git-add-dialog.h \
	git-remove-command.c \
	git-remove-command.h \
	git-remove-dialog.c \
	git-remove-dialog.h \
	git-resolve-dialog.c \
	git-resolve-dialog.h \
	git-branch.h \
	git-branch.c \
	git-branch-list-command.h \
	git-branch-list-command.c \
	git-merge-command.h \
	git-merge-command.c \
	git-branch-combo-model.c \
	git-branch-combo-model.h \
	git-merge-dialog.h \
	git-merge-dialog.c \
	git-branch-checkout-command.h \
	git-branch-checkout-command.c \
	git-switch-dialog.c \
	git-switch-dialog.h \
	git-branch-create-command.h \
	git-branch-create-command.c \
	git-create-branch-dialog.c \
	git-create-branch-dialog.h \
	git-branch-delete-command.c \
	git-branch-delete-command.h \
	git-delete-branch-dialog.c \
	git-delete-branch-dialog.h \
	git-reset-files-command.c \
	git-reset-files-command.h \
	git-unstage-dialog.c \
	git-unstage-dialog.h \
	git-checkout-files-command.c \
	git-checkout-files-command.h \
	git-checkout-files-dialog.c \
	git-checkout-files-dialog.h \
	giggle-graph-renderer.c \
	giggle-graph-renderer.h \
	git-log-command.c \
	git-log-command.h \
	git-log-dialog.c \
	git-log-dialog.h \
	git-revision.c \
	git-revision.h \
	git-log-message-command.c \
	git-log-message-command.h \
	git-ref.h \
	git-ref.c \
	git-ref-command.c \
	git-ref-command.h \
	git-raw-output-command.c \
	git-raw-output-command.h \
	git-diff-tree-command.c \
	git-diff-tree-command.h \
	git-create-tag-dialog.c \
	git-create-tag-dialog.h \
	git-tag-create-command.c \
	git-tag-create-command.h \
	git-reset-dialog.c \
	git-reset-dialog.h \
	git-reset-tree-command.c \
	git-reset-tree-command.h \
	git-revert-command.c \
	git-revert-command.h \
	git-revert-dialog.c \
	git-revert-dialog.h \
	git-fetch-command.c \
	git-fetch-command.h \
	git-fetch-dialog.c \
	git-fetch-dialog.h \
	git-rebase-continue-command.c \
	git-rebase-continue-command.h \
	git-rebase-dialog.c \
	git-rebase-dialog.h \
	git-rebase-start-command.c \
	git-rebase-start-command.h \
	git-bisect-dialog.c \
	git-bisect-dialog.h \
	git-bisect-reset-command.c \
	git-bisect-reset-command.h \
	git-bisect-start-command.c \
	git-bisect-start-command.h \
	git-bisect-state-command.c \
	git-bisect-state-command.h \
	git-file-command.c \
	git-file-command.h \
	git-ignore-command.c \
	git-ignore-command.h \
	git-ignore-dialog.c \
	git-ignore-dialog.h \
	git-add-remote-dialog.c \
	git-add-remote-dialog.h \
	git-remote-add-command.c \
	git-remote-add-command.h \
	git-delete-remote-dialog.c \
	git-delete-remote-dialog.h \
	git-remote-delete-command.c \
	git-remote-delete-command.h \
	git-remote-list-command.c \
	git-remote-list-command.h \
	git-format-patch-command.h \
	git-format-patch-command.c \
	git-create-patch-series-dialog.c \
	git-create-patch-series-dialog.h \
	git-pull-command.c \
	git-pull-command.h \
	git-pull-dialog.c \
	git-pull-dialog.h \
	git-list-tree-command.c \
	git-list-tree-command.h \
	git-cat-blob-command.c \
	git-cat-blob-command.h \
	git-cat-file-menu.c \
	git-cat-file-menu.h \
	git-vcs-interface.c \
	git-vcs-interface.h \
	git-clone-command.h \
	git-clone-command.c \
	git-push-command.c \
	git-push-command.h \
	git-push-dialog.c \
	git-push-dialog.h

libanjuta_git_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

# Plugin dependencies
libanjuta_git_la_LIBADD = \
	$(GTK_LIBS) \
	$(LIBANJUTA_LIBS)

EXTRA_DIST = \
	$(plugin_in_files) \
	$(git_plugin_DATA) \
	$(git_ui_DATA) \
	$(git_glade_DATA) \
	$(git_pixmaps_DATA)

-include $(top_srcdir)/git.mk
