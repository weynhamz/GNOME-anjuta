# Sample Makefile for a anjuta plugin.

# Plugin glade file
git_gladedir = $(anjuta_glade_dir)
git_glade_DATA = anjuta-git.ui

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
	git-diff-command.c \
	git-diff-command.h \
	git-status.c \
	git-status.h \
	git-status-command.c \
	git-status-command.h \
	git-commit-command.h \
	git-commit-command.c \
	git-add-command.h \
	git-add-command.c \
	git-remove-command.c \
	git-remove-command.h \
	git-branch.h \
	git-branch.c \
	git-branch-list-command.h \
	git-branch-list-command.c \
	git-merge-command.h \
	git-merge-command.c \
	git-branch-checkout-command.h \
	git-branch-checkout-command.c \
	git-branch-create-command.h \
	git-branch-create-command.c \
	git-branch-delete-command.c \
	git-branch-delete-command.h \
	git-reset-files-command.c \
	git-reset-files-command.h \
	git-checkout-files-command.c \
	git-checkout-files-command.h \
	giggle-graph-renderer.c \
	giggle-graph-renderer.h \
	git-log-command.c \
	git-log-command.h \
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
	git-tag-create-command.c \
	git-tag-create-command.h \
	git-reset-tree-command.c \
	git-reset-tree-command.h \
	git-revert-command.c \
	git-revert-command.h \
	git-fetch-command.c \
	git-fetch-command.h \
	git-rebase-continue-command.c \
	git-rebase-continue-command.h \
	git-rebase-start-command.c \
	git-rebase-start-command.h \
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
	git-remote-add-command.c \
	git-remote-add-command.h \
	git-remote-delete-command.c \
	git-remote-delete-command.h \
	git-remote-list-command.c \
	git-remote-list-command.h \
	git-format-patch-command.h \
	git-format-patch-command.c \
	git-pull-command.c \
	git-pull-command.h \
	git-list-tree-command.c \
	git-list-tree-command.h \
	git-cat-blob-command.c \
	git-cat-blob-command.h \
	git-vcs-interface.c \
	git-vcs-interface.h \
	git-clone-command.h \
	git-clone-command.c \
	git-push-command.c \
	git-push-command.h \
	git-apply-mailbox-command.c \
	git-apply-mailbox-command.h \
	git-apply-mailbox-continue-command.c \
	git-apply-mailbox-continue-command.h \
	git-cherry-pick-command.c \
	git-cherry-pick-command.h \
	git-tag-delete-command.c \
	git-tag-delete-command.h \
	git-tag-list-command.c \
	git-tag-list-command.h \
	git-stash-save-command.h \
	git-stash-save-command.c \
	git-stash.c \
	git-stash.h \
	git-stash-list-command.c \
	git-stash-list-command.h \
	git-stash-apply-command.c \
	git-stash-apply-command.h \
	git-stash-show-command.c \
	git-stash-show-command.h \
	git-stash-drop-command.c \
	git-stash-drop-command.h \
	git-stash-clear-command.c \
	git-stash-clear-command.h \
	git-init-command.c \
	git-init-command.h \
	git-pane.c \
	git-pane.h \
	git-branches-pane.c \
	git-branches-pane.h \
	git-create-branch-pane.c \
	git-create-branch-pane.h \
	git-delete-branches-pane.c \
	git-delete-branch-pane.h \
	git-switch-branch-pane.c \
	git-switch-branch-pane.h \
	git-merge-pane.c \
	git-merge-pane.h \
	git-status-pane.h \
	git-status-pane.c \
	git-commit-pane.h \
	git-commit-pane.c \
	git-add-files-pane.c \
	git-add-files-pane.h \
	git-remove-files-pane.c \
	git-remove-files-pane.h \
	git-remotes-pane.c \
	git-remotes-pane.h \
	git-push-pane.c \
	git-push-pane.h \
	git-repository-selector.c \
	git-repository-selector.h \
	git-pull-pane.h \
	git-pull-pane.c \
	git-checkout-pane.c \
	git-checkout-pane.h \
	git-unstage-pane.c \
	git-unstage-pane.h \
	git-diff-pane.c \
	git-diff-pane.h \
	git-add-remote-pane.c \
	git-add-remote-pane.h \
	git-delete-remote-pane.c \
	git-delete-remote-pane.h \
	git-fetch-pane.c \
	git-fetch-pane.h \
	git-resolve-conflicts-pane.c \
	git-resolve-conflicts-pane.h \
	git-tags-pane.c \
	git-tags-pane.h \
	git-create-tag-pane.c \
	git-create-tag-pane.h \
	git-delete-tags-pane.c \
	git-delete-tags-pane.h \
	git-stash-pane.h \
	git-stash-pane.c \
	git-stash-changes-pane.c \
	git-stash-changes-pane.h \
	git-apply-stash-pane.c \
	git-apply-stash-pane.h \
	git-diff-stash-pane.c \
	git-diff-stash-pane.h \
	git-drop-stash-pane.c \
	git-drop-stash-pane.h \
	git-clear-stash-pane.c \
	git-clear-stash-pane.h \
	git-rebase-pane.c \
	git-rebase-pane.h

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
