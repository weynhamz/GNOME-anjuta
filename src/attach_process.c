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

#include "resources.h"
#include "attach_process.h"
#include "global.h"
#include "anjuta.h"

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

static gboolean
on_attach_process_close(GtkWidget* win, gpointer data)
{
	AttachProcess* ap = data;

	attach_process_hide (ap);

	return FALSE;
}

static void
on_attach_process_tv_event (GtkWidget *w,
			    GdkEvent  *event,
			    gpointer   data)
{
	AttachProcess *ap = data;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar* text;

	g_return_if_fail (GTK_IS_TREE_VIEW (w));

	view = GTK_TREE_VIEW (w);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return ;

	gtk_tree_model_get (model, &iter,
			   PID_COLUMN, &text,
			   -1);

	ap->pid = atoi(text);
}

static void
on_attach_process_update_clicked(GtkWidget* button, gpointer data)
{
	AttachProcess* ap = data;

	attach_process_update (ap);
}

static void
on_attach_process_attach_clicked(GtkWidget* button, gpointer data)
{
	AttachProcess* ap = data;

	gtk_widget_hide (ap->widgets.window);

	if (ap->pid < 0)
		return ;

	debugger_attach_process (ap->pid);
}

AttachProcess *
attach_process_new ()
{
	AttachProcess *ap;

	ap = g_new0 (AttachProcess, 1);

	if (ap) {
		GtkTreeView *view;
		GtkTreeStore *store;
		GtkCellRenderer *renderer;
		int i;

		ap->widgets.window = glade_xml_get_widget (app->gxml, "attach_process_dialog");
		ap->widgets.treeview = glade_xml_get_widget (app->gxml, "attach_process_tv");
		ap->widgets.update_button = glade_xml_get_widget (app->gxml, "attach_process_update_button");
		ap->widgets.attach_button = glade_xml_get_widget (app->gxml, "attach_process_attach_button");

		gtk_widget_ref (ap->widgets.window);
		gtk_widget_ref (ap->widgets.treeview);
		gtk_widget_ref (ap->widgets.update_button);
		gtk_widget_ref (ap->widgets.attach_button);

		view = GTK_TREE_VIEW (ap->widgets.treeview);
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

			column = gtk_tree_view_column_new_with_attributes (column_names[i], renderer, "text", i, NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (view, column);
		}
		
		ap->pid = -1L;
		ap->is_showing = FALSE;
		ap->win_pos_x = 20;
		ap->win_pos_y = 325;
		ap->win_width = 400;
		ap->win_height = 150;
	}

	return ap;
}

void
attach_process_clear (AttachProcess * ap)
{
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ap->widgets.treeview));
	gtk_tree_store_clear (GTK_TREE_STORE (model));

	ap->pid = -1;
}

void
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


		store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (ap->widgets.treeview)));
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter,
				    PID_COLUMN, pid,
				    USER_COLUMN, user,
				    START_COLUMN, start,
				    COMMAND_COLUMN, command,
				    -1);
	}
}

void
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
			buffer[i + 1] = '\0';
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

void
attach_process_show (AttachProcess * ap)
{
	if (ap)
	{
		attach_process_update (ap);
		if (ap->is_showing)
		{
			gdk_window_raise (ap->widgets.window->window);
			return;
		}
		gtk_widget_set_uposition (ap->widgets.window, ap->win_pos_x,
					  ap->win_pos_y);
		gtk_window_set_default_size (GTK_WINDOW (ap->widgets.window),
					     ap->win_width, ap->win_height);
		gtk_widget_show (ap->widgets.window);
		ap->is_showing = TRUE;
	}
}

void
attach_process_hide (AttachProcess * ap)
{
	if (ap)
	{
		if (ap->is_showing == FALSE)
			return;
		gdk_window_get_root_origin (ap->widgets.window->window,
					    &ap->win_pos_x, &ap->win_pos_y);
		gdk_window_get_size (ap->widgets.window->window,
				     &ap->win_width, &ap->win_height);
		ap->is_showing = FALSE;
	}
}

gboolean
attach_process_save_yourself (AttachProcess * ap, FILE * stream)
{
	if (!ap)
		return FALSE;
	if (ap->is_showing)
	{
		gdk_window_get_root_origin (ap->widgets.window->window,
					    &ap->win_pos_x, &ap->win_pos_y);
		gdk_window_get_size (ap->widgets.window->window,
				     &ap->win_width, &ap->win_height);
	}
	fprintf (stream, "attach.process.win.pos.x=%d\n", ap->win_pos_x);
	fprintf (stream, "attach.process.win.pos.y=%d\n", ap->win_pos_y);
	fprintf (stream, "attach.process.win.width=%d\n", ap->win_width);
	fprintf (stream, "attach.process.win.height=%d\n", ap->win_height);
	return TRUE;
}

gboolean
attach_process_load_yourself (AttachProcess * ap, PropsID props)
{
	if (!ap)
		return FALSE;
	ap->win_pos_x = prop_get_int (props, "attach.process.win.pos.x", 20);
	ap->win_pos_y = prop_get_int (props, "attach.process.win.pos.y", 325);
	ap->win_width = prop_get_int (props, "attach.process.win.width", 400);
	ap->win_height =
		prop_get_int (props, "attach.process.win.height", 150);
	return TRUE;
}

void
attach_process_destroy (AttachProcess * ap)
{
	if (ap)
	{
		attach_process_clear (ap);

		gtk_widget_unref (ap->widgets.window);
		gtk_widget_unref (ap->widgets.treeview);
		gtk_widget_unref (ap->widgets.update_button);
		gtk_widget_unref (ap->widgets.attach_button);

		gtk_widget_destroy (ap->widgets.window);

		g_free (ap);
	}
}
