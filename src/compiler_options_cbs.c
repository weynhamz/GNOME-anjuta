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
//#include "messagebox.h"
#include "compiler_options_cbs.h"
#include "resources.h"
#include "anjuta_info.h"

gboolean
on_compt_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	CompilerOptions *co = user_data;
	compiler_options_set_in_properties(co, co->props);
	compiler_options_hide(co);
	return TRUE;
}


static void
on_clear_clist_confirm_yes_clicked (GtkButton * button, gpointer data)
{
	GtkListStore *clist = data;
	gtk_list_store_clear (clist);
}

gboolean
on_comopt_close (GtkWidget * w, gpointer user_data)
{
	CompilerOptions *co = user_data;
	compiler_options_set_in_properties(co, co->props);
	compiler_options_hide(co);
	return TRUE;
}

void
on_comopt_help_clicked (GtkButton * button, gpointer user_data)
{
}

void
on_co_supp_clist_row_activated             (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data)
{

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
	fprintf (tmp, _("Macros in configure.in file:\n"));
	fprintf (tmp, "%s\n", anjuta_supports[index][ANJUTA_SUPPORT_MACROS]);
	fprintf (tmp, _("Variables for cflags: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_PRJ_CFLAGS]);
	fprintf (tmp, _("Variables for libraries: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_PRJ_LIBS]);
	fprintf (tmp, _("Compile time cflags: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_FILE_CFLAGS]);
	fprintf (tmp, _("Compile time libraries: %s\n\n"),
		 anjuta_supports[index][ANJUTA_SUPPORT_FILE_LIBS]);
	fprintf (tmp, _("Entries in acconfig.h file:\n"));
	fprintf (tmp, "%s\n", anjuta_supports[index][ANJUTA_SUPPORT_ACCONFIG_H]);
	fprintf (tmp, _("Installation status:\n"));
	str = anjuta_supports[index][ANJUTA_SUPPORT_INSTALL_STATUS];
	if (str)
	{
		gchar *cmd, *count;
		const gchar *path;
		
		cmd = g_strdup (str);
		count = cmd;
		while (isspace(*count) == FALSE)
		{
			if (*count == '\0')
				break;
			count++;
		}
		*count = '\0';
		path = g_find_program_in_path ( cmd );
		if (path)
			fprintf (tmp, _("    Installed => %s\n"), path);
		else
			fprintf (tmp, _("    Not Installed\n"), path);
			
		g_free (cmd);
	}
	else
		fprintf (tmp, _("    Not Installed.\n"));
	fclose (tmp);
	anjuta_info_show_file (tmp_file, 300, 500);
	remove (tmp_file);
}

static gboolean
verify_new_entry (GtkWidget *tree, const gchar *str, gint col)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree));
	g_assert (model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gchar *text;
		gtk_tree_model_get (model, &iter, col, &text, -1);
		if (strcmp(str, text) == 0)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	return TRUE;
}

static void
add_new_entry (GtkWidget *tree, GtkWidget *entry, gint col)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *text;
	
	text = g_strstrip((gchar*)gtk_entry_get_text (GTK_ENTRY (entry)));
	if (strlen (text) == 0)
		return;
	if (verify_new_entry (tree, text, 0) == FALSE)
	{
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		return;
	}
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree));
	g_assert (model);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	if (col == 0)
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, text);
	else
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0,
			TRUE, col, text, -1);
	gtk_entry_set_text (GTK_ENTRY (entry), "");
}

static void
update_entry (GtkWidget *tree, GtkWidget *entry, gint col)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *text;
	
	text = g_strstrip((gchar*)gtk_entry_get_text (GTK_ENTRY (entry)));
	if (strlen (text) == 0)
		return;
	if (verify_new_entry (tree, text, 0) == FALSE)
	{
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		return;
	}
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree));
	g_assert (model);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	if (col == 0)
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, text);
	else
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0,
			TRUE, col, text, -1);
	gtk_entry_set_text (GTK_ENTRY (entry), "");
}

void
on_co_inc_clist_row_activated             (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data)
{

}

void
on_co_inc_add_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	add_new_entry (co->widgets.inc_clist, co->widgets.inc_entry, 0);
}

void
on_co_inc_update_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	update_entry (co->widgets.inc_clist, co->widgets.inc_entry, 0);
}

void
on_co_inc_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}

void
on_co_inc_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}

void
on_co_lib_clist_row_activated             (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data)
{

}

void
on_co_lib_add_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	add_new_entry (co->widgets.lib_clist, co->widgets.lib_entry, 1);
}

void
on_co_lib_update_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	update_entry (co->widgets.lib_clist, co->widgets.lib_entry, 1);
}

void
on_co_lib_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}

void
on_co_lib_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}

void
on_co_lib_paths_clist_row_activated             (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data)
{

}

void
on_co_lib_paths_add_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	add_new_entry (co->widgets.lib_paths_clist, co->widgets.lib_paths_entry, 0);
}

void
on_co_lib_paths_update_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	update_entry (co->widgets.lib_paths_clist, co->widgets.lib_paths_entry, 0);
}

void
on_co_lib_paths_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}

void
on_co_lib_paths_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}

void
on_co_def_clist_row_activated             (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data)
{

}

void
on_co_def_add_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	add_new_entry (co->widgets.def_clist, co->widgets.def_entry, 0);
}

void
on_co_def_update_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
	update_entry (co->widgets.def_clist, co->widgets.def_entry, 0);
}

void
on_co_def_remove_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}

void
on_co_def_clear_clicked (GtkButton * button, gpointer data)
{
	CompilerOptions *co = data;
}
