/*
 *  macro_dialog.c (c) 2005 Johannes Schmid
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

#include "macro-dialog.h"
#include "macro-edit.h"
#include "macro-actions.h"
#include <libanjuta/interfaces/ianjuta-editor.h>

enum
{
	OK,
	CANCEL
};

static gboolean
on_macro_dialog_key_press_event(GtkWidget *widget, GdkEventKey *event,
                               MacroPlugin * plugin)
{
	if (event->keyval == GDK_Escape)
	{
		gtk_widget_hide (GTK_WIDGET (plugin->macro_dialog));
		return TRUE;
	}
	return FALSE;
}

static void
on_ok_clicked (MacroPlugin * plugin)
{
	MacroDialog *dialog = MACRO_DIALOG (plugin->macro_dialog);
	GtkTreeSelection *selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (dialog->macro_tree));
	
	GtkTreeModel* model = macro_db_get_model(dialog->macro_db);
	GtkTreeIter iter;
	gchar* text;
	gint offset = 0;
	
	g_return_if_fail (plugin != NULL);
	g_return_if_fail (model != NULL);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;
	text = macro_db_get_macro(plugin, dialog->macro_db, &iter, &offset);
	if (text)
	{
		if (plugin->current_editor != NULL)
		{
			gint i;
			IAnjutaIterable *pos;
			pos = ianjuta_editor_get_position (IANJUTA_EDITOR(plugin->current_editor),
											   NULL);
			ianjuta_editor_insert (IANJUTA_EDITOR(plugin->current_editor),
			                       pos, text, -1, NULL);
			for (i = 0; i < offset; i++)
				ianjuta_iterable_next (pos, NULL);
			ianjuta_editor_goto_position (IANJUTA_EDITOR(plugin->current_editor), 
			                              pos, NULL);
			g_object_unref (pos);
		}
		g_free(text);
		gtk_widget_hide (plugin->macro_dialog);
	}
}

static void
on_cancel_clicked (MacroPlugin * plugin)
{
	g_return_if_fail (plugin != NULL);
	gtk_widget_hide (GTK_WIDGET (plugin->macro_dialog));
}

static void
on_dialog_response (GtkWidget * dialog, gint response, MacroPlugin * plugin)
{
	g_return_if_fail (plugin != NULL);
	switch (response)
	{
	case OK:
		on_ok_clicked (plugin);
		break;
	case CANCEL:
		on_cancel_clicked (plugin);
		break;
	default:
		break;
	}
}

static void
on_macro_selection_changed (GtkTreeSelection * selection,
			    MacroDialog * dialog)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (dialog != NULL);
	GtkTreeIter iter;
	GtkTreeModel *model = macro_db_get_model (dialog->macro_db);
	GtkWidget *edit;
	GtkWidget *remove;
	GtkTextBuffer *text_buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW
					  (dialog->preview_text));

	edit = glade_xml_get_widget (dialog->gxml, "macro_edit");
	remove = glade_xml_get_widget (dialog->gxml, "macro_remove");

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *name;
		gchar *category;
		gchar shortcut;
		gchar *text;
		gchar *details;
		gchar *details_utf8;
		gboolean is_category;
		gboolean predefined;
		gint offset;

		gtk_tree_model_get (model, &iter,
				    MACRO_NAME, &name,
				    MACRO_CATEGORY, &category,
				    MACRO_SHORTCUT, &shortcut,
				    MACRO_IS_CATEGORY, &is_category,
				    MACRO_PREDEFINED, &predefined, -1);
		if (!is_category)
		{
			details = g_strdup_printf ("Name:\t %s\n"
						   "Category:\t %s\nShortcut:\t %c\n",
						   name, category, shortcut);
			/* Keep pango happy */
			details_utf8 = g_utf8_normalize (details, -1,
							 G_NORMALIZE_DEFAULT_COMPOSE);

			gtk_label_set_text (GTK_LABEL (dialog->details_label),
					    details_utf8);
			text = macro_db_get_macro(dialog->plugin, dialog->macro_db, &iter, &offset);
			if (text != NULL)
			{
				gtk_text_buffer_set_text (text_buffer, text, -1);

				gtk_widget_set_sensitive (edit, !predefined);
				gtk_widget_set_sensitive (remove, !predefined);
				g_free (text);
				return;
			}
		}
	}
	gtk_label_set_text (GTK_LABEL (dialog->details_label), "");
	gtk_text_buffer_set_text (text_buffer, "", -1);
	gtk_widget_set_sensitive (edit, FALSE);
	gtk_widget_set_sensitive (remove, FALSE);
}

static void
on_add_clicked (GtkButton * add, MacroDialog * dialog)
{
	g_return_if_fail (dialog != NULL);
	MacroEdit *edit =
		MACRO_EDIT (macro_edit_new (MACRO_ADD, dialog->macro_db));
	gtk_window_set_modal (GTK_WINDOW (edit), TRUE);
	gtk_widget_show (GTK_WIDGET (edit));
}

static void
on_remove_clicked (GtkButton * remove, MacroDialog * dialog)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (dialog->macro_tree));
	GtkTreeModel *model = macro_db_get_model (dialog->macro_db);
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gboolean is_category;
		gboolean predefined;
		gtk_tree_model_get (model, &iter,
				    MACRO_PREDEFINED, &predefined,
				    MACRO_IS_CATEGORY, &is_category, -1);
		if (is_category || predefined)
			return;
		else
		{
			macro_db_remove (dialog->macro_db, &iter);
		}
	}
}

static void
on_edit_clicked (GtkButton * ok, MacroDialog * dialog)
{
	GtkTreeSelection *select = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (dialog->macro_tree));
	MacroEdit *edit;

	g_return_if_fail (dialog != NULL);

	edit = MACRO_EDIT (macro_edit_new (MACRO_EDIT, dialog->macro_db));
	if (edit == NULL)
		return;
	macro_edit_fill (edit, select);
	gtk_window_set_modal (GTK_WINDOW (edit), TRUE);
	gtk_widget_show (GTK_WIDGET (edit));
}

static gpointer parent_class;

static void
macro_dialog_dispose (GObject * object)
{
	MacroDialog *dialog = MACRO_DIALOG (object);
	if (dialog->gxml)
	{
		g_object_unref (dialog->gxml);
		dialog->gxml = NULL;
	}
	if (dialog->macro_db)
	{
		g_object_unref (dialog->macro_db);
		dialog->macro_db = NULL;
	}
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
macro_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
macro_dialog_class_init (MacroDialogClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = macro_dialog_dispose;
	object_class->finalize = macro_dialog_finalize;
}

static void
macro_dialog_init (MacroDialog * dialog)
{
	GtkTreeSelection *select;
	dialog->gxml = glade_xml_new (GLADE_FILE, "macro_dialog_table", NULL);
	GtkWidget *table =
		glade_xml_get_widget (dialog->gxml, "macro_dialog_table");

	g_return_if_fail (dialog != NULL);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), table);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), "Insert", OK,
				GTK_STOCK_CANCEL, CANCEL, NULL);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Insert macro"));

	dialog->macro_tree =
		glade_xml_get_widget (dialog->gxml, "macro_tree");
	dialog->preview_text =
		glade_xml_get_widget (dialog->gxml, "macro_preview");
	dialog->details_label =
		glade_xml_get_widget (dialog->gxml, "macro_details");
			
	glade_xml_signal_connect_data (dialog->gxml, "on_edit_clicked",
				       G_CALLBACK (on_edit_clicked), dialog);
	glade_xml_signal_connect_data (dialog->gxml, "on_add_clicked",
				       G_CALLBACK (on_add_clicked), dialog);
	glade_xml_signal_connect_data (dialog->gxml, "on_remove_clicked",
				       G_CALLBACK (on_remove_clicked),
				       dialog);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW
					      (dialog->macro_tree));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (on_macro_selection_changed), dialog);
}

GType
macro_dialog_get_type (void)
{
	static GType macro_dialog_type = 0;
	if (!macro_dialog_type)
	{
		static const GTypeInfo md_info = {
			sizeof (MacroDialogClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) macro_dialog_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (MacroDialog),
			0,
			(GInstanceInitFunc) macro_dialog_init,
		};
		macro_dialog_type =
			g_type_register_static (GTK_TYPE_DIALOG,
						"MacroDialog", &md_info, 0);
	}
	return macro_dialog_type;
}

GtkWidget *
macro_dialog_new (MacroPlugin * plugin)
{
	g_return_val_if_fail (plugin != NULL, NULL);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	MacroDialog *dialog =
		MACRO_DIALOG (g_object_new (macro_dialog_get_type (), NULL));
	
	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (on_dialog_response), plugin);
	g_signal_connect(G_OBJECT(dialog), "key-press-event",
			G_CALLBACK(on_macro_dialog_key_press_event), plugin);
	
	plugin->macro_dialog = GTK_WIDGET (dialog);
	dialog->plugin = plugin;	
	dialog->macro_db = plugin->macro_db;
	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->macro_tree),
				 macro_db_get_model (dialog->macro_db));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Macro name",
							   renderer,
							   "text", MACRO_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->macro_tree),
				     column);
	
	return GTK_WIDGET (dialog);
}
