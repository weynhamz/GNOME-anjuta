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

AttachProcess *
attach_process_new ()
{
	AttachProcess *ap;
	ap = g_new0 (AttachProcess, 1);
	ap->pid = -1L;
	return ap;
}

static void
attach_process_clear (AttachProcess * ap)
{
	GtkTreeModel *model;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ap->treeview));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	ap->pid = -1;
}

static void
attach_process_add_pid (AttachProcess * ap, gchar * line)
{
	gchar pid[10];
	gchar user[512];
	gchar start[512];
	gchar time_s[512];
	gint count;

	if (!ap)
		return;

	/* this should not happen, abort */
	if (isspace (line[0]))
		return;

	count = sscanf (line, "%s %s %*s %*s %*s %*s %*s %*s %s %s",
			user, pid, start, time_s);

	if (count == 4) {
		GtkTreeIter iter;
		GtkTreeStore *store;
		gchar *command;

		command = strstr (line, time_s);
		command += strlen (time_s);

		if (!command)
			return;	/* Should not happen */

		store = GTK_TREE_STORE (gtk_tree_view_get_model
								(GTK_TREE_VIEW (ap->treeview)));
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter,
							PID_COLUMN, pid,
							USER_COLUMN, user,
							START_COLUMN, start,
							COMMAND_COLUMN, command,
							-1);
	}
}

static void
attach_process_update (AttachProcess * ap)
{
	gchar buffer[FILE_BUFFER_SIZE], *tmp, *cmd;
	gint i, ch_pid, count, first_flag;
	FILE *file;
	gchar *shell;

	if (anjuta_is_installed ("ps", TRUE) == FALSE)
		return;

	tmp = get_a_tmp_file ();
	cmd = g_strconcat ("ps --sort=-pid -auxw > ", tmp, NULL);
	shell = gnome_util_user_shell ();
	ch_pid = fork ();
	if (ch_pid == 0)
	{
		execlp (shell, shell, "-c", cmd, NULL);
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
	file = fopen (tmp, "r");
	if (file == NULL)
	{
		anjuta_system_error (errno, _("Unable to open the file: %s\n"), tmp);
		remove (tmp);
		g_free (tmp);
		g_free (cmd);
		return;
	}
	attach_process_clear (ap);
	i = 0;
	first_flag = 0;
	while (!feof (file))
	{
		if (i > FILE_BUFFER_SIZE - 3)
			i = 0;
		count = fread (&buffer[i], sizeof (char), 1, file);
		if (count != 1)
		{
			fclose (file);
			remove (tmp);
			g_free (tmp);
			return;
		}
		if (buffer[i] == '\n')
		{
			buffer[i] = '\0';
			if (first_flag > 2)
				attach_process_add_pid (ap, buffer);
			first_flag++;
			i = 0;
			continue;
		}
		i++;
	}
	fclose (file);
	remove (tmp);
	g_free (tmp);
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
		case GTK_RESPONSE_CLOSE:
			gtk_widget_destroy (ap->dialog);
			ap->pid = -1L;
			ap->dialog = NULL;
	}
}

static gboolean
on_delete_event (GtkWidget *dialog, GdkEvent *event, AttachProcess *ap)
{
	g_return_val_if_fail (ap, FALSE);
	ap->dialog = NULL;
	ap->pid = -1L;
	return FALSE;
}

void
attach_process_show (AttachProcess * ap)
{
	GladeXML *gxml;
	GtkTreeView *view;
	GtkTreeStore *store;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	int i;
	
	g_return_if_fail (ap);

	if (ap->dialog) return;
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "attach_process_dialog", NULL);
	ap->dialog = glade_xml_get_widget (gxml, "attach_process_dialog");
	ap->treeview = glade_xml_get_widget (gxml, "attach_process_tv");
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
	g_object_unref (G_OBJECT (store));

	renderer = gtk_cell_renderer_text_new ();

	for (i = PID_COLUMN; i < COLUMNS_NB; i++) {
		GtkTreeViewColumn *column;

		column = gtk_tree_view_column_new_with_attributes (column_names[i],
													 renderer, "text", i, NULL);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);
	}
	attach_process_update (ap);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ap->treeview));
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_selection_changed), ap);
	g_signal_connect (G_OBJECT (ap->dialog), "response",
					  G_CALLBACK (on_response), ap);
	g_signal_connect (G_OBJECT (ap->dialog), "delete_event",
					  G_CALLBACK (on_delete_event), ap);

	gtk_window_set_transient_for (GTK_WINDOW (ap->dialog),
								  GTK_WINDOW (app->widgets.window));
	gtk_widget_show (ap->dialog);
}

void
attach_process_destroy (AttachProcess * ap)
{
	g_return_if_fail (ap);
	g_free (ap);
}
