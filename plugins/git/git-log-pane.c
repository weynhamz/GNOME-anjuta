/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-log-pane.h"

enum
{
	LOG_COL_REVISION
};

enum
{
	LOADING_COL_PULSE,
	LOADING_COL_INDICATOR
};

/* Loading view modes */
typedef enum
{
	/* Show the real log viewer */
	LOG_VIEW_NORMAL,

	/* Show the loading view */
	LOG_VIEW_LOADING
} LogViewMode;

enum
{
	BRANCH_COL_ACTIVE,
	BRANCH_COL_ACTIVE_ICON,
	BRANCH_COL_NAME
};

/* DnD source targets */
static GtkTargetEntry drag_source_targets[] =
{
	{
		"STRING",
		0,
		0
	}
};

/* DnD target targets */
static GtkTargetEntry drag_target_targets[] =
{
	{
		"text/uri-list",
		0,
		0
	}
};

struct _GitLogPanePriv
{
	GtkBuilder *builder;
	GtkListStore *log_model;
	GtkCellRenderer *graph_renderer;
	GHashTable *refs;
	gchar *path;

	/* This table maps branch names and iters in the branch combo model. When
	 * branches get refreshed, use this to make sure that the same branch the
	 * user was looking at stays selected, unless that branch no longer exists.
	 * In that case the new active branch is selected */
	GHashTable *branches_table;
	gchar *selected_branch;
	gboolean viewing_active_branch;
	GtkTreeRowReference *active_branch_ref;

	/* Loading spinner data */
	guint current_spin_count;
	guint spin_cycle_steps;
	guint spin_cycle_duration;
	gint spin_timer_id;
	GtkListStore *log_loading_model;
	GtkTreeIter spinner_iter;
};

G_DEFINE_TYPE (GitLogPane, git_log_pane, GIT_TYPE_PANE);

static void
on_branch_list_command_started (AnjutaCommand *command, 
                                GitLogPane *self)
{
	GtkComboBox *branch_combo;
	GtkListStore *log_branch_combo_model;

	branch_combo = GTK_COMBO_BOX (gtk_builder_get_object (self->priv->builder,
	                                                       "branch_combo"));
	log_branch_combo_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                                 "log_branch_combo_model"));

	gtk_combo_box_set_model (branch_combo, NULL);
	gtk_list_store_clear (log_branch_combo_model);

	g_hash_table_remove_all (self->priv->branches_table);
}

static void
on_branch_list_command_finished (AnjutaCommand *command,
                                 guint return_code,
                                 GitLogPane *self)
{
	GtkComboBox *branch_combo;
	GtkTreeModel *log_branch_combo_model;
	GtkTreeIter *iter;

	branch_combo = GTK_COMBO_BOX (gtk_builder_get_object (self->priv->builder,
	                                                          "branch_combo"));
	log_branch_combo_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                                 "log_branch_combo_model"));

	gtk_combo_box_set_model (branch_combo, log_branch_combo_model);

	/* If the user was viewing the active branch, switch to the newly active
	 * branch if it changes. If another branch was being viewed, stay on that
	 * one */
	if ((!self->priv->viewing_active_branch) && 
	    (self->priv->selected_branch && 
	    g_hash_table_lookup_extended (self->priv->branches_table, 
	                                  self->priv->selected_branch, NULL, 
	                                  (gpointer) &iter)))
	{
		gtk_combo_box_set_active_iter (branch_combo, iter);
	}
	else
	{
		if (gtk_tree_row_reference_valid (self->priv->active_branch_ref)) 
		{
			GtkTreePath *path = gtk_tree_row_reference_get_path (self->priv->active_branch_ref);
			gtk_tree_model_get_iter (log_branch_combo_model, iter, path);
			gtk_combo_box_set_active_iter (branch_combo, iter);
			gtk_tree_path_free (path);
		}
	}
	
}

static void
on_branch_list_command_data_arrived (AnjutaCommand *command,
                                     GitLogPane *self)
{
	GtkListStore *log_branch_combo_model;
	AnjutaPlugin *plugin;
	GList *current_branch;
	GitBranch *branch;
	gchar *name;
	GtkTreeIter iter;

	log_branch_combo_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                                 "log_branch_combo_model"));
	plugin = anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self));
	current_branch = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command));

	while (current_branch)
	{
		branch = current_branch->data;
		name = git_branch_get_name (branch);

		gtk_list_store_append (log_branch_combo_model, &iter);

		if (git_branch_is_active (branch))
		{
			GtkTreePath *path;
			gtk_list_store_set (log_branch_combo_model, &iter,
			                    BRANCH_COL_ACTIVE, TRUE,
			                    BRANCH_COL_ACTIVE_ICON, GTK_STOCK_APPLY,
			                    -1);

			path = gtk_tree_model_get_path (GTK_TREE_MODEL (log_branch_combo_model), &iter);
			gtk_tree_row_reference_free (self->priv->active_branch_ref);
			self->priv->active_branch_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (log_branch_combo_model), path);
			gtk_tree_path_free (path);
		}
		else
		{
			gtk_list_store_set (log_branch_combo_model, &iter,
			                    BRANCH_COL_ACTIVE, FALSE,
			                    BRANCH_COL_ACTIVE_ICON, NULL,
			                    -1);
		}

		gtk_list_store_set (log_branch_combo_model, &iter, 
		                    BRANCH_COL_NAME, name, 
		                    -1);
		g_hash_table_insert (self->priv->branches_table, g_strdup (name), 
		                     g_memdup (&iter, sizeof (GtkTreeIter)));

		g_free (name);
		
		current_branch = g_list_next (current_branch);
	}
	
}

static gboolean
on_spinner_timeout (GitLogPane *self)
{
	if (self->priv->current_spin_count == self->priv->spin_cycle_steps)
		self->priv->current_spin_count = 0;
	else
		self->priv->current_spin_count++;

	gtk_list_store_set (self->priv->log_loading_model,
	                    &(self->priv->spinner_iter),
	                    LOADING_COL_PULSE,
	                    self->priv->current_spin_count,
	                    -1);
	return TRUE;
}

static void
git_log_pane_set_view_mode (GitLogPane *self, LogViewMode mode)
{
	GtkNotebook *loading_notebook;

	loading_notebook = GTK_NOTEBOOK (gtk_builder_get_object (self->priv->builder,
	                                                         "loading_notebook"));

	switch (mode)
	{
		case LOG_VIEW_LOADING:
			/* Don't create more than one timer */
			if (self->priv->spin_timer_id <= 0)
			{
				self->priv->spin_timer_id = g_timeout_add ((guint) self->priv->spin_cycle_duration / self->priv->spin_cycle_steps,
				                                           (GSourceFunc) on_spinner_timeout,
				                                           self);
			}
			
			break;
		case LOG_VIEW_NORMAL:
			if (self->priv->spin_timer_id > 0)
			{
				g_source_remove (self->priv->spin_timer_id);
				self->priv->spin_timer_id = 0;
			}

			/* Reset the spinner */
			self->priv->current_spin_count = 0;

			gtk_list_store_set (self->priv->log_loading_model,
			                    &(self->priv->spinner_iter), 
			                    LOADING_COL_PULSE, 0,
			                    -1);
			break;
		default:
			break;
	}

	gtk_notebook_set_current_page (loading_notebook, mode);
}

static void
on_log_command_finished (AnjutaCommand *command, guint return_code, 
						 GitLogPane *self)
{
	GtkTreeView *log_view;
	GQueue *queue;
	GtkTreeIter iter;
	GitRevision *revision;

	/* Show the actual log view */
	git_log_pane_set_view_mode (self, LOG_VIEW_NORMAL);

	log_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder, 
	                                                  "log_view"));
	
	if (return_code != 0)
	{
		/* Don't report erros in the log view as this is usually no user requested
		 * operation and thus error messages are confusing instead just show an
		 * empty log.
		 */
#if 0
		git_pane_report_errors (command, return_code,
		                        ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self))));
#endif
		g_object_ref (self->priv->log_model);
		gtk_tree_view_set_model (GTK_TREE_VIEW (log_view), NULL);
		g_object_unref (command);
		
		return;
	}
	
	
	g_object_ref (self->priv->log_model);
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_view), NULL);
	
	queue = git_log_command_get_output_queue (GIT_LOG_COMMAND (command));
	
	while (g_queue_peek_head (queue))
	{
		revision = g_queue_pop_head (queue);
		
		gtk_list_store_append (self->priv->log_model, &iter);
		gtk_list_store_set (self->priv->log_model, &iter, LOG_COL_REVISION, 
		                    revision, -1);

		g_object_unref (revision);
	}
	
	giggle_graph_renderer_validate_model (GIGGLE_GRAPH_RENDERER (self->priv->graph_renderer),
										  GTK_TREE_MODEL (self->priv->log_model),
										  0);
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_view), 
							 GTK_TREE_MODEL (self->priv->log_model));
	g_object_unref (self->priv->log_model);
	
	g_object_unref (command);
}

static void
refresh_log (GitLogPane *self)
{
	Git *plugin;
	GtkTreeViewColumn *graph_column;
	GitLogCommand *log_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	graph_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                             "graph_column"));
	
	/* We don't support filters for now */
	log_command = git_log_command_new (plugin->project_root_directory,
	                                   self->priv->selected_branch,
	                                   self->priv->path,
	                                   NULL,
	                                   NULL,
	                                   NULL,
	                                   NULL,
	                                   NULL,
	                                   NULL);

	/* Hide the graph column if we're looking at the log of a path. The graph
	 * won't be correct in this case. */
	if (self->priv->path)
		gtk_tree_view_column_set_visible (graph_column, FALSE);
	else
		gtk_tree_view_column_set_visible (graph_column, TRUE);

	g_signal_connect (G_OBJECT (log_command), "command-finished",
	                  G_CALLBACK (on_log_command_finished),
	                  self);

	gtk_list_store_clear (self->priv->log_model);

	/* Show the loading spinner */
	git_log_pane_set_view_mode (self, LOG_VIEW_LOADING);

	anjuta_command_start (ANJUTA_COMMAND (log_command));
}

static void
on_ref_command_finished (AnjutaCommand *command, guint return_code,
                         GitLogPane *self)
{
	Git *plugin;
	GitBranchListCommand *branch_list_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));

	if (self->priv->refs)
		g_hash_table_unref (self->priv->refs);

	self->priv->refs = git_ref_command_get_refs (GIT_REF_COMMAND (command));

	/* Refresh the branch display after the refs get updated */
	branch_list_command = git_branch_list_command_new (plugin->project_root_directory,
	                                                   GIT_BRANCH_TYPE_ALL);

	g_signal_connect (G_OBJECT (branch_list_command), "command-started",
	                  G_CALLBACK (on_branch_list_command_started),
	                  self);

	g_signal_connect (G_OBJECT (branch_list_command), "command-finished",
	                  G_CALLBACK (on_branch_list_command_finished),
	                  self);

	g_signal_connect (G_OBJECT (branch_list_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	g_signal_connect (G_OBJECT (branch_list_command), "data-arrived",
	                  G_CALLBACK (on_branch_list_command_data_arrived),
	                  self);

	anjuta_command_start (ANJUTA_COMMAND (branch_list_command));
}

static void
on_branch_combo_changed (GtkComboBox *combo_box, GitLogPane *self)
{
	GtkTreeModel *log_branch_combo_model;
	gchar *branch;
	GtkTreeIter iter;
	gboolean active;

	log_branch_combo_model = gtk_combo_box_get_model (combo_box);

	if (gtk_combo_box_get_active_iter (combo_box, &iter))
	{
		gtk_tree_model_get (log_branch_combo_model, &iter,
		                    BRANCH_COL_ACTIVE, &active, 
							BRANCH_COL_NAME, &branch, 
							-1);

		self->priv->viewing_active_branch = active;

		g_free (self->priv->selected_branch);
		self->priv->selected_branch = g_strdup (branch);

		g_free (branch);

		/* Refresh the log after the branches are refreshed so that everything 
		 * happens in a consistent order (refs, branches, log) and that 
		 * the log only gets refreshed once. Doing the refresh here also 
		 * lets us kill two birds with one stone: we can handle refreshes and
		 * different branch selections all in one place. */
		refresh_log (self);
	}
}


static void
author_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
					  GtkTreeModel *model, GtkTreeIter *iter, 
					  gpointer user_data)
{
	GitRevision *revision;
	gchar *author;
	
	gtk_tree_model_get (model, iter, LOG_COL_REVISION, &revision, -1);
	author = git_revision_get_author (revision);
	
	g_object_unref (revision);
	
	g_object_set (renderer, "text", author, NULL);
	
	g_free (author);
}

static void
date_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
					GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GitRevision *revision;
	gchar *date;
	
	gtk_tree_model_get (model, iter, LOG_COL_REVISION, &revision, -1);
	date = git_revision_get_formatted_date (revision);
	
	g_object_unref (revision);
	
	g_object_set (renderer, "text", date, NULL);
	
	g_free (date);
}

static void
short_log_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
						 GtkTreeModel *model, GtkTreeIter *iter, 
						 gpointer user_data)
{
	GitRevision *revision;
	gchar *short_log;
	
	gtk_tree_model_get (model, iter, LOG_COL_REVISION, &revision, -1);
	short_log = git_revision_get_short_log (revision);
	
	g_object_unref (revision);
	
	g_object_set (renderer, "text", short_log, NULL);
	
	g_free (short_log);
}

static void
ref_icon_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
						GtkTreeModel *model, GtkTreeIter *iter, 
						GitLogPane *self)
{
	GitRevision *revision;
	gchar *sha;
	
	gtk_tree_model_get (model, iter, LOG_COL_REVISION, &revision, -1);
	sha = git_revision_get_sha (revision);
	
	g_object_unref (revision);
	
	if (g_hash_table_lookup_extended (self->priv->refs, sha, NULL, NULL))
		g_object_set (renderer, "stock-id", GTK_STOCK_INFO, NULL);
	else
		g_object_set (renderer, "stock-id", "", NULL);
	
	g_free (sha);
}

static gboolean
on_log_view_query_tooltip (GtkWidget *log_view, gint x, gint y,
                           gboolean keyboard_mode, GtkTooltip *tooltip,
                           GitLogPane *self)
{
	gboolean ret;
	GtkTreeViewColumn *ref_icon_column;
	gint bin_x;
	gint bin_y;
	GtkTreeViewColumn *current_column;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GitRevision *revision;
	gchar *sha;
	GList *ref_list;
	GList *current_ref;
	GString *tooltip_string;
	gchar *ref_name;
	GitRefType ref_type;
	
	ret = FALSE;
	
	ref_icon_column = gtk_tree_view_get_column (GTK_TREE_VIEW (log_view), 0);
	
	gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (log_view),
													   x, y, &bin_x, &bin_y);
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (log_view), bin_x, 
									   bin_y, &path, &current_column, NULL, 
	                                   NULL))
	{
		/* We need to be in the ref icon column */
		if (current_column == ref_icon_column)
		{
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (log_view));
			gtk_tree_model_get_iter (model, &iter, path);

			gtk_tree_model_get (model, &iter, LOG_COL_REVISION, &revision, -1);
			sha = git_revision_get_sha (revision);

			g_object_unref (revision);

			ref_list = g_hash_table_lookup (self->priv->refs, sha);
			g_free (sha);

			if (ref_list)
			{
				current_ref = ref_list;
				tooltip_string = g_string_new ("");

				while (current_ref)
				{
					ref_name = git_ref_get_name (GIT_REF (current_ref->data));
					ref_type = git_ref_get_ref_type (GIT_REF (current_ref->data));

					if (tooltip_string->len > 0)
						g_string_append (tooltip_string, "\n");

					switch (ref_type)
					{
						case GIT_REF_TYPE_BRANCH:
							g_string_append_printf (tooltip_string,
							                        _("<b>Branch:</b> %s"),
							                        ref_name );
							break;
						case GIT_REF_TYPE_TAG:
							g_string_append_printf (tooltip_string,
							                        _("<b>Tag:</b> %s"),
							                        ref_name);
							break;
						case GIT_REF_TYPE_REMOTE:
							g_string_append_printf (tooltip_string,
							                        _("<b>Remote:</b> %s"),
							                        ref_name);
							break;
						default:
							break;
					}

					g_free (ref_name);
					current_ref = g_list_next (current_ref);
				}

				gtk_tooltip_set_markup (tooltip, tooltip_string->str);
				g_string_free (tooltip_string, TRUE);

				ret = TRUE;
			}
		}

		gtk_tree_path_free (path);
	}
	
	return ret;	
}

static void
on_log_message_command_finished (AnjutaCommand *command, guint return_code,
								 GitLogPane *self)
{
	GtkWidget *log_text_view;
	GtkTextBuffer *buffer;
	gchar *log_message;
	
	log_text_view = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                    "log_text_view"));
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (log_text_view));
	log_message = git_log_message_command_get_message (GIT_LOG_MESSAGE_COMMAND (command));
	
	gtk_text_buffer_set_text (buffer, log_message, strlen (log_message));
	
	g_free (log_message);
	g_object_unref (command);
}

static gboolean
on_log_view_row_selected (GtkTreeSelection *selection, 
                          GtkTreeModel *model,
                          GtkTreePath *path, 
                          gboolean path_currently_selected,
                          GitLogPane *self)
{
	Git *plugin;
	GtkTreeIter iter;
	GitRevision *revision;
	gchar *sha;
	GitLogMessageCommand *log_message_command;
	
	if (!path_currently_selected)
	{
		plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, LOG_COL_REVISION, &revision, -1);
		sha = git_revision_get_sha (revision);
		
		log_message_command = git_log_message_command_new (plugin->project_root_directory,
														   sha);
		
		g_free (sha);
		g_object_unref (revision);
		
		g_signal_connect (G_OBJECT (log_message_command), "command-finished",
						  G_CALLBACK (on_log_message_command_finished),
						  self);
		
		anjuta_command_start (ANJUTA_COMMAND (log_message_command));
	}
	
	return TRUE;
}

static void
on_log_view_drag_data_get (GtkWidget *log_view, 
                           GdkDragContext *drag_context,
                           GtkSelectionData *data,
                           guint info, guint time,
                           GitLogPane *self)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GitRevision *revision;
	gchar *sha;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_view));

	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		gtk_tree_selection_get_selected (selection, NULL, &iter);

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->log_model), &iter, 
		                    0, &revision, -1);
		
		sha = git_revision_get_sha (revision);

		gtk_selection_data_set_text (data, sha, -1);

		g_object_unref (revision);
		g_free (sha);
	}
}

static void
on_log_pane_drag_data_received (GtkWidget *widget,
                                GdkDragContext *context, gint x, gint y,
                                GtkSelectionData *data, guint target_type,
                                guint time, GitLogPane *self)
{
	Git *plugin;
	AnjutaEntry *path_entry;
	gboolean success;
	gchar **uri_list;
	GFile *parent_file;
	GFile *file;
	gchar *path;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	path_entry = ANJUTA_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                   "path_entry"));
	success = FALSE;

	if ((data != NULL) && 
	    (gtk_selection_data_get_length (data) >= 0))
	{
		if (target_type == 0)
		{
			uri_list = gtk_selection_data_get_uris (data);
			parent_file = NULL;
			
			parent_file = g_file_new_for_path (plugin->project_root_directory);

			/* Take only the first file */
			file = g_file_new_for_uri (uri_list[0]);

			if (parent_file)
			{
				path = g_file_get_relative_path (parent_file, file);

				g_object_unref (parent_file);
			}
			else
				path = g_file_get_path (file);

			if (path)
			{
				anjuta_entry_set_text (path_entry, path);

				g_free (self->priv->path);
				self->priv->path = g_strdup (path);

				refresh_log (self);

				g_free (path);
			}
			
			success = TRUE;

			g_object_unref (file);
			g_strfreev (uri_list);
		}
	}

	/* Do not delete source data */
	gtk_drag_finish (context, success, FALSE, time);
}

static gboolean
on_log_pane_drag_drop (GtkWidget *widget, GdkDragContext *context, 
                       gint x, gint y, guint time,
                       GitLogPane *self)
{
	GdkAtom target_type;

	target_type = gtk_drag_dest_find_target (widget, context, NULL);

	if (target_type != GDK_NONE)
		gtk_drag_get_data (widget, context, target_type, time);
	else
		gtk_drag_finish (context, FALSE, FALSE, time);

	return TRUE;
}

static void
on_path_entry_icon_release (GtkEntry *entry, 
                            GtkEntryIconPosition position,
                            GdkEvent *event,
                            GitLogPane *self)
{	
	if (position == GTK_ENTRY_ICON_SECONDARY)
	{
		if (self->priv->path)
		{
			g_free (self->priv->path);
			self->priv->path = NULL;

			refresh_log (self);
		}
	}
}

static void
git_log_pane_init (GitLogPane *self)
{
	gchar *objects[] = {"log_pane",
						"log_branch_combo_model",
						"log_loading_model",
						"find_button_image",
						NULL};
	GError *error = NULL;
	GtkWidget *log_pane;
	GtkWidget *path_entry;
	GtkTreeView *log_view;
	GtkTreeViewColumn *ref_icon_column;
	GtkTreeViewColumn *graph_column;
	GtkTreeViewColumn *short_log_column;
	GtkTreeViewColumn *author_column;
	GtkTreeViewColumn *date_column;
	GtkCellRenderer *ref_icon_renderer;
	GtkCellRenderer *short_log_renderer;
	GtkCellRenderer *author_renderer;
	GtkCellRenderer *date_renderer;
	GtkTreeViewColumn *loading_spinner_column;
	GtkCellRenderer *loading_spinner_renderer;
	GtkCellRenderer *loading_indicator_renderer;
	GtkComboBox *branch_combo;
	GtkTreeSelection *selection;

	self->priv = g_new0 (GitLogPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	log_pane = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                               "log_pane"));
	path_entry = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                 "path_entry"));
	log_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                  "log_view"));
	ref_icon_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                                "ref_icon_column"));
	graph_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                             "graph_column"));
	short_log_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                                 "short_log_column"));
	author_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                              "author_column"));
	date_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                							"date_column"));
	ref_icon_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                               "ref_icon_renderer"));
	author_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                             "author_renderer"));
	date_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                           "date_renderer"));
	branch_combo = GTK_COMBO_BOX (gtk_builder_get_object (self->priv->builder,
	                                                      "branch_combo"));
	loading_spinner_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                                       "loading_spinner_column"));
	selection = gtk_tree_view_get_selection (log_view);

	/* Path entry */
	g_signal_connect (G_OBJECT (path_entry), "icon-release",
	                  G_CALLBACK (on_path_entry_icon_release),
	                  self);

	/* Set up the log model */
	self->priv->log_model = gtk_list_store_new (1, GIT_TYPE_REVISION);

	/* Ref icon column */
	gtk_tree_view_column_set_cell_data_func (ref_icon_column, ref_icon_renderer,
	                                         (GtkTreeCellDataFunc) ref_icon_cell_function,
	                                         self, NULL);


	/* Graph column */
	self->priv->graph_renderer = giggle_graph_renderer_new ();

	gtk_tree_view_column_pack_start (graph_column, self->priv->graph_renderer,
	                                 TRUE);
	gtk_tree_view_column_add_attribute (graph_column, self->priv->graph_renderer,
	                                    "revision", 0);

	/* Short log column. We have to create this render ouselves becuause Glade
	 * doesn't seem to give us to option to pack it with expand */
	short_log_renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (short_log_renderer), "ellipsize", 
	              PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_pack_start (short_log_column, short_log_renderer, 
	                                 TRUE);
	gtk_tree_view_column_set_cell_data_func (short_log_column, short_log_renderer,
	                                         (GtkTreeCellDataFunc) short_log_cell_function,
	                                         NULL, NULL);

	/* Author column */
	gtk_tree_view_column_set_cell_data_func (author_column, author_renderer,
	                                         (GtkTreeCellDataFunc) author_cell_function,
	                                         NULL, NULL);

	/* Date column */
	gtk_tree_view_column_set_cell_data_func (date_column, date_renderer,
	                                         (GtkTreeCellDataFunc) date_cell_function,
	                                         NULL, NULL);
	
	gtk_tree_view_set_model (log_view, GTK_TREE_MODEL (self->priv->log_model));

	/* Ref icon tooltip */
	g_signal_connect (G_OBJECT (log_view), "query-tooltip",
	                  G_CALLBACK (on_log_view_query_tooltip),
	                  self);

	/* Loading indicator. The loading indicator is a second tree view display
	 * that looks just like the real log display, except that it displays a 
	 * spinner renderer and the text "Loading..." in the Short Log column. */
	self->priv->log_loading_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                                        "log_loading_model"));
	loading_spinner_renderer = gtk_cell_renderer_spinner_new ();
	loading_indicator_renderer = gtk_cell_renderer_text_new ();

	g_object_set (G_OBJECT (loading_spinner_renderer), "active", TRUE, NULL);

	gtk_tree_view_column_pack_start (loading_spinner_column, 
	                                 loading_spinner_renderer, FALSE);
	gtk_tree_view_column_pack_start (loading_spinner_column, 
	                                 loading_indicator_renderer, TRUE);
	gtk_tree_view_column_add_attribute (loading_spinner_column,
	                                    loading_spinner_renderer,
	                                    "pulse", LOADING_COL_PULSE);
	gtk_tree_view_column_add_attribute (loading_spinner_column,
	                                    loading_indicator_renderer,
	                                    "text", LOADING_COL_INDICATOR);

	/* DnD source */
	gtk_tree_view_enable_model_drag_source (log_view,
	                                        GDK_BUTTON1_MASK,
	                                        drag_source_targets,
	                                        G_N_ELEMENTS (drag_source_targets),
	                                        GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (log_view), "drag-data-get",
	                  G_CALLBACK (on_log_view_drag_data_get),
	                  self);

	/* DnD target. Use this as a means of selecting a file to view the 
	 * log of. Files or folders would normally be dragged in from the file 
	 * manager, but they can come from any source that supports URI's. */
	gtk_drag_dest_set (log_pane, 
	                   GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT, 
	                   drag_target_targets,
	                   G_N_ELEMENTS (drag_target_targets), 
	                   GDK_ACTION_COPY | GDK_ACTION_MOVE);

	g_signal_connect (G_OBJECT (log_pane), "drag-data-received",
	                  G_CALLBACK (on_log_pane_drag_data_received),
	                  self);

	g_signal_connect (G_OBJECT (log_pane), "drag-drop",
	                  G_CALLBACK (on_log_pane_drag_drop),
	                  self);

	/* The loading view always has one row. Cache a copy of its iter for easy
	 * access. */
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->log_loading_model), 
	                               &(self->priv->spinner_iter));

	/* FIXME: GtkSpinner doesn't have those anymore */
	self->priv->spin_cycle_duration = 1000;
	self->priv->spin_cycle_steps =  12;
	
	g_object_set (G_OBJECT (loading_spinner_renderer), "active", TRUE, NULL);

	/* Log message display */
	gtk_tree_selection_set_select_function (selection,
	                                        (GtkTreeSelectionFunc) on_log_view_row_selected,
	                                        self, NULL);

	/* Branch handling */
	self->priv->branches_table = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                                    g_free, g_free);

	g_signal_connect (G_OBJECT (branch_combo), "changed",
	                  G_CALLBACK (on_branch_combo_changed),
	                  self);
	
}

static void
git_log_pane_finalize (GObject *object)
{
	GitLogPane *self;

	self = GIT_LOG_PANE (object);

	gtk_tree_row_reference_free (self->priv->active_branch_ref);
	g_object_unref (self->priv->builder);
	g_free (self->priv->path);
	g_hash_table_destroy (self->priv->branches_table);
	g_hash_table_unref (self->priv->refs);
	g_free (self->priv->selected_branch);
	g_free (self->priv);

	G_OBJECT_CLASS (git_log_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_log_pane_get_widget (AnjutaDockPane *pane)
{
	GitLogPane *self;

	self = GIT_LOG_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder, 
	                                           "log_pane"));
}

static void
git_log_pane_class_init (GitLogPaneClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_log_pane_finalize;
	pane_class->get_widget = git_log_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_log_pane_new (Git *plugin)
{
	GitLogPane *self;

	self = g_object_new (GIT_TYPE_LOG_PANE, "plugin", plugin, NULL);

	g_signal_connect (G_OBJECT (plugin->ref_command), "command-finished",
	                  G_CALLBACK (on_ref_command_finished),
	                  self);

	return ANJUTA_DOCK_PANE (self);
}

void
git_log_pane_set_working_directory (GitLogPane *self, 
                                    const gchar *working_directory)
{
	/* TODO: Add public function implementation here */
}

GitRevision *
git_log_pane_get_selected_revision (GitLogPane *self)
{
	GtkTreeView *log_view;
	GtkTreeSelection *selection;
	GitRevision *revision;
	GtkTreeIter iter;

	log_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                  "log_view"));
	selection = gtk_tree_view_get_selection (log_view);
	revision = NULL;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->log_model), &iter,
		                    LOG_COL_REVISION, &revision, -1);
	}

	return revision;
}
