/*
    attach_process.c
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "properties.h"
#include "resources.h"
#include "attach_process.h"
#include "global.h"
#include "anjuta.h"
#include "debugger.h"

enum
{
	CLEAR_INITIAL,
	CLEAR_UPDATE,
	CLEAR_REVIEW,
	CLEAR_FINAL
};

struct _AttachProcessPriv
{
	gboolean    hide_paths;
	gboolean    hide_params;
	gboolean    process_tree;

	gchar*      ps_output;
	GSList*     iter_stack;
	gint        iter_stack_level;
	gint        num_spaces_to_skip;
	gint        num_spaces_per_level;
};

enum {
	PID_COLUMN,
	USER_COLUMN,
	START_COLUMN,
	COMMAND_COLUMN,
	COLUMNS_NB
};

static char *column_names[COLUMNS_NB] = {
	N_("Pid"), N_("User"), N_("Time"), N_("Command")
};


static void attach_process_clear (AttachProcess * ap, gint ClearRequest);


AttachProcess *
attach_process_new ()
{
	AttachProcess *ap;
	ap = g_new0 (AttachProcess, 1);
	ap->priv = g_new0 (AttachProcessPriv, 1);
	attach_process_clear (ap, CLEAR_INITIAL);
	return ap;
}

static void
attach_process_clear (AttachProcess * ap, gint ClearRequest)
{
	GtkTreeModel *model;

	// latest ps output
	switch (ClearRequest)
	{
	case CLEAR_UPDATE:
	case CLEAR_FINAL:
		if (ap->priv->ps_output)
		{
			g_free (ap->priv->ps_output);
		}
	case CLEAR_INITIAL:
		ap->priv->ps_output = NULL;
	}

	// helper variables
	switch (ClearRequest)
	{
	case CLEAR_INITIAL:
	case CLEAR_UPDATE:
	case CLEAR_REVIEW:
		ap->pid = -1L;
		ap->priv->iter_stack = NULL;
		ap->priv->iter_stack_level = -1;
		ap->priv->num_spaces_to_skip = -1;
	}

	// tree model
	switch (ClearRequest)
	{
	case CLEAR_UPDATE:
	case CLEAR_REVIEW:
	case CLEAR_FINAL:
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (ap->treeview));
		gtk_tree_store_clear (GTK_TREE_STORE (model));
	}

	// dialog widget
	if (ClearRequest == CLEAR_FINAL)
	{
		// The dialog is destroyed at the places where this
		// function is called.
		// gtk_widget_destroy (ap->dialog);
		ap->dialog = NULL;
	}
}

static inline gchar *
skip_spaces (gchar *pos)
{
	while (*pos == ' ')
		pos++;
	return pos;
}

static inline gchar *
skip_token (gchar *pos)
{
	while (*pos != ' ')
		pos++;
	*pos++ = '\0';
	return pos;
}

static gchar *
skip_token_and_spaces (gchar *pos)
{
	pos = skip_token (pos);
	return skip_spaces (pos);
}

static GtkTreeIter *
iter_stack_push_new (AttachProcess *ap, GtkTreeStore *store)
{
	GtkTreeIter *new_iter, *top_iter;
	new_iter = g_new (GtkTreeIter, 1);
	top_iter = (GtkTreeIter *) (g_slist_nth_data (ap->priv->iter_stack, 0));
	ap->priv->iter_stack =
			g_slist_prepend (ap->priv->iter_stack, (gpointer) (new_iter));
	gtk_tree_store_append (store, new_iter, top_iter);
	ap->priv->iter_stack_level++;
	return new_iter;
}

static gboolean
iter_stack_pop (AttachProcess *ap)
{
	GtkTreeIter *iter;
	
	if (ap->priv->iter_stack_level < 0)
		return FALSE;

	iter = (GtkTreeIter *) (g_slist_nth_data (ap->priv->iter_stack, 0));
	ap->priv->iter_stack =
			g_slist_delete_link (ap->priv->iter_stack, ap->priv->iter_stack);
	g_free (iter);
	ap->priv->iter_stack_level--;
	return TRUE;
}

static void
iter_stack_clear (AttachProcess *ap)
{
	while (iter_stack_pop (ap));
}

static gchar *
calc_depth_and_get_iter (AttachProcess *ap, GtkTreeStore *store,
			GtkTreeIter **iter, gchar *pos)
{
	gchar *orig_pos;
	guint num_spaces, depth, i;

	// count spaces
	orig_pos = pos;
	while (*pos == ' ')
		pos++;
	num_spaces = pos - orig_pos;

	if (ap->priv->process_tree)
	{
		if (ap->priv->num_spaces_to_skip < 0)
		{
			// first process to be inserted
			ap->priv->num_spaces_to_skip = num_spaces;
			ap->priv->num_spaces_per_level = -1;
			*iter = iter_stack_push_new (ap, store);
		}
		else
		{
			if (ap->priv->num_spaces_per_level < 0)
			{
				if (num_spaces == ap->priv->num_spaces_to_skip)
				{
					// num_spaces_per_level still unknown
					iter_stack_pop (ap);
					*iter = iter_stack_push_new (ap, store);
				}
				else
				{
					// first time at level 1
					ap->priv->num_spaces_per_level = 
							num_spaces - ap->priv->num_spaces_to_skip;
					*iter = iter_stack_push_new (ap, store);
				}
			}
			else
			{
				depth = (num_spaces - ap->priv->num_spaces_to_skip) /
						ap->priv->num_spaces_per_level;
				if (depth == ap->priv->iter_stack_level)
				{
					// level not changed
					iter_stack_pop (ap);
					*iter = iter_stack_push_new (ap, store);
				}
				else
					if (depth == ap->priv->iter_stack_level + 1)
						*iter = iter_stack_push_new (ap, store);
					else
						if (depth < ap->priv->iter_stack_level)
						{
							// jump some levels backward
							depth = ap->priv->iter_stack_level - depth;
							for (i = 0; i <= depth; i++)
								iter_stack_pop (ap);
							*iter = iter_stack_push_new (ap, store);
						}
						else
						{
							// should never get here
							g_warning("Unknown error");
							iter_stack_pop (ap);
							*iter = iter_stack_push_new (ap, store);
						}
			}
		}
	}
	else
	{
		iter_stack_pop (ap);
		*iter = iter_stack_push_new (ap, store);
	}

	return pos;
}

static gchar *
skip_path (gchar *pos)
{
	/* can't use g_path_get_basename() - wouldn't work for a processes
	started with parameters containing '/' */
	gchar c, *final_pos = pos;

	if (*pos == G_DIR_SEPARATOR)
		do
		{
			c = *pos;
			if (c == G_DIR_SEPARATOR)
			{
				final_pos = ++pos;
				continue;
			}
			else
				if (c == ' ' || c == '\0')
					break;
				else
					++pos;
		}
		while (1);

	return final_pos;
}

static inline void
remove_params (gchar *pos)
{
	do
	{
		if (*(++pos) == ' ')
			*pos = '\0';
	}
	while (*pos);
}

static void
attach_process_add_line (AttachProcess *ap, GtkTreeStore *store, gchar *line)
{
	gchar *pid, *user, *start, *command, *tmp;
	GtkTreeIter *iter;

	pid = skip_spaces (line); // skip leading spaces
	user = skip_token_and_spaces (pid); // skip PID
	start = skip_token_and_spaces (user); // skip USER
	tmp = skip_token (start); // skip START (do not skip spaces)

	command = calc_depth_and_get_iter (ap, store, &iter, tmp);

	if (ap->priv->hide_paths)
	{
		command = skip_path (command);
	}

	if (ap->priv->hide_params)
	{
		remove_params(command);
	}

	gtk_tree_store_set (store, iter,
						PID_COLUMN, pid,
						USER_COLUMN, user,
						START_COLUMN, start,
						COMMAND_COLUMN, command,
						-1);
}

static void
attach_process_review (AttachProcess *ap)
{
	gchar *ps_output, *begin, *end;
	guint line_num = 0;
	GtkTreeStore *store;

	g_return_if_fail (ap);
	g_return_if_fail (ap->priv->ps_output);
	store = GTK_TREE_STORE (gtk_tree_view_get_model 
							(GTK_TREE_VIEW (ap->treeview)));
	g_return_if_fail (store);

	ps_output = g_strdup (ap->priv->ps_output);
	end = ps_output;
	while (*end)
	{
		begin = end;
		while (*end && *end != '\n') end++;
		if (++line_num > 2) // skip description line & process 'init'
		{
			*end = '\0';
			attach_process_add_line (ap, store, begin);
		}
		end++;
	}
	g_free (ps_output);

	iter_stack_clear (ap);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (ap->treeview));
}

static void
attach_process_update (AttachProcess * ap)
{
	gchar *tmp, *tmp1, *cmd;
	gint ch_pid;
	gchar *shell;
	GtkTreeStore *store;
	gboolean result;

	g_return_if_fail (ap);
	store = GTK_TREE_STORE (gtk_tree_view_get_model
							(GTK_TREE_VIEW (ap->treeview)));
	g_return_if_fail (store);

	if (anjuta_is_installed ("ps", TRUE) == FALSE)
		return;

	tmp = get_a_tmp_file ();
	cmd = g_strconcat ("ps axw -H -o pid,user,start_time,args > ", tmp, NULL);
	shell = gnome_util_user_shell ();
	ch_pid = fork ();
	if (ch_pid == 0)
	{
		execlp (shell, shell, "-c", cmd, NULL);
		g_warning (_("Cannot execute command: \"%s\""), shell);
		_exit(1);
	}
	if (ch_pid < 0)
	{
		anjuta_system_error (errno, _("Unable to execute: %s."), cmd);
		g_free (tmp);
		g_free (cmd);
		return;
	}
	waitpid (ch_pid, NULL, 0);
	g_free (cmd);

	result = g_file_get_contents (tmp, &tmp1, NULL, NULL);
	remove (tmp);
	g_free (tmp);
	if (! result)
	{
		anjuta_system_error (errno, _("Unable to open the file: %s\n"), tmp);
		return;
	}

	attach_process_clear (ap, CLEAR_UPDATE);
	ap->priv->ps_output = anjuta_util_convert_to_utf8 (tmp1);
	g_free (tmp1);
	if (ap->priv->ps_output)
	{
		attach_process_review (ap);
	}
}

static void
on_selection_changed (GtkTreeSelection *selection, AttachProcess *ap)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail (ap);
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar* text;
		gtk_tree_model_get (model, &iter, PID_COLUMN, &text, -1);
		ap->pid = atoi (text);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (ap->dialog), 
										   GTK_RESPONSE_OK, TRUE);
	}
	else
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (ap->dialog), 
										   GTK_RESPONSE_OK, FALSE);
		ap->pid = -1L;
	}
}

static void
on_response (GtkWidget* dialog, gint res, gpointer data)
{
	AttachProcess* ap = data;

	g_return_if_fail (ap);
	switch (res)
	{
		case GTK_RESPONSE_APPLY:
			attach_process_update (ap);
			break;
		case GTK_RESPONSE_OK:
			if (ap->pid > 0) debugger_attach_process (ap->pid);
		case GTK_RESPONSE_CANCEL:
			attach_process_clear (ap, CLEAR_FINAL);
			gtk_widget_destroy (dialog);
	}
}

static gboolean
on_delete_event (GtkWidget *dialog, GdkEvent *event, AttachProcess *ap)
{
	g_return_val_if_fail (ap, FALSE);
	attach_process_clear (ap, CLEAR_FINAL);
	return FALSE;
}

static gint
sort_pid (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
		gpointer user_data)
{
	gchar *nptr;
	gint pid1, pid2;

	gtk_tree_model_get (model, a, PID_COLUMN, &nptr, -1);
	pid1 = atoi (nptr);

	gtk_tree_model_get (model, b, PID_COLUMN, &nptr, -1);
	pid2 = atoi (nptr);

	return pid1 - pid2;
}

static void
on_toggle_hide_paths (GtkToggleButton *togglebutton, AttachProcess * ap)
{
	ap->priv->hide_paths = gtk_toggle_button_get_active (togglebutton);
	attach_process_clear (ap, CLEAR_REVIEW);
	attach_process_review (ap);
}

static void
on_toggle_hide_params (GtkToggleButton *togglebutton, AttachProcess * ap)
{
	ap->priv->hide_params = gtk_toggle_button_get_active (togglebutton);
	attach_process_clear (ap, CLEAR_REVIEW);
	attach_process_review (ap);
}

static void
on_toggle_process_tree (GtkToggleButton *togglebutton, AttachProcess * ap)
{
	ap->priv->process_tree = gtk_toggle_button_get_active (togglebutton);
	attach_process_clear (ap, CLEAR_REVIEW);
	attach_process_review (ap);
}

void
attach_process_show (AttachProcess * ap)
{
	GladeXML *gxml;
	GtkTreeView *view;
	GtkTreeStore *store;
	GtkTreeSelection *selection;
	GtkCheckButton *checkb_hide_paths;
	GtkCheckButton *checkb_hide_params;
	GtkCheckButton *checkb_process_tree;
	gint i;
	
	g_return_if_fail (ap);

	if (ap->dialog) return;
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "attach_process_dialog", NULL);
	ap->dialog = glade_xml_get_widget (gxml, "attach_process_dialog");
	ap->treeview = glade_xml_get_widget (gxml, "attach_process_tv");
	checkb_hide_paths = GTK_CHECK_BUTTON (
							glade_xml_get_widget (gxml, "checkb_hide_paths"));
	checkb_hide_params = GTK_CHECK_BUTTON (
							glade_xml_get_widget (gxml, "checkb_hide_params"));
	checkb_process_tree = GTK_CHECK_BUTTON (
							glade_xml_get_widget (gxml, "checkb_process_tree"));
	g_object_unref (gxml);

	view = GTK_TREE_VIEW (ap->treeview);
	store = gtk_tree_store_new (COLUMNS_NB,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
					 GTK_SELECTION_SINGLE);

	for (i = 0; i < COLUMNS_NB; i++) {
		
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		
		renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes (column_names[i],
													renderer, "text", i, NULL);
		gtk_tree_view_column_set_sort_column_id(column, i);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);
		if (i == COMMAND_COLUMN)
			gtk_tree_view_set_expander_column(view, column);
	}
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), PID_COLUMN,
					sort_pid, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
					START_COLUMN, GTK_SORT_DESCENDING);
	gtk_tree_view_set_search_column (view, COMMAND_COLUMN);

	ap->priv->hide_paths = gtk_toggle_button_get_active (
						GTK_TOGGLE_BUTTON (checkb_hide_paths));
	ap->priv->hide_params = gtk_toggle_button_get_active (
						GTK_TOGGLE_BUTTON (checkb_hide_params));
	ap->priv->process_tree = gtk_toggle_button_get_active (
						GTK_TOGGLE_BUTTON (checkb_process_tree));

	attach_process_update (ap);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ap->treeview));
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_selection_changed), ap);
	g_signal_connect (G_OBJECT (ap->dialog), "response",
					  G_CALLBACK (on_response), ap);
	g_signal_connect (G_OBJECT (ap->dialog), "delete_event",
					  G_CALLBACK (on_delete_event), ap);
	g_signal_connect (GTK_OBJECT (checkb_hide_paths), "toggled",
						G_CALLBACK (on_toggle_hide_paths), ap);
	g_signal_connect (GTK_OBJECT (checkb_hide_params), "toggled",
						G_CALLBACK (on_toggle_hide_params), ap);
	g_signal_connect (GTK_OBJECT (checkb_process_tree), "toggled",
						G_CALLBACK (on_toggle_process_tree), ap);

	gtk_window_set_transient_for (GTK_WINDOW (ap->dialog),
								  GTK_WINDOW (app->widgets.window));
	gtk_widget_show (ap->dialog);
	
	g_object_unref (G_OBJECT (store));
}

void
attach_process_destroy (AttachProcess * ap)
{
	g_return_if_fail (ap);
	g_free (ap->priv);
	g_free (ap);
}
