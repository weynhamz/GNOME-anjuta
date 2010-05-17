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

	anjuta_command_start_automatic_monitor (ANJUTA_COMMAND (git_plugin->local_branch_list_command));
	anjuta_command_start (ANJUTA_COMMAND (git_plugin->local_branch_list_command));
	anjuta_command_start (ANJUTA_COMMAND (git_plugin->remote_branch_list_command));

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
	anjuta_command_stop_automatic_monitor (ANJUTA_COMMAND (git_plugin->remote_branch_list_command));
	
	g_free (git_plugin->project_root_directory);
	git_plugin->project_root_directory = NULL;

	gtk_widget_set_sensitive (git_plugin->dock, FALSE);
	gtk_widget_set_sensitive (git_plugin->command_bar, FALSE);
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

	/* Add the panes to the dock */
	git_plugin->branches_pane = git_branches_pane_new (git_plugin);
	anjuta_dock_add_pane (ANJUTA_DOCK (git_plugin->dock), "Branches", 
	                      _("Branches"), NULL, git_plugin->branches_pane,   
	                      GDL_DOCK_CENTER, branch_entries, 
	                      G_N_ELEMENTS (branch_entries), git_plugin);
	
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
