/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) James Liggett 2008

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#include "plugin.h"
#include "git-vcs-interface.h"
#include "git-branches-pane.h"
#include "git-create-branch-pane.h"
#include "git-delete-branches-pane.h"
#include "git-switch-branch-pane.h"
#include "git-merge-pane.h"
#include "git-status-pane.h"
#include "git-commit-pane.h"
#include "git-add-files-pane.h"
#include "git-remove-files-pane.h"
#include "git-remotes-pane.h"
#include "git-push-pane.h"
#include "git-pull-pane.h"
#include "git-checkout-pane.h"
#include "git-unstage-pane.h"
#include "git-diff-pane.h"
#include "git-add-remote-pane.h"
#include "git-delete-remote-pane.h"
#include "git-fetch-pane.h"
#include "git-resolve-conflicts-pane.h"
#include "git-tags-pane.h"
#include "git-create-tag-pane.h"
#include "git-delete-tags-pane.h"
#include "git-stash-pane.h"
#include "git-stash-changes-pane.h"
#include "git-apply-stash-pane.h"

AnjutaCommandBarEntry branch_entries[] =
{
	{
		ANJUTA_COMMAND_BAR_ENTRY_FRAME,
		"NULL",
		N_("Branch tools"),
		NULL,
		NULL,
		NULL
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"CreateBranch",
		N_("Create a branch"),
		N_("Create a branch"),
		GTK_STOCK_NEW,
		G_CALLBACK (on_create_branch_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"DeleteBranches",
		N_("Delete selected branches"),
		N_("Delete selected branches"),
		GTK_STOCK_DELETE,
		G_CALLBACK (on_delete_branches_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Switch",
		N_("Switch to this branch"),
		N_("Switch to the selected branch"),
		GTK_STOCK_JUMP_TO,
		G_CALLBACK (on_switch_branch_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Merge",
		N_("Merge"),
		N_("Merge a revision into the current branch"),
		GTK_STOCK_CONVERT,
		G_CALLBACK (on_merge_button_clicked)
	}
};

AnjutaCommandBarEntry tag_entries[] =
{
	{
		ANJUTA_COMMAND_BAR_ENTRY_FRAME,
		"NULL",
		N_("Tag tools"),
		NULL,
		NULL,
		NULL
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"CreateTag",
		N_("Create a tag"),
		N_("Create a tag"),
		GTK_STOCK_NEW,
		G_CALLBACK (on_create_tag_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"DeleteTags",
		N_("Delete selected tags"),
		N_("Delete selected tags"),
		GTK_STOCK_DELETE,
		G_CALLBACK (on_delete_tags_button_clicked)
	}
};


AnjutaCommandBarEntry status_entries[] = 
{
	{
		ANJUTA_COMMAND_BAR_ENTRY_FRAME,
		"NULL",
		N_("Changes"),
		NULL,
		NULL,
		NULL
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Commit",
		N_("Commit"),
		N_("Commit changes"),
		GTK_STOCK_APPLY,
		G_CALLBACK (on_commit_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Diff",
		N_("Diff uncommitted changes"),
		N_("Show a diff of uncommitted changes in an editor"),
		GTK_STOCK_ZOOM_100,
		G_CALLBACK (on_diff_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_FRAME,
		"NULL",
		N_("Files"),
		NULL,
		NULL,
		NULL
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"AddFiles",
		N_("Add"),
		N_("Add files to the index"),
		GTK_STOCK_ADD,
		G_CALLBACK (on_add_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"RemoveFiles",
		N_("Remove"),
		N_("Remove files from the repository"),
		GTK_STOCK_REMOVE,
		G_CALLBACK (on_remove_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Checkout",
		N_("Check out"),
		N_("Revert changes in unstaged files"),
		GTK_STOCK_UNDO,
		G_CALLBACK (on_checkout_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Unstage",
		N_("Unstage"),
		N_("Remove staged files from the index"),
		GTK_STOCK_CANCEL,
		G_CALLBACK (on_unstage_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"ResolveConflicts",
		N_("Resolve conflicts"),
		N_("Mark selected conflicted files as resolved"),
		GTK_STOCK_PREFERENCES,
		G_CALLBACK (on_resolve_conflicts_button_clicked)
	}
};

AnjutaCommandBarEntry remotes_entries[] =
{
	{
		ANJUTA_COMMAND_BAR_ENTRY_FRAME,
		"NULL",
		N_("Remote repository tools"),
		NULL,
		NULL,
		NULL
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"AddRemote",
		N_("Add a remote"),
		N_("Add a remote repository"),
		GTK_STOCK_NEW,
		G_CALLBACK (on_add_remote_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"DeleteRemote",
		N_("Delete selected remote"),
		N_("Delete a remote"),
		GTK_STOCK_DELETE,
		G_CALLBACK (on_delete_remote_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Push",
		N_("Push"),
		N_("Push changes to a remote repository"),
		GTK_STOCK_GO_FORWARD,
		G_CALLBACK (on_push_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Pull",
		N_("Pull"),
		N_("Pull changes from a remote repository"),
		GTK_STOCK_GO_BACK,
		G_CALLBACK (on_pull_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"Fetch",
		N_("Fetch"),
		N_("Fetch changes from remote repositories"),
		GTK_STOCK_CONNECT,
		G_CALLBACK (on_fetch_button_clicked)
	}
	
};

AnjutaCommandBarEntry stash_entries[] =
{
	{
		ANJUTA_COMMAND_BAR_ENTRY_FRAME,
		"NULL",
		N_("Stash tools"),
		NULL,
		NULL,
		NULL
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"StashChanges",
		N_("Stash uncommitted changes"),
		N_("Save uncommitted changes without committing them"),
		GTK_STOCK_SAVE,
		G_CALLBACK (on_stash_changes_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"ApplyStash",
		N_("Apply selected stash"),
		N_("Apply stashed changes back into the working tree"),
		GTK_STOCK_APPLY,
		G_CALLBACK (on_apply_stash_button_clicked)
	},
	{
		ANJUTA_COMMAND_BAR_ENTRY_BUTTON,
		"ApplyStashIndex",
		N_("Apply stash and restore index"),
		N_("Apply stashed changes back into the working tree and the index"),
		GTK_STOCK_OK,
		G_CALLBACK (on_apply_stash_index_button_clicked)
	}
	
};

static gpointer parent_class;

static void
on_project_root_added (AnjutaPlugin *plugin, const gchar *name, 
					   const GValue *value, gpointer user_data)
{
	Git *git_plugin;
	gchar *project_root_uri;
	GFile *file;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	
	g_free (git_plugin->project_root_directory);
	project_root_uri = g_value_dup_string (value);
	file = g_file_new_for_uri (project_root_uri);
	git_plugin->project_root_directory = g_file_get_path (file);
	g_object_unref (file);
	
	g_free (project_root_uri);

	g_object_set (G_OBJECT (git_plugin->local_branch_list_command),
	              "working-directory", git_plugin->project_root_directory, 
	              NULL);
	g_object_set (G_OBJECT (git_plugin->remote_branch_list_command),
	              "working-directory", git_plugin->project_root_directory, 
	              NULL);
	g_object_set (G_OBJECT (git_plugin->commit_status_command),
	              "working-directory", git_plugin->project_root_directory,
	              NULL);
	g_object_set (G_OBJECT (git_plugin->not_updated_status_command),
	              "working-directory", git_plugin->project_root_directory,
	              NULL);
	g_object_set (G_OBJECT (git_plugin->remote_list_command),
	              "working-directory", git_plugin->project_root_directory,
	              NULL);
	g_object_set (G_OBJECT (git_plugin->tag_list_command),
	              "working-directory", git_plugin->project_root_directory,
	              NULL);
	g_object_set (G_OBJECT (git_plugin->stash_list_command),
	              "working-directory", git_plugin->project_root_directory,
	              NULL);

	anjuta_command_start_automatic_monitor (ANJUTA_COMMAND (git_plugin->local_branch_list_command));
	anjuta_command_start_automatic_monitor (ANJUTA_COMMAND (git_plugin->commit_status_command));
	anjuta_command_start_automatic_monitor (ANJUTA_COMMAND (git_plugin->remote_list_command));
	anjuta_command_start_automatic_monitor (ANJUTA_COMMAND (git_plugin->tag_list_command));
	anjuta_command_start_automatic_monitor (ANJUTA_COMMAND (git_plugin->stash_list_command));
	anjuta_command_start (ANJUTA_COMMAND (git_plugin->local_branch_list_command));
	anjuta_command_start (ANJUTA_COMMAND (git_plugin->commit_status_command));
	anjuta_command_start (ANJUTA_COMMAND (git_plugin->remote_list_command));
	anjuta_command_start (ANJUTA_COMMAND (git_plugin->tag_list_command));
	anjuta_command_start (ANJUTA_COMMAND (git_plugin->stash_list_command));
	
	gtk_widget_set_sensitive (git_plugin->dock, TRUE);
	gtk_widget_set_sensitive (git_plugin->command_bar, TRUE);
}

static void
on_project_root_removed (AnjutaPlugin *plugin, const gchar *name, 
						 gpointer user_data)
{
	Git *git_plugin;
	AnjutaStatus *status;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	status = anjuta_shell_get_status (plugin->shell, NULL);
	
	anjuta_command_stop_automatic_monitor (ANJUTA_COMMAND (git_plugin->local_branch_list_command));
	anjuta_command_stop_automatic_monitor (ANJUTA_COMMAND (git_plugin->commit_status_command));
	
	g_free (git_plugin->project_root_directory);
	git_plugin->project_root_directory = NULL;

	gtk_widget_set_sensitive (git_plugin->dock, FALSE);
	gtk_widget_set_sensitive (git_plugin->command_bar, FALSE);

	anjuta_status_set_default (status, _("Branch"), NULL);
}

static void
on_editor_added (AnjutaPlugin *plugin, const gchar *name, const GValue *value,
				 gpointer user_data)
{
	Git *git_plugin;
	IAnjutaEditor *editor;
	GFile *current_editor_file;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	editor = g_value_get_object (value);
	
	g_free (git_plugin->current_editor_filename);	
	git_plugin->current_editor_filename = NULL;
	
	if (IANJUTA_IS_EDITOR (editor))
	{
		current_editor_file = ianjuta_file_get_file (IANJUTA_FILE (editor), 
													 NULL);

		if (current_editor_file)
		{		
			git_plugin->current_editor_filename = g_file_get_path (current_editor_file);
			g_object_unref (current_editor_file);
		}
	}
}

static void
on_editor_removed (AnjutaPlugin *plugin, const gchar *name, gpointer user_data)
{
	Git *git_plugin;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	
	g_free (git_plugin->current_editor_filename);
	git_plugin->current_editor_filename = NULL;
}

/* This function is used to run the next command in a series of commands, 
 * usually used in cases where different types of data need to be treated 
 * differently, like local vs. remote branches or staged or unstaged status
 * items. */
static void
run_next_command (AnjutaCommand *command, 
                  guint return_code, 
                  AnjutaCommand *next_command)
{
	if (return_code == 0)
		anjuta_command_start (ANJUTA_COMMAND (next_command));
}

static void
on_branch_list_command_data_arrived (AnjutaCommand *command, Git *plugin)
{
	AnjutaStatus *status;
	GList *current_branch;
	GitBranch *branch;
	gchar *name;

	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	current_branch = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command));

	while (current_branch)
	{
		branch = current_branch->data;

		if (git_branch_is_active (branch))
		{
			name = git_branch_get_name (branch);

			anjuta_status_set_default (status, _("Branch"), "%s", name);
			g_free (name);
		}

		current_branch = g_list_next (current_branch);
	}
}

static gboolean
git_activate_plugin (AnjutaPlugin *plugin)
{
	Git *git_plugin;
	GtkWidget *command_bar_viewport;
	GtkWidget *dock_viewport;
	
	DEBUG_PRINT ("%s", "Git: Activating Git plugin …");
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);

	/* Command bar and dock */
	git_plugin->command_bar = anjuta_command_bar_new ();
	git_plugin->dock = anjuta_dock_new ();
	command_bar_viewport = gtk_viewport_new (NULL, NULL);
	dock_viewport = gtk_viewport_new (NULL, NULL);
	git_plugin->command_bar_window = gtk_scrolled_window_new (NULL, NULL);
	git_plugin->dock_window = gtk_scrolled_window_new (NULL, NULL);

	gtk_container_add (GTK_CONTAINER (command_bar_viewport), 
	                   git_plugin->command_bar);
	gtk_container_add (GTK_CONTAINER (dock_viewport), git_plugin->dock);
	gtk_container_add (GTK_CONTAINER (git_plugin->command_bar_window), 
	                   command_bar_viewport);
	gtk_container_add (GTK_CONTAINER (git_plugin->dock_window), dock_viewport);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (git_plugin->command_bar_window), 
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (git_plugin->dock_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (git_plugin->command_bar_window),
	                                     GTK_SHADOW_NONE);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (git_plugin->dock_window),
	                                     GTK_SHADOW_NONE);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (command_bar_viewport), 
	                              GTK_SHADOW_NONE);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (dock_viewport), 
	                              GTK_SHADOW_NONE);
	

	anjuta_dock_set_command_bar (ANJUTA_DOCK (git_plugin->dock), 
	                             ANJUTA_COMMAND_BAR (git_plugin->command_bar));

	anjuta_shell_add_widget (plugin->shell, git_plugin->command_bar_window, 
	                         "GitCommandBar", _("Git Tasks"), NULL,
	                         ANJUTA_SHELL_PLACEMENT_LEFT, NULL);

	anjuta_shell_add_widget (plugin->shell, git_plugin->dock_window, "GitDock", 
	                         _("Git"), NULL, ANJUTA_SHELL_PLACEMENT_CENTER,
	                         NULL);

	/* Create the branch list commands. There are two commands because some 
	 * views need to be able to tell the difference between local and 
	 * remote branches */
	git_plugin->local_branch_list_command = git_branch_list_command_new (NULL,
	                                                                     GIT_BRANCH_TYPE_LOCAL);
	git_plugin->remote_branch_list_command = git_branch_list_command_new (NULL,
	                                                                      GIT_BRANCH_TYPE_REMOTE);

	g_signal_connect (G_OBJECT (git_plugin->local_branch_list_command), 
	                  "command-finished",
	                  G_CALLBACK (run_next_command),
	                  git_plugin->remote_branch_list_command);

	/* Detect the active branch and indicate it in the status bar. The active
	 * branch should never be a remote branch in practice, but it is possible to
	 * have that happen nonetheless. */
	g_signal_connect (G_OBJECT (git_plugin->local_branch_list_command),
	                  "data-arrived",
	                  G_CALLBACK (on_branch_list_command_data_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (git_plugin->remote_branch_list_command),
	                  "data-arrived",
	                  G_CALLBACK (on_branch_list_command_data_arrived),
	                  plugin);

	/* Create the status list commands. The different commands correspond to 
	 * the two different sections in status output: Changes to be committed 
	 * (staged) and Changed but not updated (unstaged.) */
	git_plugin->commit_status_command = git_status_command_new (NULL,
	                                                            GIT_STATUS_SECTION_COMMIT);
	git_plugin->not_updated_status_command = git_status_command_new (NULL,
	                                                                 GIT_STATUS_SECTION_NOT_UPDATED);

	/* Remote list command */
	git_plugin->remote_list_command = git_remote_list_command_new (NULL);

	/* Always run the not updated commmand after the commmit command. */
	g_signal_connect (G_OBJECT (git_plugin->commit_status_command), 
	                  "command-finished",
	                  G_CALLBACK (run_next_command),
	                  git_plugin->not_updated_status_command);

	/* Tag list command */
	git_plugin->tag_list_command = git_tag_list_command_new (NULL);

	/* Stash list command */
	git_plugin->stash_list_command = git_stash_list_command_new (NULL);

	/* Add the panes to the dock */
	git_plugin->status_pane = git_status_pane_new (git_plugin);
	anjuta_dock_add_pane (ANJUTA_DOCK (git_plugin->dock), "Status",
	                      _("Status"), NULL, git_plugin->status_pane,
	                      GDL_DOCK_CENTER, status_entries, 
	                      G_N_ELEMENTS (status_entries), git_plugin);
	
	git_plugin->branches_pane = git_branches_pane_new (git_plugin);
	anjuta_dock_add_pane (ANJUTA_DOCK (git_plugin->dock), "Branches", 
	                      _("Branches"), NULL, git_plugin->branches_pane,   
	                      GDL_DOCK_CENTER, branch_entries, 
	                      G_N_ELEMENTS (branch_entries), git_plugin);

	git_plugin->tags_pane = git_tags_pane_new (git_plugin);
	anjuta_dock_add_pane (ANJUTA_DOCK (git_plugin->dock), "Tags", _("Tags"),
	                      NULL, git_plugin->tags_pane, GDL_DOCK_CENTER,
	                      tag_entries, G_N_ELEMENTS (tag_entries), plugin);
	
	git_plugin->remotes_pane = git_remotes_pane_new (git_plugin);
	anjuta_dock_add_pane (ANJUTA_DOCK (git_plugin->dock), "Remotes", 
	                      _("Remotes"), NULL, git_plugin->remotes_pane,
	                      GDL_DOCK_CENTER, remotes_entries, 
	                      G_N_ELEMENTS (remotes_entries), git_plugin);

	git_plugin->stash_pane = git_stash_pane_new (git_plugin);
	anjuta_dock_add_pane (ANJUTA_DOCK (git_plugin->dock), "Stash", 
	                      _("Stash"), NULL, git_plugin->stash_pane,
	                      GDL_DOCK_CENTER, stash_entries, 
	                      G_N_ELEMENTS (stash_entries), git_plugin);

	
	/* Add watches */
	git_plugin->project_root_watch_id = anjuta_plugin_add_watch (plugin,
																 IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
																 on_project_root_added,
																 on_project_root_removed,
																 NULL);
	
	git_plugin->editor_watch_id = anjuta_plugin_add_watch (plugin,
														   IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
														   on_editor_added,
														   on_editor_removed,
														   NULL);
	
	
	/* Git needs a working directory to work with; it can't take full paths,
	 * so make sure that Git can't be used if there's no project opened. */
	if (!git_plugin->project_root_directory)
	{
		gtk_widget_set_sensitive (git_plugin->command_bar, FALSE);
		gtk_widget_set_sensitive (git_plugin->dock, FALSE);	
	}
	
	return TRUE;
}

static gboolean
git_deactivate_plugin (AnjutaPlugin *plugin)
{
	Git *git_plugin;
	AnjutaStatus *status;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	status = anjuta_shell_get_status (plugin->shell, NULL);
	
	DEBUG_PRINT ("%s", "Git: Dectivating Git plugin.\n");
	
	anjuta_plugin_remove_watch (plugin, git_plugin->project_root_watch_id, 
								TRUE);
	anjuta_plugin_remove_watch (plugin, git_plugin->editor_watch_id,
								TRUE);

	anjuta_shell_remove_widget (plugin->shell, git_plugin->command_bar_window, NULL);
	anjuta_shell_remove_widget (plugin->shell, git_plugin->dock_window, NULL);

	g_object_unref (git_plugin->local_branch_list_command);
	g_object_unref (git_plugin->remote_branch_list_command);
	g_object_unref (git_plugin->commit_status_command);
	g_object_unref (git_plugin->not_updated_status_command);
	g_object_unref (git_plugin->remote_list_command);
	g_object_unref (git_plugin->tag_list_command);
	g_object_unref (git_plugin->stash_list_command);
	
	g_free (git_plugin->project_root_directory);
	g_free (git_plugin->current_editor_filename);
	
	return TRUE;
}

static void
git_finalize (GObject *obj)
{
	Git *git_plugin;

	git_plugin = ANJUTA_PLUGIN_GIT (obj);

	g_object_unref (git_plugin->command_queue);
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
git_dispose (GObject *obj)
{
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
git_instance_init (GObject *obj)
{
	Git *plugin = ANJUTA_PLUGIN_GIT (obj);

	plugin->command_queue = anjuta_command_queue_new (ANJUTA_COMMAND_QUEUE_EXECUTE_AUTOMATIC);
}

static void
git_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = git_activate_plugin;
	plugin_class->deactivate = git_deactivate_plugin;
	klass->finalize = git_finalize;
	klass->dispose = git_dispose;
}

ANJUTA_PLUGIN_BEGIN (Git, git);
ANJUTA_PLUGIN_ADD_INTERFACE (git_ivcs, IANJUTA_TYPE_VCS);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (Git, git);
