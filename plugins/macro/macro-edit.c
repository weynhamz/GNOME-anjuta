/*
 * macro_edit.c (c) 2005 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "macro-edit.h"

enum
{
	OK,
	CANCEL
};

static gboolean
on_macro_edit_key_press_event(GtkWidget *widget, GdkEventKey *event,
                               gpointer user_data)
{
	if (event->keyval == GDK_Escape)
	{
		gtk_widget_hide (widget);
		return TRUE;
	}
	return FALSE;
}

static void
fill_category_combo (MacroEdit * edit, GtkWidget * combo)
{
	GtkTreeIter iter_user;
	GtkTreeIter iter_cat;
	GtkTreeModel *tree_model = macro_db_get_model (edit->macro_db);

	if (gtk_tree_model_get_iter_first (tree_model, &iter_user))
	{
		gtk_tree_model_iter_next (tree_model, &iter_user);
		if (gtk_tree_model_iter_children
		    (tree_model, &iter_cat, &iter_user))
		{
			do
			{
				gchar *name;
				gboolean is_category;
				gtk_tree_model_get (tree_model, &iter_cat,
						    MACRO_NAME, &name,
						    MACRO_IS_CATEGORY,
						    &is_category, -1);
				if (is_category && name)
				{
					gtk_combo_box_append_text
						(GTK_COMBO_BOX
						 (edit->category_entry),
						 name);
				}
			}
			while (gtk_tree_model_iter_next
			       (tree_model, &iter_cat));
		}
	}
}

static void
on_add_ok_clicked (MacroEdit * edit)
{
	GtkTextIter begin, end;
	GtkTextBuffer *buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (edit->text));
	gchar *text;

	g_return_if_fail (edit != NULL);
	gtk_text_buffer_get_start_iter (buffer, &begin);
	gtk_text_buffer_get_end_iter (buffer, &end);
	text = gtk_text_buffer_get_text (buffer, &begin, &end, -1);

	macro_db_add (edit->macro_db,
		      gtk_entry_get_text (GTK_ENTRY (edit->name_entry)),
		      gtk_entry_get_text (GTK_ENTRY
					  (GTK_BIN (edit->category_entry)->
					   child)),
		      gtk_entry_get_text (GTK_ENTRY (edit->shortcut_entry)),
		      text);
	gtk_widget_destroy (GTK_WIDGET (edit));
}

static void
on_add_cancel_clicked (MacroEdit * edit)
{
	gtk_widget_hide (GTK_WIDGET (edit));
	gtk_widget_destroy (GTK_WIDGET (edit));
}

static void
on_edit_ok_clicked (MacroEdit * edit)
{
	GtkTextIter begin, end;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTextBuffer *buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (edit->text));
	gchar *text;

	g_return_if_fail (edit != NULL);
	gtk_tree_selection_get_selected (edit->select, &model, &iter);
	gtk_text_buffer_get_start_iter (buffer, &begin);
	gtk_text_buffer_get_end_iter (buffer, &end);
	text = gtk_text_buffer_get_text (buffer, &begin, &end, -1);

	macro_db_change (edit->macro_db, &iter,
			 gtk_entry_get_text (GTK_ENTRY (edit->name_entry)),
			 gtk_entry_get_text (GTK_ENTRY
					     (GTK_BIN (edit->category_entry)->
					      child)),
			 gtk_entry_get_text (GTK_ENTRY
					     (edit->shortcut_entry)), text);
	gtk_widget_destroy (GTK_WIDGET (edit));
}

static void
on_edit_cancel_clicked (MacroEdit * edit)
{
	on_add_cancel_clicked (edit);
}

static void
on_dialog_response (GtkWidget * dialog, gint response, MacroEdit * edit)
{
	if (edit->type == MACRO_EDIT)
	{
		switch (response)
		{
		case OK:
			on_edit_ok_clicked (edit);
			break;
		case CANCEL:
			on_edit_cancel_clicked (edit);
			break;
		}
	}
	else if (edit->type == MACRO_ADD)
	{
		switch (response)
		{
		case OK:
			on_add_ok_clicked (edit);
			break;
		case CANCEL:
			on_add_cancel_clicked (edit);
			break;
		}
	}
}

static gpointer parent_class;

static void
macro_edit_dispose (GObject * edit)
{
	//MacroEdit *medit = MACRO_EDIT (edit);
	//g_object_unref(medit->gxml);
	G_OBJECT_CLASS (parent_class)->dispose (edit);
}

static void
macro_edit_finalize (GObject * edit)
{
	//MacroEdit *medit = MACRO_EDIT (edit);
	//g_object_unref(medit->gxml);
	G_OBJECT_CLASS (parent_class)->finalize (edit);
}

static void
macro_edit_class_init (MacroEditClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = macro_edit_dispose;
	object_class->finalize = macro_edit_finalize;
}

static void
macro_edit_init (MacroEdit * edit)
{
	GtkWidget *table;

	edit->gxml = glade_xml_new (GLADE_FILE, "macro_edit_table", NULL);

	table = glade_xml_get_widget (edit->gxml, "macro_edit_table");
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (edit)->vbox), table);
	gtk_dialog_add_buttons (GTK_DIALOG (edit), GTK_STOCK_OK, OK,
				GTK_STOCK_CANCEL, CANCEL, NULL);
	g_signal_connect (G_OBJECT (edit), "response",
			  G_CALLBACK (on_dialog_response), edit);
	gtk_window_set_title (GTK_WINDOW (edit), _("Add/Edit macro"));

	edit->name_entry = glade_xml_get_widget (edit->gxml, "macro_name");
	edit->category_entry = gtk_combo_box_entry_new_text ();
	gtk_widget_show (edit->category_entry);
	gtk_table_attach_defaults (GTK_TABLE (table),
				   edit->category_entry, 1, 2, 2, 3);
	edit->shortcut_entry =
		glade_xml_get_widget (edit->gxml, "macro_shortcut");
	edit->text = glade_xml_get_widget (edit->gxml, "macro_text");

}

GType
macro_edit_get_type (void)
{
	static GType macro_edit_type = 0;
	if (!macro_edit_type)
	{
		static const GTypeInfo me_info = {
			sizeof (MacroEditClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) macro_edit_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (MacroEdit),
			0,
			(GInstanceInitFunc) macro_edit_init,
		};
		macro_edit_type =
			g_type_register_static (GTK_TYPE_DIALOG, "MacroEdit",
						&me_info, 0);
	}
	return macro_edit_type;
}

GtkWidget *
macro_edit_new (int type, MacroDB * db)
{
	MacroEdit *edit =
		MACRO_EDIT (g_object_new (macro_edit_get_type (), NULL));
	edit->type = type;
	edit->macro_db = db;
	fill_category_combo (edit, edit->category_entry);
	
	g_signal_connect(G_OBJECT(edit), "key-press-event",
			G_CALLBACK(on_macro_edit_key_press_event), NULL);
	
	return GTK_WIDGET (edit);
}

void
macro_edit_fill (MacroEdit * edit, GtkTreeSelection * select)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		gchar *name;
		gchar *category;
		gchar shortcut;
		gchar *text;
		gboolean is_category;
		gboolean predefined;



		gtk_tree_model_get (model, &iter,
				    MACRO_NAME, &name,
				    MACRO_CATEGORY, &category,
				    MACRO_SHORTCUT, &shortcut,
				    MACRO_TEXT, &text,
				    MACRO_IS_CATEGORY, &is_category,
				    MACRO_PREDEFINED, &predefined, -1);
		if (!is_category && !predefined)
		{
			GtkTextBuffer *text_buffer;
			gchar *shortcut_string =
				g_strdup_printf ("%c", shortcut);
			gtk_entry_set_text (GTK_ENTRY (edit->name_entry),
					    name);
			gtk_entry_set_text (GTK_ENTRY
					    (GTK_BIN (edit->category_entry)->
					     child), category);
			gtk_entry_set_text (GTK_ENTRY (edit->shortcut_entry),
					    shortcut_string);
			g_free (shortcut_string);

			text_buffer =
				gtk_text_view_get_buffer (GTK_TEXT_VIEW
							  (edit->text));
			gtk_text_buffer_set_text (text_buffer, text, -1);
		}
	}
	edit->select = select;
}

void macro_edit_set_macro (MacroEdit* edit, const gchar* macro)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit->text));
	gtk_text_buffer_set_text(buffer, macro, strlen(macro));
}
