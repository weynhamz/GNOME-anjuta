/*
 * compiler_options_cbs.c Copyright (C) 2000  Kh. Naba Kumar Singh
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

#include <ctype.h>
#include <gnome.h>

#include "anjuta.h"
#include "compiler_options.h"
#include "messagebox.h"
#include "compiler_options_cbs.h"
#include "resources.h"
#include "anjuta_info.h"

extern GdkPixmap *list_unselect_pixmap, *list_select_pixmap;
extern GdkBitmap *list_unselect_mask, *list_select_mask;

static void
on_clear_clist_confirm_yes_clicked (GtkButton * button, gpointer data);

gboolean
on_comopt_delete_event (GtkWidget * w, GdkEvent * event, gpointer user_data)
{
	CompilerOptions *co = user_data;
	compiler_options_hide (co);
	return TRUE;
}


void
on_comopt_help_clicked (GtkButton * button, gpointer user_data)
{
}

void
on_comopt_ok_clicked (GtkButton * button, gpointer user_data)
{
	CompilerOptions *co = user_data;
	on_comopt_apply_clicked (NULL, co);
	compiler_options_hide (co);
}

void
on_comopt_apply_clicked (GtkButton * button, gpointer user_data)
{
	gint i;
	CompilerOptions *co = user_data;

	for (i = 0; i < 16; i++)
	{
		co->warning_button_state[i] =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (co->widgets.
						       warning_button[i]));
	}
	for (i = 0; i < 4; i++)
	{
		co->optimize_button_state[i] =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (co->widgets.
						       optimize_button[i]));
	}
	for (i = 0; i < 2; i++)
	{
		co->other_button_state[i] =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (co->widgets.
						       other_button[i]));
	}

	if (co->other_c_flags)
		g_free (co->other_c_flags);
	co->other_c_flags =
		g_strdup (gtk_entry_get_text
			  (GTK_ENTRY (co->widgets.other_c_flags_entry)));
	
	if (co->other_l_flags)
		g_free (co->other_l_flags);
	co->other_l_flags =
		g_strdup (gtk_entry_get_text
			  (GTK_ENTRY (co->widgets.other_l_flags_entry)));
	
	if (co->other_l_libs)
		g_free (co->other_l_libs);
	co->other_l_libs =
		g_strdup (gtk_entry_get_text
			  (GTK_ENTRY (co->widgets.other_l_libs_entry)));
	
	compiler_options_set_in_properties (co, co->props);

	if (app->project_dbase->project_is_open)
	{
		app->project_dbase->is_saved = FALSE;
	}
	else
		anjuta_save_settings ();
}

void
on_comopt_cancel_clicked (GtkButton * button, gpointer user_data)
{
	CompilerOptions *co = user_data;
	compiler_options_hide (co);
}

void
on_co_supp_clist_select_row (GtkCList * clist,
			     gint row,
			     gint column,
			     GdkEvent * event, gpointer user_data)
{
	CompilerOptions *co = user_data;
	co->supp_index = row;
	if (row >= 0 && g_list_length (clist->row_list) > 0)
	{
		if (event == NULL)
			return;
		if (event->type != GDK_2BUTTON_PRESS)
			return;
		if (((GdkEventButton *) event)->button != 1)
			return;
		co_clist_row_data_toggle_state (clist, row);
	}
}

void
on_co_inc_clist_select_row (GtkCList * clist,
			    gint row,
			    gint column, GdkEvent * event, gpointer user_data)
{
	gchar *text;
	CompilerOptions *co = user_data;
	co->inc_index = row;
	if (row >= 0 && g_list_length (clist->row_list) > 0)
	{
		gtk_clist_get_text (clist, row, 0, &text);
		gtk_entry_set_text (GTK_ENTRY (co->widgets.inc_entry), text);
	}
}

void
on_co_lib_stock_clist_select_row (GtkCList * clist,
				  gint row,
				  gint column,
				  GdkEvent * event, gpointer user_data)
{
	gchar *text;
	CompilerOptions *co = user_data;
	if (row >= 0 && g_list_length (clist->row_list) > 0)
	{
		gtk_clist_get_text (clist, row, 0, &text);
		gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_entry), text);
	}
}

void
on_co_lib_clist_select_row (GtkCList * clist,
			    gint row,
			    gint column, GdkEvent * event, gpointer user_data)
{
	gchar *text;
	CompilerOptions *co = user_data;
	co->lib_index = row;
	if (row >= 0 && g_list_length (clist->row_list) > 0)
	{
		gtk_clist_get_text (clist, row, 1, &text);
		gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_entry), text);
		if (event == NULL)
			return;
		if (event->type != GDK_2BUTTON_PRESS)
			return;
		if (((GdkEventButton *) event)->button != 1)
			return;
		co_clist_row_data_toggle_state (clist, row);
	}
}

void
on_co_lib_paths_clist_select_row (GtkCList * clist,
				  gint row,
				  gint column,
				  GdkEvent * event, gpointer user_data)
{
	gchar *text;
	CompilerOptions *co = user_data;
	co->lib_paths_index = row;
	if (row >= 0 && g_list_length (clist->row_list) > 0)
	{
		gtk_clist_get_text (clist, row, 0, &text);
		gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_paths_entry), text);
	}
}

void
on_co_def_clist_select_row (GtkCList * clist,
			    gint row,
			    gint column, GdkEvent * event, gpointer user_data)
{
	gchar *text;
	CompilerOptions *co = user_data;
	co->def_index = row;
	if (row >= 0 && g_list_length (clist->row_list) > 0)
	{
		gtk_clist_get_text (clist, row, 0, &text);
		gtk_entry_set_text (GTK_ENTRY (co->widgets.def_entry), text);
	}
}

void
on_co_supp_info_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co;
	gchar *tmp_file, *str;
	FILE *tmp;
	gint index;

	co = data;
	index = co->supp_index;
	tmp_file = get_a_tmp_file ();
	tmp = fopen (tmp_file, "w");
	if (!tmp)
	{
		anjuta_error (_("Cannot create: %s."), tmp_file);
		return;
	}
	fprintf (tmp, _("Support ID: %s\n\n"), anjuta_supports[index][ANJUTA_SUPPORT_ID]);
	fprintf (tmp, _("Description:\n"));
	fprintf (tmp, "%s\n\n", anjuta_supports[index][ANJUTA_SUPPORT_DESCRIPTION]);
	fprintf (tmp, _("Dependencies: %s\n\n"), anjuta_supports[index][ANJUTA_SUPPORT_DEPENDENCY]);
	fprintf (tmp, _("Macros in configure.in fie:\n"));
	fprintf (tmp, "%s\n", anjuta_supports[index][ANJUTA_SUPPORT_MACROS]);
	fprintf (tmp, _("Variable(s) for cflags: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_PRJ_CFLAGS]);
	fprintf (tmp, _("Variable(s) for Libs: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_PRJ_LIBS]);
	fprintf (tmp, _("Compile time Cflags: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_FILE_CFLAGS]);
	fprintf (tmp, _("Compile time Libs: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_FILE_LIBS]);
	fprintf (tmp, _("Entries in acconfig.h file:\n"));
	fprintf (tmp, "%s\n", anjuta_supports[index][ANJUTA_SUPPORT_ACCONFIG_H]);
	fprintf (tmp, _("Installed status:\n"));
	str = anjuta_supports[index][ANJUTA_SUPPORT_INSTALL_STATUS];
	if (str)
	{
		gchar *cmd, *count, *path;
		cmd = g_strdup (str);
		count = cmd;
		while (isspace(*count) == FALSE)
		{
			if (*count == '\0')
				break;
			count++;
		}
		*count = '\0';
		path = gnome_is_program_in_path ( cmd );
		fprintf (tmp, _("    Installed => %s\n"), path);
		g_free (cmd);
	}
	else
		fprintf (tmp, _("    Not Installed.\n"));
	fclose (tmp);
	anjuta_info_show_file (tmp_file, 300, 500);
	remove (tmp_file);
}

void
on_co_supp_help_clicked (GtkButton * button, gpointer data)
{
}

void
on_co_inc_add_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *dummy[1];
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;

	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.inc_entry));
	if (strlen (text) == 0)
		return;
	dummy[0] = text;
	
	max_rows = g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.inc_clist), cur_row, 0, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */;
			gtk_entry_set_text (GTK_ENTRY (co->widgets.inc_entry), "");
			return;
		}
	}
	gtk_clist_append (GTK_CLIST (co->widgets.inc_clist), dummy);
	gtk_entry_set_text (GTK_ENTRY (co->widgets.inc_entry), "");
	compiler_options_update_controls (co);
}

void
on_co_inc_update_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;

	if (g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list) < 1)
		return;
	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.inc_entry));
	if (strlen (text) == 0)
		return;
	
	max_rows = g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.inc_clist), cur_row, 0, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */
			gtk_entry_set_text (GTK_ENTRY (co->widgets.inc_entry), "");
			return;
		}
	}
	gtk_clist_set_text (GTK_CLIST (co->widgets.inc_clist), co->inc_index, 0, text);
}

void
on_co_inc_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list) < 1)
		return;
	gtk_entry_set_text (GTK_ENTRY (co->widgets.inc_entry), "");
	gtk_clist_remove (GTK_CLIST (co->widgets.inc_clist), co->inc_index);
	compiler_options_update_controls (co);
}

void
on_co_inc_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list) < 1)
		return;
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to clear the list?"),
		     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		     on_clear_clist_confirm_yes_clicked, NULL,
		     co->widgets.inc_clist);
}

void
on_co_inc_help_clicked (GtkButton * button, gpointer data)
{
}

void
on_co_lib_add_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *dummy[2];
	gchar dumtext[] = "";
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;

	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.lib_entry));
	if (strlen (text) == 0)
		return;
	
	max_rows = g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.lib_clist), cur_row, 1, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */
			gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_entry), "");
			return;
		}
	}
	
	dummy[0] = dumtext;
	dummy[1] = text;
	gtk_clist_append (GTK_CLIST (co->widgets.lib_clist), dummy);
	co_clist_row_data_set_true (GTK_CLIST(co->widgets.lib_clist), 
			  GTK_CLIST(co->widgets.lib_clist)->rows-1);
	compiler_options_update_controls (co);
}

void
on_co_lib_update_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;

	if (g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list) < 1)
		return;
	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.lib_entry));
	if (strlen (text) == 0)
		return;
	max_rows = g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.lib_clist), cur_row, 1, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */
			gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_entry), "");
			return;
		}
	}
	gtk_clist_set_text (GTK_CLIST (co->widgets.lib_clist), co->lib_index,
			    1, text);
}

void
on_co_lib_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list) < 1)
		return;
	gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_entry), "");
	gtk_clist_remove (GTK_CLIST (co->widgets.lib_clist), co->lib_index);
	compiler_options_update_controls (co);
}

void
on_co_lib_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list) < 1)
		return;
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to clear the list?"),
		     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		     on_clear_clist_confirm_yes_clicked, NULL,
		     co->widgets.lib_clist);
}

void
on_co_lib_help_clicked (GtkButton * button, gpointer data)
{
}

void
on_co_lib_paths_add_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *dummy[1];
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;

	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.lib_paths_entry));
	if (strlen (text) == 0)
		return;
	
	max_rows = g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.lib_paths_clist), cur_row, 0, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */
			gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_paths_entry), "");
			return;
		}
	}
	dummy[0] = text;
	gtk_clist_append (GTK_CLIST (co->widgets.lib_paths_clist), dummy);
	gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_paths_entry), "");
	compiler_options_update_controls (co);
}

void
on_co_lib_paths_update_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;

	if (g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->row_list)
	    < 1)
		return;
	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.lib_paths_entry));
	if (strlen (text) == 0)
		return;
	
	max_rows = g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.lib_paths_clist), cur_row, 0, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */
			gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_paths_entry), "");
			return;
		}
	}
	gtk_clist_set_text (GTK_CLIST (co->widgets.lib_paths_clist),
			    co->lib_paths_index, 0, text);
}

void
on_co_lib_paths_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->row_list)
	    < 1)
		return;
	gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_paths_entry), "");
	gtk_clist_remove (GTK_CLIST (co->widgets.lib_paths_clist),
			  co->lib_paths_index);
	compiler_options_update_controls (co);
}

void
on_co_lib_paths_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->row_list)
	    < 1)
		return;
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to clear the list?"),
		     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		     on_clear_clist_confirm_yes_clicked, NULL,
		     co->widgets.lib_paths_clist);
}

void
on_co_lib_paths_help_clicked (GtkButton * button, gpointer data)
{
}

void
on_co_def_add_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *row_text;
	gchar *dummy[1];
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;

	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.def_entry));
	if (strlen (text) == 0)
		return;
	dummy[0] = text;
	max_rows = g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.def_clist), cur_row, 0, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */
			gtk_entry_set_text (GTK_ENTRY (co->widgets.def_entry), "");
			return;
		}
	}
	
	gtk_clist_append (GTK_CLIST (co->widgets.def_clist), dummy);
	gtk_entry_set_text (GTK_ENTRY (co->widgets.def_entry), "");
	compiler_options_update_controls (co);
}

void
on_co_def_update_clicked (GtkButton * button, gpointer data)
{
	gchar *text;
	gchar *row_text;
	gint max_rows;
	gint cur_row;
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list) < 1)
		return;
	text = gtk_entry_get_text (GTK_ENTRY (co->widgets.def_entry));
	if (strlen (text) == 0)
		return;
	
	max_rows = g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list);
	for (cur_row = 0; cur_row < max_rows; cur_row++)
	{
		gtk_clist_get_text(GTK_CLIST (co->widgets.def_clist), cur_row, 0, &row_text);
		if (strcmp(text, row_text) == 0)
		{
			/* Maybe print a message here */
			gtk_entry_set_text (GTK_ENTRY (co->widgets.def_entry), "");
			return;
		}
	}
	gtk_clist_set_text (GTK_CLIST (co->widgets.def_clist), co->def_index,
			    0, text);
}

void
on_co_def_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list) < 1)
		return;
	gtk_entry_set_text (GTK_ENTRY (co->widgets.def_entry), "");
	gtk_clist_remove (GTK_CLIST (co->widgets.def_clist), co->def_index);
	compiler_options_update_controls (co);
}

void
on_co_def_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	if (g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list) < 1)
		return;
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to clear the list?"),
		     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		     on_clear_clist_confirm_yes_clicked, NULL,
		     co->widgets.def_clist);
}

void
on_co_def_help_clicked (GtkButton * button, gpointer data)
{
}

static void
on_clear_clist_confirm_yes_clicked (GtkButton * button, gpointer data)
{
	GtkCList *clist = data;
	gtk_clist_clear (clist);
	compiler_options_update_controls (app->compiler_options);
}
