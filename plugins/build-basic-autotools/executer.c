/*
    executer.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-terminal.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#include "executer.h"

#define PREF_USE_SB "build.use_scratchbox"
#define PREF_SB_PATH "build.scratchbox.path"

static gboolean
get_program_parameters (BasicAutotoolsPlugin *plugin,
						const gchar *pre_select_uri,
						gchar **program_uri,
						gchar **program_args,
						gboolean *run_in_terminal)
{
	GladeXML *gxml;
	GtkWidget *dlg, *treeview, *use_terminal_check, *arguments_entry;
	GtkWidget *treeview_frame;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	gint response;
	GList *node;
	GtkTreeIter iter;
	GList *exec_targets;
	IAnjutaProjectManager *pm;
	gboolean success = FALSE;
	
	if (plugin->project_root_dir)
	{
		pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);
		g_return_val_if_fail (pm != NULL, FALSE);
		exec_targets =
		ianjuta_project_manager_get_targets (pm, 
							 IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE,
											 NULL);
		if (!exec_targets)
		{
			anjuta_util_dialog_error(GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
															 _("No executables in this project!"));
			return FALSE;
		}
	}
	else
	{
		exec_targets = NULL;
	}
		
	gxml = glade_xml_new (GLADE_FILE, "execute_dialog", NULL);
	dlg = glade_xml_get_widget (gxml, "execute_dialog");
	treeview = glade_xml_get_widget (gxml, "programs_treeview");
	treeview_frame = glade_xml_get_widget (gxml, "treeview_frame");
	use_terminal_check = glade_xml_get_widget (gxml, "program_run_in_terminal");
	arguments_entry = glade_xml_get_widget (gxml, "program_arguments");
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg),
								  GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_terminal_check),
								  plugin->run_in_terminal);
	if (plugin->program_args)
		gtk_entry_set_text (GTK_ENTRY (arguments_entry),
							plugin->program_args);
	
	if (g_list_length (exec_targets) > 0)
	{
		/* Populate treeview */
    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
 		gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
								 GTK_TREE_MODEL (store));
		g_object_unref (store);
		GtkTreeSelection* selection =
          gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode (selection,
									 GTK_SELECTION_BROWSE);
		node = exec_targets;
		while (node)
		{
			gchar *local_path;
			const gchar *rel_path;
		
			local_path = gnome_vfs_get_local_path_from_uri ((gchar*)node->data);
			if (local_path == NULL)
			{
				g_free (node->data);
				node = g_list_next (node);
				continue;
			}
			
			rel_path =
				(gchar*)local_path +
				 strlen (plugin->project_root_dir) + 1;
			
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, rel_path, 1,
								node->data, -1);
      
      if (plugin->last_exec_uri && g_str_equal (plugin->last_exec_uri, node->data))
      { 
				GtkTreePath* path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
        gtk_tree_selection_select_iter (selection, &iter);

				gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), path, NULL, FALSE, 0, 0);
				
				gtk_tree_path_free (path);
        g_free (plugin->last_exec_uri);
        plugin->last_exec_uri = NULL;
      }
      
			g_free (local_path);
			g_free (node->data);
			node = g_list_next (node);
		}
		g_list_free (exec_targets);
		
		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_sizing (column,
										 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_title (column, _("Program"));

		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (column, renderer, FALSE);
		gtk_tree_view_column_add_attribute (column, renderer, "text",
											0);
		gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
		gtk_tree_view_set_expander_column (GTK_TREE_VIEW (treeview), column);
                   
    if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
      gtk_tree_selection_select_iter (selection, &iter);
    }
	}
	else
	{
		gtk_widget_hide (treeview_frame);
		gtk_window_set_default_size (GTK_WINDOW (dlg), 400, -1);
	}
	
	/* Run dialog */
	gtk_dialog_set_default_response (GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  response = gtk_dialog_run (GTK_DIALOG (dlg));
	if (response == GTK_RESPONSE_OK)
	{
		GtkTreeSelection *sel;
		GtkTreeModel *model;
		gchar *target = NULL;
		
		if (exec_targets)
		{
			sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
			if (gtk_tree_selection_get_selected (sel, &model, &iter))
			{
				gtk_tree_model_get (model, &iter, 1, &target, -1);
				if (program_uri)
					*program_uri = target;
				if (run_in_terminal)
					*run_in_terminal = gtk_toggle_button_get_active (
									GTK_TOGGLE_BUTTON (use_terminal_check));
				if (program_args)
					*program_args = g_strdup (gtk_entry_get_text (
											GTK_ENTRY (arguments_entry)));

        plugin->last_exec_uri = g_strdup(target);
				success = TRUE;
			}
		}
		else
		{
			if (run_in_terminal)
				*run_in_terminal = gtk_toggle_button_get_active (
									GTK_TOGGLE_BUTTON (use_terminal_check));
			if (program_args)
				*program_args = g_strdup (gtk_entry_get_text (
												GTK_ENTRY (arguments_entry)));
			success = TRUE;
		}
	}
	gtk_widget_destroy (dlg);
	g_object_unref (gxml);
	return success;
}

void
execute_program (BasicAutotoolsPlugin* plugin, const gchar *pre_select_uri)
{
	AnjutaPreferences* prefs = anjuta_shell_get_preferences (ANJUTA_PLUGIN(plugin)->shell, NULL);
	gboolean run_in_terminal, error_condition;
	gchar *target = NULL, *args = NULL, *dir = NULL, *cmd = NULL;

	g_return_if_fail (pre_select_uri != NULL ||
					  plugin->project_root_dir != NULL ||
					  plugin->current_editor_filename != NULL);
	
	error_condition = FALSE;
	if (pre_select_uri)
	{
		target = g_strdup (pre_select_uri);
		if (!get_program_parameters (plugin, pre_select_uri,
									 NULL, &args, &run_in_terminal))
			return;
	}
	else
	{
		if (plugin->project_root_dir)
		{
			/* Get a program from project */
			if (!get_program_parameters (plugin, NULL,
										 &target, &args, &run_in_terminal))
				return;
		}
		else
		{
			/* Execute current file */
			if (!plugin->current_editor_filename)
			{
				error_condition = TRUE;
				target = NULL;
				anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
										  _("No file or project currently opened."));
			}
			else
			{
				gchar *ext;
				
				target = g_strdup (plugin->current_editor_filename);
				ext = strrchr (target, '.');
				if (ext)
				{
					*ext = '\0';
				}
			}
			if (!get_program_parameters (plugin, NULL,
										 NULL, &args, &run_in_terminal))
			{
				error_condition = TRUE;
			}
		}
	}
	
	/* Doing some checks before actualing starting */
	if (!error_condition)
	{
		/* Save states */
		if (args)
		{
			g_free (plugin->program_args);
			plugin->program_args = g_strdup (args);
		}
		plugin->run_in_terminal = run_in_terminal;
		
		/* Check if target is local */
		gchar *local_uri;
		
		local_uri = gnome_vfs_get_local_path_from_uri (target);
		if (local_uri == NULL)
		{
			error_condition = TRUE;
			anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
									  _("Program '%s' is not a local file"),
									  target);
		}
		else
		{
			g_free (target);
			target = local_uri;
		}
	}
	
	if (!error_condition &&
		g_file_test (target, G_FILE_TEST_EXISTS) == FALSE)
	{
		error_condition = TRUE;
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Program '%s' does not exists"), target);
	}
	
	if (!error_condition &&
		g_file_test (target, G_FILE_TEST_IS_EXECUTABLE) == FALSE)
	{
		error_condition = TRUE;
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Program '%s' does not have execution permission"),
								  target);
	}
	
	if (!error_condition &&
		plugin->project_root_dir == NULL &&
		pre_select_uri == NULL)
	{
		/* Check if file program is up to date */
		gchar *filename, *prog, *temp;
		gint s_re, e_re;
		struct stat s_stat, e_stat;
		IAnjutaEditor *te;
		
		anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell, "current_editor",
						  G_TYPE_OBJECT, &te, NULL);
		
		filename = gnome_vfs_get_local_path_from_uri (target);
		prog = NULL;
		prog = g_strdup (filename);
		temp = g_strrstr (prog, ".");
		if (temp)
			*(--temp) = '\0';
		s_re = stat (filename, &s_stat);
		e_re = stat (prog, &e_stat);
		g_free (prog);
		g_free (filename);
		
		if ((e_re != 0) || (s_re != 0))
		{
			error_condition = TRUE;
			anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
									  _("No executable for this file."));
		}
		else if (ianjuta_file_savable_is_dirty (IANJUTA_FILE_SAVABLE (te), NULL) ||
				 (e_stat.st_mtime < s_stat.st_mtime))
		{
			anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
										_("Executable '%s' is not up-to-date."),
										filename);
			/* But continue execution */
		}
	}
	
	if (error_condition)
	{
		g_free (target);
		g_free (args);
		return;
	}
	
	
	if (args && strlen (args) > 0)
		cmd = g_strconcat ("\"",target, "\" ", args, NULL);
	else
		cmd = g_strconcat ("\"", target, "\"", NULL);

	if (anjuta_preferences_get_int (prefs , PREF_USE_SB))
	{
		const gchar* sb_path = anjuta_preferences_get(prefs, PREF_SB_PATH);
		/* we need to skip the /scratchbox/users part, maybe could be done more clever */
		const gchar* real_dir = strstr(target, "/home");
		gchar* oldcmd = cmd;
		cmd = g_strdup_printf("%s/login -d %s \"%s\"", sb_path,
									  real_dir, oldcmd);
		g_free(oldcmd);
		dir = g_strdup(real_dir);
	}
	else
		dir = g_path_get_dirname (target);
	if (run_in_terminal)
	{
		IAnjutaTerminal *term;
		
		term = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										   IAnjutaTerminal, NULL);
		if (term)
		{
			

			if (plugin->commands[IANJUTA_BUILDABLE_COMMAND_EXECUTE])
			{
				gchar *oldcmd = cmd;

				cmd = g_strdup_printf (plugin->commands[IANJUTA_BUILDABLE_COMMAND_EXECUTE],
					oldcmd);

				g_free (oldcmd);
			} else {
				gchar* launcher_path = g_find_program_in_path("anjuta_launcher");

				if (launcher_path != NULL)
				{
					gchar* oldcmd = cmd;
				
					cmd = g_strconcat ("anjuta_launcher ", oldcmd, NULL);

					g_free (oldcmd);
					g_free (launcher_path);
				}
				else
				{
					DEBUG_PRINT("Missing anjuta_launcher");
				}
				
			}

			ianjuta_terminal_execute_command (term, dir, cmd, NULL);
		}
		else
		{
			DEBUG_PRINT ("No installed terminal plugin found");
			gnome_execute_shell (dir, cmd);
		}
	}
	else
	{
		gnome_execute_shell (dir, cmd);
	}
	
	g_free (dir);
	g_free (cmd);
	g_free (target);
	g_free (args);
}
