/*
 * src_paths_cbs.c Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <gnome.h>

#include "anjuta.h"
#include "src_paths.h"
//#include "messagebox.h"
#include "src_paths_cbs.h"
#include "resources.h"

static void
on_clear_clist_confirm_yes_clicked (GtkButton * button, gpointer data);

gboolean
on_src_paths_close (GtkWidget * w, gpointer user_data)
{
	SrcPaths *co = user_data;
	src_paths_hide (co);
	return FALSE;
}



void
on_src_paths_help_clicked (GtkButton * button, gpointer user_data)
{
}

void
on_src_paths_ok_clicked (GtkButton * button, gpointer user_data)
{
	SrcPaths *co = user_data;
	on_src_paths_apply_clicked (NULL, co);
	if (NULL != co)
		gnome_dialog_close(GNOME_DIALOG(co->widgets.window));
}

void
on_src_paths_apply_clicked (GtkButton * button, gpointer user_data)
{
	if (app->project_dbase->project_is_open)
		app->project_dbase->is_saved = FALSE;
	else
		anjuta_save_settings ();
}

void
on_src_paths_cancel_clicked (GtkButton * button, gpointer user_data)
{
	SrcPaths *co = user_data;
	if (NULL != co)
		gnome_dialog_close(GNOME_DIALOG(co->widgets.window));
}

void
on_src_paths_src_clist_select_row (GtkCList * clist,
				   gint row,
				   gint column,
				   GdkEvent * event, gpointer user_data)
{
	gchar *text;
	SrcPaths *co = user_data;

//entry_file = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY (co->widgets.src_entry));
	co->src_index = row;
	gtk_clist_get_text (clist, row, 0, &text);
	gtk_entry_set_text (GTK_ENTRY (co->widgets.src_entry), text);
}

void
on_src_paths_src_add_clicked (GtkButton * button, gpointer data)
{
	gchar *text, *entry_text;
	gchar *dummy[1];
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	SrcPaths *co = data;

	entry_text =
		g_strdup (gtk_entry_get_text (GTK_ENTRY (co->widgets.src_entry)));
	text = g_strstrip(entry_text);
	if (!text || strlen (text) == 0)
		return;
	dummy[0] = text;

	max_rows = g_list_length (GTK_CLIST (co->widgets.src_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.src_clist), cur_row, 0, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */;
			gtk_entry_set_text (GTK_ENTRY (co->widgets.src_entry), "");
			g_free (entry_text);
			return;
		}
	}
	gtk_clist_append (GTK_CLIST (co->widgets.src_clist), dummy);
	gtk_entry_set_text (GTK_ENTRY (co->widgets.src_entry), "");
	src_paths_update_controls (co);
	g_free (entry_text);
}

void
on_src_paths_src_update_clicked (GtkButton * button, gpointer data)
{
	const gchar *text;
	SrcPaths *co = data;

	if (g_list_length (GTK_CLIST (co->widgets.src_clist)->row_list) < 1)
		return;
	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.src_entry));
	if (strlen (text) == 0)
		return;
	gtk_clist_set_text (GTK_CLIST (co->widgets.src_clist), co->src_index,
			    0, text);
}

void
on_src_paths_src_remove_clicked (GtkButton * button, gpointer data)
{
	SrcPaths *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.src_clist)->row_list) < 1)
		return;
	gtk_signal_disconnect_by_func (GTK_OBJECT (co->widgets.src_clist),
			    GTK_SIGNAL_FUNC (on_src_paths_src_clist_select_row ),
			    co);
	gtk_entry_set_text (GTK_ENTRY (co->widgets.src_entry), "");
	gtk_clist_remove (GTK_CLIST (co->widgets.src_clist), co->src_index);
	gtk_signal_connect (GTK_OBJECT (co->widgets.src_clist), "select_row",
			    GTK_SIGNAL_FUNC (on_src_paths_src_clist_select_row ),
				co);
	src_paths_update_controls (co);
}

void
on_src_paths_src_clear_clicked (GtkButton * button, gpointer data)
{
	SrcPaths *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.src_clist)->row_list) < 1)
		return;
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to clear the list?"),
		     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		     on_clear_clist_confirm_yes_clicked, NULL,
		     co->widgets.src_clist);
}

void
on_src_paths_src_help_clicked (GtkButton * button, gpointer data)
{
}

static void
on_clear_clist_confirm_yes_clicked (GtkButton * button, gpointer data)
{
	GtkCList *clist = data;
	gtk_clist_clear (clist);
	src_paths_update_controls (app->src_paths);
}
