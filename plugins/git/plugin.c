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
#include "git-diff-dialog.h"
#include "git-commit-dialog.h"
#include "git-add-dialog.h"
#include "git-remove-dialog.h"
#include "git-resolve-dialog.h"
#include "git-merge-dialog.h"
#include "git-switch-dialog.h"
#include "git-create-branch-dialog.h"
#include "git-delete-branch-dialog.h"
#include "git-unstage-dialog.h"
#include "git-checkout-files-dialog.h"
#include "git-log-dialog.h"
#include "git-create-tag-dialog.h"
#include "git-reset-dialog.h"
#include "git-revert-dialog.h"
#include "git-fetch-dialog.h"
#include "git-rebase-dialog.h"
#include "git-bisect-dialog.h"
#include "git-ignore-dialog.h"
#include "git-add-remote-dialog.h"
#include "git-delete-remote-dialog.h"
#include "git-create-patch-series-dialog.h"
#include "git-pull-dialog.h"
#include "git-cat-file-menu.h"
#include "git-push-dialog.h"
#include "git-apply-mailbox-dialog.h"
#include "git-cherry-pick-dialog.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-git.xml"

static gpointer parent_class;

static GtkActionEntry actions_git[] = 
{
	{
		"ActionMenuGit",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Git"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitCommit",                       /* Action name */
		GTK_STOCK_YES,                            /* Stock icon, if any */
		N_("_Commit..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_commit)    /* action callback */
	},
	{
		"ActionGitFetch",                       /* Action name */
		GTK_STOCK_CONNECT,                            /* Stock icon, if any */
		N_("_Fetch"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_fetch)    /* action callback */
	},
	{
		"ActionMenuGitRebase",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Rebase"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitRebaseStart",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Start..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_rebase_start)    /* action callback */
	},
	{
		"ActionGitRebaseContinue",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Continue"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_rebase_continue)    /* action callback */
	},
	{
		"ActionGitRebaseSkip",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Skip"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_rebase_skip)    /* action callback */
	},
	{
		"ActionGitRebaseAbort",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Abort"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_rebase_abort)    /* action callback */
	},
		{
		"ActionGitPush",                       /* Action name */
		GTK_STOCK_GO_FORWARD,                            /* Stock icon, if any */
		N_("_Push..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_push)    /* action callback */
	},
	{
		"ActionGitPull",                       /* Action name */
		GTK_STOCK_GO_BACK,                            /* Stock icon, if any */
		N_("_Pull..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_pull)    /* action callback */
	},
	{
		"ActionGitDiffUncommitted",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_Diff uncommitted changes"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_diff)    /* action callback */
	},
	{
		"ActionGitAdd",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Add..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_add)    /* action callback */
	},
	{
		"ActionGitRemove",                       /* Action name */
		GTK_STOCK_REMOVE,                            /* Stock icon, if any */
		N_("_Remove..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_remove)    /* action callback */
	},
	{
		"ActionGitIgnore",                       /* Action name */
		GTK_STOCK_DIALOG_ERROR,                            /* Stock icon, if any */
		N_("_Ignore..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_ignore)    /* action callback */
	},
	{
		"ActionGitCheckoutFiles",                       /* Action name */
		GTK_STOCK_UNDO,                            /* Stock icon, if any */
		N_("_Check out files..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_checkout_files)    /* action callback */
	},
	{
		"ActionGitUnstageFiles",                       /* Action name */
		GTK_STOCK_CANCEL,                            /* Stock icon, if any */
		N_("_Unstage files..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_unstage)    /* action callback */
	},
	{
		"ActionGitResolve",                       /* Action name */
		GTK_STOCK_PREFERENCES,                            /* Stock icon, if any */
		N_("_Resolve conflicts..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_resolve)    /* action callback */
	},
	{
		"ActionMenuGitPatches",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("Patches"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitCreatePatchSeries",                       /* Action name */
		GTK_STOCK_DND_MULTIPLE,                            /* Stock icon, if any */
		N_("Create patch series..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_create_patch_series)    /* action callback */
	},
	{
		"ActionMenuGitApplyMailboxFiles",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("Apply mailbox files"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitApplyMailboxApply",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Apply..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_apply_mailbox_apply)    /* action callback */
	},
	{
		"ActionGitApplyMailboxContinue",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Continue with resolved conflicts"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_apply_mailbox_resolved)    /* action callback */
	},
	{
		"ActionGitApplyMailboxSkip",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Skip current patch"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_apply_mailbox_skip)    /* action callback */
	},
	{
		"ActionGitApplyMailboxAbort",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Abort"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_apply_mailbox_abort)    /* action callback */
	},
	{
		"ActionMenuGitBranches",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Branches"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitCreateBranch",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Create branch..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_create_branch)    /* action callback */
	},
	{
		"ActionGitDeleteBranch",                       /* Action name */
		GTK_STOCK_DELETE,                            /* Stock icon, if any */
		N_("_Delete branch..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_delete_branch)    /* action callback */
	},
	{
		"ActionGitSwitch",                       /* Action name */
		GTK_STOCK_JUMP_TO,                            /* Stock icon, if any */
		N_("_Switch to another branch..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_switch)    /* action callback */
	},
	{
		"ActionGitMerge",                       /* Action name */
		GTK_STOCK_CONVERT,                            /* Stock icon, if any */
		N_("_Merge..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_merge)    /* action callback */
	},
	{
		"ActionGitCherryPick",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Cherry pick..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_cherry_pick)    /* action callback */
	},
	{
		"ActionMenuGitRemoteBranches",                       /* Action name */
		GTK_STOCK_NETWORK,                            /* Stock icon, if any */
		N_("_Remote branches"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitRemoteAdd",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Add..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_add_remote)    /* action callback */
	},
	{
		"ActionGitRemoteDelete",                       /* Action name */
		GTK_STOCK_DELETE,                            /* Stock icon, if any */
		N_("_Delete..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_delete_remote)    /* action callback */
	},
	{
		"ActionGitCreateTag",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Create tag..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_create_tag)    /* action callback */
	},
	{
		"ActionGitReset",                       /* Action name */
		GTK_STOCK_REFRESH,                            /* Stock icon, if any */
		N_("_Reset tree..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_reset)    /* action callback */
	},
	{
		"ActionGitRevert",                       /* Action name */
		GTK_STOCK_UNDO,                            /* Stock icon, if any */
		N_("_Revert commit..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_revert)    /* action callback */
	},
	{
		"ActionMenuGitBisect",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("Bisect"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitBisectStart",                       /* Action name */
		GTK_STOCK_MEDIA_PLAY,                            /* Stock icon, if any */
		N_("_Start..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_bisect_start)    /* action callback */
	},
	{
		"ActionGitBisectReset",                       /* Action name */
		GTK_STOCK_REFRESH,                            /* Stock icon, if any */
		N_("_Reset"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_bisect_reset)    /* action callback */
	},
	{
		"ActionGitBisectGood",                       /* Action name */
		GTK_STOCK_YES,                            /* Stock icon, if any */
		N_("_Good"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_bisect_good)    /* action callback */
	},
	{
		"ActionGitBisectBad",                       /* Action name */
		GTK_STOCK_NO,                            /* Stock icon, if any */
		N_("_Bad"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_bisect_bad)    /* action callback */
	},
	{
		"ActionGitLog",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_View log..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_menu_git_log)    /* action callback */
	}
};

static GtkActionEntry actions_log[] =
{
	{
		"ActionGitLogCommitDiff",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_Show commit diff"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_commit_diff)    /* action callback */
	},
	{
		"ActionGitLogViewRevision",                       /* Action name */
		GTK_STOCK_FIND,                            /* Stock icon, if any */
		N_("_View selected revision"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_cat_file)    /* action callback */
	},
	{
		"ActionGitLogCreateBranch",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Create branch..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_create_branch)    /* action callback */
	},
	{
		"ActionGitLogCreateTag",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Create tag..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_create_tag)    /* action callback */
	},
	{
		"ActionGitLogReset",                       /* Action name */
		GTK_STOCK_REFRESH,                            /* Stock icon, if any */
		N_("_Reset tree..."),               /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_reset)    /* action callback */
	},
	{
		"ActionGitLogRevert",                       /* Action name */
		GTK_STOCK_UNDO,                            /* Stock icon, if any */
		N_("_Revert commit..."),               /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_revert)    /* action callback */
	},
	{
		"ActionGitLogCherryPick",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Cherry pick..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_cherry_pick)    /* action callback */
	},
	{
		"ActionMenuGitLogBisect",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Bisect"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitLogBisectGood",                       /* Action name */
		GTK_STOCK_YES,                            /* Stock icon, if any */
		N_("_Set good revision"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_bisect_good)    /* action callback */
	},
	{
		"ActionGitLogBisectBad",                       /* Action name */
		GTK_STOCK_NO,                            /* Stock icon, if any */
		N_("_Set bad revision"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_log_menu_git_bisect_bad)    /* action callback */
	}
};

static GtkActionEntry actions_fm[] =
{
	{
		"ActionMenuGitFM",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Git"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL    /* action callback */
	},
	{
		"ActionGitFMLog",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_View log..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_fm_git_log)    /* action callback */
	},
	{
		"ActionGitFMAdd",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Add..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_fm_git_add)    /* action callback */
	},
	{
		"ActionGitFMRemove",                       /* Action name */
		GTK_STOCK_REMOVE,                            /* Stock icon, if any */
		N_("_Remove..."),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		G_CALLBACK (on_fm_git_remove)    /* action callback */
	}
};

static void
on_project_root_added (AnjutaPlugin *plugin, const gchar *name, 
					   const GValue *value, gpointer user_data)
{
	Git *git_plugin;
	gchar *project_root_uri;
	GFile *file;
	AnjutaUI *ui;
	GtkAction *git_menu_action;
	GtkAction *git_fm_menu_action;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	
	g_free (git_plugin->project_root_directory);
	project_root_uri = g_value_dup_string (value);
	file = g_file_new_for_uri (project_root_uri);
	git_plugin->project_root_directory = g_file_get_path (file);
	g_object_unref (file);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	git_menu_action = anjuta_ui_get_action (ui, 
											"ActionGroupGit",
											"ActionMenuGit");
	git_fm_menu_action = anjuta_ui_get_action (ui, 
											   "ActionGroupGitFM",
											   "ActionMenuGitFM");
	
	gtk_action_set_sensitive (git_menu_action, TRUE);
	gtk_action_set_sensitive (git_fm_menu_action, TRUE);
	gtk_widget_set_sensitive (git_plugin->log_viewer, TRUE);
	
	g_free (project_root_uri);
	
	git_plugin->bisect_file_monitor = bisect_menus_init (git_plugin);
	git_plugin->log_refresh_monitor = git_log_setup_refresh_monitor (git_plugin);
	git_log_refresh_branches (git_plugin);
}

static void
on_project_root_removed (AnjutaPlugin *plugin, const gchar *name, 
						 gpointer user_data)
{
	AnjutaUI *ui;
	GtkAction *git_menu_action;
	GtkAction *git_fm_menu_action;
	Git *git_plugin;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	
	g_free (git_plugin->project_root_directory);
	git_plugin->project_root_directory = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	git_menu_action = anjuta_ui_get_action (ui, 
											"ActionGroupGit",
											"ActionMenuGit");
	git_fm_menu_action = anjuta_ui_get_action (ui, 
												"ActionGroupGitFM",
												"ActionMenuGitFM");
	
	gtk_action_set_sensitive (git_menu_action, FALSE);
	gtk_action_set_sensitive (git_fm_menu_action, FALSE);
	gtk_widget_set_sensitive (git_plugin->log_viewer, FALSE);
	git_log_window_clear (git_plugin);
	
	g_file_monitor_cancel (git_plugin->bisect_file_monitor);
	g_file_monitor_cancel (git_plugin->log_refresh_monitor);
	g_object_unref (git_plugin->bisect_file_monitor);
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

static void
on_fm_file_added (AnjutaPlugin *plugin, const char *name,
				  const GValue *value, gpointer data)
{
	Git *git_plugin;
	GFile *file;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	
	g_free (git_plugin->current_fm_filename);
	
	file = G_FILE (g_value_get_object (value));
	git_plugin->current_fm_filename = g_file_get_path (file);
}

static void
on_fm_file_removed (AnjutaPlugin *plugin, const char *name, gpointer data)
{
	Git *git_plugin;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	
	g_free (git_plugin->current_fm_filename);
	git_plugin->current_fm_filename = NULL;
}

static gboolean
git_activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	Git *git_plugin;
	GtkAction *git_menu_action;
	GtkAction *git_fm_menu_action;
	
	DEBUG_PRINT ("%s", "Git: Activating Git plugin ...");
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Add all our actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupGit",
										_("Git operations"),
										actions_git,
										G_N_ELEMENTS (actions_git),
										GETTEXT_PACKAGE, TRUE, plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupGitLog",
										_("Git log operations"),
										actions_log,
										G_N_ELEMENTS (actions_log),
										GETTEXT_PACKAGE, TRUE, plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupGitFM",
										_("Git FM operations"),
										actions_fm,
										G_N_ELEMENTS (actions_fm),
										GETTEXT_PACKAGE, TRUE, plugin);
										
	git_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	git_plugin->log_popup_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
															"/PopupLog");
	
	/* Log viewer */
	git_plugin->log_viewer = git_log_window_create (git_plugin);
	anjuta_shell_add_widget (plugin->shell,
							 git_plugin->log_viewer,
							 "GitLogViewer",
							 _("Git Log"),
							 GTK_STOCK_ZOOM_100,
							 ANJUTA_SHELL_PLACEMENT_CENTER,
							 NULL);
	g_object_unref (git_plugin->log_viewer);
	
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
	
	git_plugin->fm_watch_id = anjuta_plugin_add_watch (plugin,
													   IANJUTA_FILE_MANAGER_SELECTED_FILE,
													   on_fm_file_added,
													   on_fm_file_removed,
													   NULL);
	
	
	
	/* Git needs a working directory to work with; it can't take full paths,
	 * so make sure that Git can't be used if there's no project opened. */
	git_menu_action = anjuta_ui_get_action (anjuta_shell_get_ui (plugin->shell, 
																 NULL), 
											"ActionGroupGit",
											"ActionMenuGit");
	git_fm_menu_action = anjuta_ui_get_action (anjuta_shell_get_ui (plugin->shell, 
																	NULL), 
											   "ActionGroupGitFM",
											   "ActionMenuGitFM");
	
	if (!git_plugin->project_root_directory)
	{
		gtk_action_set_sensitive (git_menu_action, FALSE);
		gtk_action_set_sensitive (git_fm_menu_action, FALSE);
		gtk_widget_set_sensitive (git_plugin->log_viewer, FALSE); 
	}
	
	return TRUE;
}

static gboolean
git_deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	Git *git_plugin;
	
	git_plugin = ANJUTA_PLUGIN_GIT (plugin);
	
	DEBUG_PRINT ("%s", "Git: Dectivating Git plugin ...");
	anjuta_ui_unmerge (ui, git_plugin->uiid);
	anjuta_plugin_remove_watch (plugin, git_plugin->project_root_watch_id, 
								TRUE);
	anjuta_plugin_remove_watch (plugin, git_plugin->editor_watch_id,
								TRUE);
	anjuta_plugin_remove_watch (plugin, git_plugin->fm_watch_id,
								TRUE);
	
	g_free (git_plugin->project_root_directory);
	g_free (git_plugin->current_editor_filename);
	g_free (git_plugin->current_fm_filename);
	
	anjuta_shell_remove_widget (plugin->shell, git_plugin->log_viewer, NULL);
	gtk_widget_destroy (git_plugin->log_popup_menu);
	
	return TRUE;
}

static void
git_finalize (GObject *obj)
{
	/* Finalization codes here */
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
	plugin->uiid = 0;
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
