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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "macro-dialog.h"
#include "macro-edit.h"
#include <libanjuta/interfaces/ianjuta-editor.h>

static void on_ok_clicked (MacroPlugin * plugin);
static void on_cancel_clicked (MacroPlugin * dialog);
static void on_dialog_response (GtkWidget * dialog, gint response,
				MacroPlugin * plugin);

static void on_macro_selection_changed (GtkTreeSelection * selection,
					MacroDialog * dialog);
static void on_add_clicked (GtkButton * add, MacroDialog * dialog);
static void on_remove_clicked (GtkButton * remove, MacroDialog * dialog);
static void on_edit_clicked (GtkButton * edit, MacroDialog * dialog);

static gboolean on_shortcut_pressed (GtkWidget * tree, GdkEventKey * event,
				     MacroPlugin * plugin);

static gboolean match_shortcut (MacroPlugin * plugin, GtkTreeIter * iter,
				gchar key, gchar shortcut);

static void macro_dialog_class_init (MacroDialogClass * klass);
static void macro_dialog_init (MacroDialog * dialog);
static void macro_dialog_dispose (GObject * object);

static gpointer parent_class;

enum
{
	OK,
	CANCEL
};

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

static void
macro_dialog_class_init (MacroDialogClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = macro_dialog_dispose;
}

static void
macro_dialog_init (MacroDialog * dialog)
{
	GtkTreeSelection *select;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	dialog->gxml = glade_xml_new (GLADE_FILE, "macro_dialog_table", NULL);
	GtkWidget *table =
		glade_xml_get_widget (dialog->gxml, "macro_dialog_table");

	g_return_if_fail (dialog != NULL);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), table);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, OK,
				GTK_STOCK_CANCEL, CANCEL, NULL);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 300);
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

	dialog->macro_db = macro_db_new ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->macro_tree),
				 macro_db_get_model (dialog->macro_db));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Macro name",
							   renderer,
							   "text", MACRO_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->macro_tree),
				     column);
}

GtkWidget *
macro_dialog_new (MacroPlugin * plugin)
{
	g_return_val_if_fail (plugin != NULL, NULL);
	MacroDialog *dialog =
		MACRO_DIALOG (g_object_new (macro_dialog_get_type (), NULL));
	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (on_dialog_response), plugin);

	gtk_widget_grab_focus (dialog->macro_tree);
	g_signal_connect (G_OBJECT (dialog->macro_tree), "key-press-event",
			  G_CALLBACK (on_shortcut_pressed), plugin);

	plugin->macro_dialog = GTK_WIDGET (dialog);
	return GTK_WIDGET (dialog);
}

void
macro_dialog_dispose (GObject * object)
{
	MacroDialog *dialog = MACRO_DIALOG (object);
	g_object_unref (dialog->gxml);
	g_object_unref (dialog->macro_db);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT (object)));
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

		gtk_tree_model_get (model, &iter,
				    MACRO_NAME, &name,
				    MACRO_CATEGORY, &category,
				    MACRO_SHORTCUT, &shortcut,
				    MACRO_TEXT, &text,
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

			gtk_text_buffer_set_text (text_buffer, text, -1);

			gtk_widget_set_sensitive (edit, !predefined);
			gtk_widget_set_sensitive (remove, !predefined);
			return;
		}
	}
	gtk_label_set_text (GTK_LABEL (dialog->details_label), "");
	gtk_text_buffer_set_text (text_buffer, "", -1);
	gtk_widget_set_sensitive (edit, FALSE);
	gtk_widget_set_sensitive (remove, FALSE);
}

static void
on_ok_clicked (MacroPlugin * plugin)
{
	MacroDialog *dialog = MACRO_DIALOG (plugin->macro_dialog);
	GtkTreeSelection *selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (dialog->macro_tree));
	GtkTreeModel *model = macro_db_get_model (dialog->macro_db);
	GtkTreeIter iter;

	g_return_if_fail (plugin != NULL);
	g_return_if_fail (model != NULL);

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *text;
		gboolean is_category;
		gtk_tree_model_get (model, &iter,
				    MACRO_TEXT, &text,
				    MACRO_IS_CATEGORY, &is_category, -1);
		if (is_category)
			return;
		else
		{
			const int CURRENT_POS = -1;
			if (plugin->current_editor != NULL)
			{
				ianjuta_editor_insert (IANJUTA_EDITOR
						       (plugin->
							current_editor),
						       CURRENT_POS, text, -1,
						       NULL);
			}
			gtk_widget_hide (plugin->macro_dialog);
		}
	}
}

static void
on_cancel_clicked (MacroPlugin * plugin)
{
	g_return_if_fail (plugin != NULL);
	gtk_widget_hide (GTK_WIDGET (plugin->macro_dialog));
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

static gboolean
on_shortcut_pressed (GtkWidget * tree, GdkEventKey * event,
		     MacroPlugin * plugin)
{
	gchar key;
	GtkTreeIter parent;
	GtkTreeIter category;
	GtkTreeIter macro;
	MacroDialog *dialog = MACRO_DIALOG (plugin->macro_dialog);
	GtkTreeModel *model = macro_db_get_model (dialog->macro_db);
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
		if (gtk_tree_model_iter_children (model, &category, &parent))
		{
			do
			{
				gboolean is_category;
				gchar shortcut;
				gtk_tree_model_get (model, &category,
						    MACRO_CATEGORY,
						    &is_category, -1);
				if (is_category
				    && gtk_tree_model_iter_children (model,
								     &macro,
								     &category))
				{
					do
					{
						MacroDialog *dialog =
							MACRO_DIALOG (plugin->
								      macro_dialog);
						gtk_tree_model_get (model,
								    &macro,
								    MACRO_SHORTCUT,
								    &shortcut,
								    -1);
						if (match_shortcut
						    (plugin, &macro, key,
						     shortcut))
							return FALSE;
					}
					while (gtk_tree_model_iter_next
					       (model, &macro));
				}
				else
				{
					gtk_tree_model_get (model, &category,
							    MACRO_SHORTCUT,
							    &shortcut, -1);
					if (match_shortcut
					    (plugin, &category, key,
					     shortcut))
						return FALSE;
				}
			}
			while (gtk_tree_model_iter_next (model, &category));
		}
	}
	while (gtk_tree_model_iter_next (model, &parent));
	return TRUE;
}

static gboolean
match_shortcut (MacroPlugin * plugin, GtkTreeIter * iter,
		gchar key, gchar shortcut)
{
	MacroDialog *dialog = MACRO_DIALOG (plugin->macro_dialog);
	if (key == shortcut)
	{
		GtkTreeView *tree = GTK_TREE_VIEW (dialog->macro_tree);
		GtkTreeSelection *select = gtk_tree_view_get_selection (tree);
		gtk_tree_selection_select_iter (select, iter);
		on_ok_clicked (plugin);
		return TRUE;
	}
	return FALSE;
}
