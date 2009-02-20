/*  
 *  macro-actions.c (c) 2005 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include "macro-actions.h"
#include "macro-db.h"
#include "macro-dialog.h"
#include "macro-edit.h"

static gboolean
match_shortcut (MacroPlugin * plugin, GtkTreeIter * iter,
		gchar key)
{
	gchar shortcut;
	gint offset = 0;
	
	gtk_tree_model_get(macro_db_get_model(plugin->macro_db), iter,
		MACRO_SHORTCUT, &shortcut, -1);
	if (key == shortcut)
	{
		gchar* text = macro_db_get_macro(plugin, plugin->macro_db, iter, &offset);
		if (plugin->current_editor != NULL && text != NULL)
		{
			gint i;
			IAnjutaIterable *pos;
			pos = ianjuta_editor_get_position (IANJUTA_EDITOR(plugin->current_editor),
			                                   NULL);
			ianjuta_editor_insert (IANJUTA_EDITOR (plugin->current_editor),
								   pos, text, -1, NULL);
			for (i = 0; i < offset; i++)
				ianjuta_iterable_next (pos, NULL);
			ianjuta_editor_goto_position (IANJUTA_EDITOR(plugin->current_editor), 
			                              pos, NULL);
			g_free(text);
			g_object_unref (pos);
		}
		return TRUE;
	}
	return FALSE;
}

/* Shortcut handling */
static gboolean
on_shortcut_pressed (GtkWidget * window, GdkEventKey * event,
					 MacroPlugin * plugin)
{
	gchar key;
	GtkTreeIter parent;
	GtkTreeIter cur_cat;
	GtkTreeModel *model = macro_db_get_model (plugin->macro_db);
	/* Plase note that this implementation is deprecated but
	 * I could not figure out how to do this with GtkIMContext as 
	 * proposed by the gtk docs */
#warning FIXME: Deprecated
	if (event->length)
		key = event->string[0];
	else
		return TRUE;
	gtk_tree_model_get_iter_first (model, &parent);
	do
	{
		if (gtk_tree_model_iter_children (model, &cur_cat, &parent))
		{
		do
		{
			GtkTreeIter cur_macro;
			if (gtk_tree_model_iter_children
			    (model, &cur_macro, &cur_cat))
			{
				do
				{
					if (match_shortcut(plugin, &cur_macro, key))
					{
						gtk_widget_destroy(window);
						return TRUE;
					}
				}
				while (gtk_tree_model_iter_next
				       (model, &cur_macro));
			}
			else
			{
				gboolean is_category;
				gtk_tree_model_get (model, &cur_cat,
						    MACRO_IS_CATEGORY,
						    &is_category, -1);
				if (is_category)
					continue;
				if (match_shortcut(plugin, &cur_cat, key))
				{
					gtk_widget_destroy(window);
					return TRUE;
				}
			}
		}
		while (gtk_tree_model_iter_next (model, &cur_cat));
		}
	}
	while (gtk_tree_model_iter_next (model, &parent));
	gtk_widget_destroy(window);
	return TRUE;
}

void
on_menu_insert_macro (GtkAction * action, MacroPlugin * plugin)
{
	if (plugin->current_editor == NULL)
		return;
	
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget* entry = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY(entry), 1);
	GtkWidget* label = gtk_label_new_with_mnemonic(_("Press macro shortcut..."));
	GtkWidget* hbox = gtk_hbox_new (FALSE, 0);	
	
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
	
	gtk_widget_set_size_request(entry, 0, 0);
	
	gtk_window_set_title(GTK_WINDOW(window), _("Press shortcut"));
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

	gtk_container_add(GTK_CONTAINER(window), hbox);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
	gtk_box_pack_end_defaults(GTK_BOX(hbox), entry);
	g_signal_connect (G_OBJECT (window), "key-press-event",
			  G_CALLBACK (on_shortcut_pressed), plugin);
	gtk_widget_grab_focus (entry);
	
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
	gtk_widget_show_all(window);
}

void on_menu_add_macro (GtkAction * action, MacroPlugin * plugin)
{
	MacroEdit* add = MACRO_EDIT(macro_edit_new(MACRO_ADD, plugin->macro_db));
	gchar* selection = NULL;
	if (plugin->current_editor != NULL)
	{
		selection = 
			ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION(plugin->current_editor), NULL);
	}
	if (selection != NULL && strlen(selection))
		macro_edit_set_macro(add, selection);
	gtk_widget_show(GTK_WIDGET(add));
}

void on_menu_manage_macro (GtkAction * action, MacroPlugin * plugin)
{
	if (plugin->macro_dialog == NULL)
		plugin->macro_dialog = macro_dialog_new (plugin);
	gtk_widget_show (plugin->macro_dialog);
}
