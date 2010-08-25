/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-editor.c
    Copyright (C) Dragos Dena 2010

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#include "snippets-editor.h"
#include "snippet-variables-store.h"
#include <string.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>

#define EDITOR_UI      PACKAGE_DATA_DIR"/glade/snippets-editor.ui"

#define ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_EDITOR, SnippetsEditorPrivate))

#define LOCAL_TYPE_STR          "Snippet"
#define GLOBAL_TYPE_STR         "Anjuta"

#define UNDEFINED_BG_COLOR      "#ffbaba"

#define NAME_COL_TITLE          _("Name")
#define TYPE_COL_TITLE          _("Type")
#define DEFAULT_COL_TITLE       _("Default value")
#define INSTANT_COL_TITLE       _("Instant value")

#define MIN_NAME_COL_WIDTH      120

#define NEW_VAR_NAME            "new_variable"

#define GROUPS_COL_NAME         0

#define ERROR_LANG_NULL           _("<b>Error:</b> You must choose at least one language for the snippet!")
#define ERROR_LANG_CONFLICT       _("<b>Error:</b> The trigger key is already in use for one of the languages!")
#define ERROR_TRIGGER_NOT_VALID   _("<b>Error:</b> The trigger key can only contain alfanumeric characters and _ !")
#define ERROR_TRIGGER_NULL        _("<b>Error:</b> You haven't entered a trigger key for the snippet!")

#define SNIPPET_VAR_START                 "${"
#define SNIPPET_VAR_END                    "}"
#define IS_SNIPPET_VAR_START(text, index)  (text[index] == '$' && text[index + 1] == '{')
#define IS_SNIPPET_VAR_END(text, index)    (text[index] == '}')

struct _SnippetsEditorPrivate
{
	SnippetsDB *snippets_db;
	AnjutaSnippet *snippet;
	AnjutaSnippet *backup_snippet;
	GtkListStore *group_store;
	GtkListStore *lang_store;

	/* A tree model with 2 entries: LOCAL_TYPE_STR and GLOBAL_TYPE_STR to be used
	   by the variables view on the type column */
	GtkListStore *type_model;
	
	/* Snippet Content editor widgets */
	GtkTextView *content_text_view; /* TODO - this should be changed later with GtkSourceView */
	GtkToggleButton *preview_button;

	/* Snippet properties widgets */
	GtkEntry *name_entry;
	GtkEntry *trigger_entry;
	GtkEntry *keywords_entry;
	GtkComboBox *languages_combo_box;
	GtkComboBox *snippets_group_combo_box;
	GtkImage *languages_notify;
	GtkImage *group_notify;
	GtkImage *trigger_notify;
	GtkImage *name_notify;

	/* If one of the following variables is TRUE, then we have an error and the Save Button
	   will be insensitive */
	gboolean languages_error;
	gboolean group_error;
	gboolean trigger_error;

	/* Variables widgets */
	GtkTreeView *variables_view;
	GtkButton *variable_add_button;
	GtkButton *variable_remove_button;
	GtkButton *variable_insert_button;

	/* Variables tree model */
	SnippetVarsStore *vars_store;
	GtkTreeModel *vars_store_sorted;

	/* Variables view cell renderers */
	GtkCellRenderer *name_combo_cell;
	GtkCellRenderer *type_combo_cell;
	GtkCellRenderer *type_pixbuf_cell;
	GtkCellRenderer *default_text_cell;
	GtkCellRenderer *instant_text_cell;
	
	/* Editor widgets */
	GtkButton *save_button;
	GtkButton *close_button;
	GtkAlignment *editor_alignment;

	/* So it will guard deleting the priv->snippet by calling snippets_editor_set_snippet
	   with snippet == NULL after deleting the backup_snippet in the saving handler. */
	gboolean saving_snippet;

};

enum
{
	VARS_VIEW_COL_NAME = 0,
	VARS_VIEW_COL_TYPE,
	VARS_VIEW_COL_DEFAULT,
	VARS_VIEW_COL_INSTANT
};

enum
{
	LANG_MODEL_COL_IN_SNIPPET = 0,
	LANG_MODEL_COL_NAME,
	LANG_MODEL_COL_N
};

/* Handlers */
static void  on_preview_button_toggled           (GtkToggleButton *preview_button,
                                                  gpointer user_data);
static void  on_save_button_clicked              (GtkButton *save_button,
                                                  gpointer user_data);
static void  on_close_button_clicked             (GtkButton *close_button,
                                                  gpointer user_data);
static void  on_name_combo_cell_edited           (GtkCellRendererText *cell,
                                                  gchar *path_string,
                                                  gchar *new_string,
                                                  gpointer user_data);
static void  on_type_combo_cell_changed          (GtkCellRendererCombo *cell,
                                                  gchar *path_string,
                                                  gchar *new_string,
                                                  gpointer user_data);
static void  on_default_text_cell_edited         (GtkCellRendererText *cell,
                                                  gchar *path_string,
                                                  gchar *new_string,
                                                  gpointer user_data);
static void  on_variables_view_row_activated     (GtkTreeView *tree_view,
                                                  GtkTreePath *path,
                                                  GtkTreeViewColumn *col,
                                                  gpointer user_data);
static void  on_variable_add_button_clicked      (GtkButton *variable_add_button,
                                                  gpointer user_data);
static void  on_variable_remove_button_clicked   (GtkButton *variable_remove_button,
                                                  gpointer user_data);
static void  on_variable_insert_button_clicked   (GtkButton *variable_insert_button,
                                                  gpointer user_data);
static void  on_variables_view_selection_changed (GtkTreeSelection *selection,
                                                  gpointer user_data);
static void  on_snippets_group_combo_box_changed (GtkComboBox *combo_box,
                                                  gpointer user_data);
static void  on_languages_combo_box_changed      (GtkComboBox *combo_box,
                                                  gpointer user_data);
static void  on_trigger_entry_text_changed       (GObject *entry_obj,
                                                  GParamSpec *param_spec,
                                                  gpointer user_data);
static void  on_name_entry_text_changed          (GObject *entry_obj,
                                                  GParamSpec *param_spec,
                                                  gpointer user_data);

G_DEFINE_TYPE (SnippetsEditor, snippets_editor, GTK_TYPE_HBOX);


static void
snippets_editor_dispose (GObject *object)
{
	SnippetsEditorPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (object));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (object);

	if (ANJUTA_IS_SNIPPET (priv->snippet))
		g_object_unref (priv->snippet);

	G_OBJECT_CLASS (snippets_editor_parent_class)->dispose (G_OBJECT (object));
}

static void
snippets_editor_class_init (SnippetsEditorClass* klass)
{
	GObjectClass *g_object_class = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR_CLASS (klass));

	g_object_class = G_OBJECT_CLASS (klass);
	g_object_class->dispose = snippets_editor_dispose;

	/* The SnippetsEditor saved the snippet. Basically, the SnippetsBrowser should
	   focus on the row where the snippet was saved. This is needed because when the
	   snippet is saved, the old one is deleted and the new one is inserted. The given
	   object is the newly inserted snippet. */
	g_signal_new ("snippet-saved",
	              ANJUTA_TYPE_SNIPPETS_EDITOR,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsEditorClass, snippet_saved),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__OBJECT,
	              G_TYPE_NONE,
	              1,
	              G_TYPE_OBJECT,
	              NULL);

	g_signal_new ("close-request",
	              ANJUTA_TYPE_SNIPPETS_EDITOR,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsEditorClass, close_request),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE,
	              0,
	              NULL);
	
	g_type_class_add_private (klass, sizeof (SnippetsEditorPrivate));
}

static void
snippets_editor_init (SnippetsEditor* snippets_editor)
{
	SnippetsEditorPrivate* priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	snippets_editor->priv = priv;

	/* Initialize the private field */
	priv->snippets_db = NULL;
	priv->snippet = NULL;	
	priv->backup_snippet = NULL;
	priv->group_store = NULL;
	priv->lang_store = NULL;
	
	priv->type_model = NULL;

	priv->content_text_view = NULL;
	priv->preview_button = NULL;

	priv->name_entry = NULL;
	priv->trigger_entry = NULL;
	priv->languages_combo_box = NULL;
	priv->snippets_group_combo_box = NULL;
	priv->keywords_entry = NULL;
	priv->languages_notify = NULL;
	priv->group_notify = NULL;
	priv->trigger_notify = NULL;
	
	priv->variables_view = NULL;
	priv->variable_add_button = NULL;
	priv->variable_remove_button = NULL;
	priv->variable_insert_button = NULL;

	priv->vars_store = NULL;
	priv->vars_store_sorted = NULL;

	priv->name_combo_cell = NULL;
	priv->type_combo_cell = NULL;
	priv->type_pixbuf_cell = NULL;
	priv->default_text_cell = NULL;
	priv->instant_text_cell = NULL;

	priv->save_button = NULL;
	priv->close_button = NULL;
	priv->editor_alignment = NULL;

	priv->saving_snippet = FALSE;
}

static void
load_snippets_editor_ui (SnippetsEditor *snippets_editor)
{
	GtkBuilder *bxml = NULL;
	SnippetsEditorPrivate *priv = NULL;
	GError *error = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);
	
	/* Load the UI file */
	bxml = gtk_builder_new ();
	if (!gtk_builder_add_from_file (bxml, EDITOR_UI, &error))
	{
		g_warning ("Couldn't load editor ui file: %s", error->message);
		g_error_free (error);
	}

	/* Edit content widgets */
	priv->content_text_view = GTK_TEXT_VIEW (gtk_builder_get_object (bxml, "content_text_view"));
	priv->preview_button = GTK_TOGGLE_BUTTON (gtk_builder_get_object (bxml, "preview_button"));
	g_return_if_fail (GTK_IS_TEXT_VIEW (priv->content_text_view));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (priv->preview_button));

	/* Edit properties widgets */
	priv->name_entry = GTK_ENTRY (gtk_builder_get_object (bxml, "name_entry"));
	priv->trigger_entry = GTK_ENTRY (gtk_builder_get_object (bxml, "trigger_entry"));
	priv->languages_combo_box = GTK_COMBO_BOX (gtk_builder_get_object (bxml, "languages_combo_box"));
	priv->snippets_group_combo_box = GTK_COMBO_BOX (gtk_builder_get_object (bxml, "snippets_group_combo_box"));
	priv->languages_notify = GTK_IMAGE (gtk_builder_get_object (bxml, "languages_notify"));
	priv->group_notify = GTK_IMAGE (gtk_builder_get_object (bxml, "group_notify"));
	priv->trigger_notify = GTK_IMAGE (gtk_builder_get_object (bxml, "trigger_notify"));
	priv->name_notify = GTK_IMAGE (gtk_builder_get_object (bxml, "name_notify"));
	priv->keywords_entry = GTK_ENTRY (gtk_builder_get_object (bxml, "keywords_entry"));
	g_return_if_fail (GTK_IS_ENTRY (priv->name_entry));
	g_return_if_fail (GTK_IS_ENTRY (priv->trigger_entry));
	g_return_if_fail (GTK_IS_COMBO_BOX (priv->languages_combo_box));
	g_return_if_fail (GTK_IS_COMBO_BOX (priv->snippets_group_combo_box));
	g_return_if_fail (GTK_IS_IMAGE (priv->languages_notify));
	g_return_if_fail (GTK_IS_IMAGE (priv->group_notify));
	g_return_if_fail (GTK_IS_ENTRY (priv->keywords_entry));
	
	/* Edit variables widgets */
	priv->variables_view = GTK_TREE_VIEW (gtk_builder_get_object (bxml, "variables_view"));
	priv->variable_add_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_add_button"));
	priv->variable_remove_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_remove_button"));
	priv->variable_insert_button = GTK_BUTTON (gtk_builder_get_object (bxml, "variable_insert_button"));
	g_return_if_fail (GTK_IS_TREE_VIEW (priv->variables_view));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_add_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_remove_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->variable_insert_button));

	/* General widgets */
	priv->save_button = GTK_BUTTON (gtk_builder_get_object (bxml, "save_button"));
	priv->close_button = GTK_BUTTON (gtk_builder_get_object (bxml, "close_button"));
	priv->editor_alignment = GTK_ALIGNMENT (gtk_builder_get_object (bxml, "editor_alignment"));
	g_return_if_fail (GTK_IS_BUTTON (priv->save_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->close_button));
	g_return_if_fail (GTK_IS_ALIGNMENT (priv->editor_alignment));

	/* Add the gtk_alignment as the child of the snippets editor */
	gtk_box_pack_start (GTK_BOX (snippets_editor),
	                    GTK_WIDGET (priv->editor_alignment),
	                    TRUE,
	                    TRUE,
	                    0);

	g_object_unref (bxml);
}

/* Variables View cell data functions and the sort function	*/

static gint
compare_var_in_snippet (gboolean in_snippet1, gboolean in_snippet2)
{
	/* Those that are in snippet go before those that aren't */
	if ((in_snippet1 && in_snippet2) || (!in_snippet1 && !in_snippet2))
		return 0;
	if (in_snippet1)
		return -1;
	return 1;
}

static gint
vars_store_sort_func (GtkTreeModel *vars_store,
                      GtkTreeIter *iter1,
                      GtkTreeIter *iter2,
                      gpointer user_data)
{
	gboolean in_snippet1 = FALSE, in_snippet2 = FALSE;
	gchar *name1 = NULL, *name2 = NULL;
	gint compare_value = 0;
	
	/* Get the values from the model */
	gtk_tree_model_get (vars_store, iter1,
	                    VARS_STORE_COL_NAME, &name1,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet1,
	                    -1);
	gtk_tree_model_get (vars_store, iter2,
	                    VARS_STORE_COL_NAME, &name2,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet2,
	                    -1);

	/* We first check if both variables are in the snippet */
	compare_value = compare_var_in_snippet (in_snippet1, in_snippet2);
	
	/* If we didn't got a compare_value until this point, we compare by name */
	if (!compare_value)
		compare_value = g_strcmp0 (name1, name2);

	g_free (name1);
	g_free (name2);
	
	return compare_value;
}

static void
set_cell_colors (GtkCellRenderer *cell, 
                 SnippetVariableType type,
                 gboolean undefined)
{
	if (undefined && type == SNIPPET_VAR_TYPE_GLOBAL)
		g_object_set (cell, "cell-background", UNDEFINED_BG_COLOR, NULL);
	else
		g_object_set (cell, "cell-background-set", FALSE, NULL);
}

static void
focus_on_in_snippet_variable (GtkTreeView *vars_view,
                              GtkTreeModel *vars_model,
                              const gchar *var_name,
                              GtkTreeViewColumn *col,
                              gboolean start_editing)
{
	GtkTreeIter iter;
	gchar *name = NULL;
	gboolean in_snippet = FALSE;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_TREE_VIEW (vars_view));
	g_return_if_fail (GTK_IS_TREE_MODEL (vars_model));

	if (!gtk_tree_model_get_iter_first (vars_model, &iter))
		return;

	/* We search for a variable which is in the snippet with the given name. If we find
	   it, we focus the cursor on it. */
	do
	{
		gtk_tree_model_get (vars_model, &iter,
		                    VARS_STORE_COL_NAME, &name,
		                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
		                    -1);

		if (!g_strcmp0 (var_name, name) && in_snippet)
		{
			GtkTreePath *path = gtk_tree_model_get_path (vars_model, &iter);

			gtk_tree_view_set_cursor (vars_view, path, col, start_editing);

			gtk_tree_path_free (path);

			g_free (name);
			return;
		}

		g_free (name);

	} while (gtk_tree_model_iter_next (vars_model, &iter));
}

static void
change_snippet_variable_name_in_content (SnippetsEditor *snippets_editor,
                                         const gchar *old_var_name,
                                         const gchar *new_var_name)
{
	SnippetsEditorPrivate *priv = NULL;
	gchar *old_content = NULL;
	GString *updated_content = NULL, *cur_var_name = NULL;
	gint i = 0, j = 0, old_content_len = 0;
	GtkTextBuffer *buffer = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	buffer = gtk_text_view_get_buffer (priv->content_text_view);

	/* We should have a snippet loaded if we got in this function */
	if (!ANJUTA_IS_SNIPPET (priv->snippet))
		g_return_if_reached ();

	/* Get the content depending on what is shown in the content editor right now */
	if (gtk_toggle_button_get_active (priv->preview_button))
	{
		old_content = g_strdup (snippet_get_content (priv->snippet));
	}
	else
	{
		GtkTextIter start_iter, end_iter;

		gtk_text_buffer_get_start_iter (buffer, &start_iter);
		gtk_text_buffer_get_end_iter (buffer, &end_iter);
		old_content = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
	}
	
	old_content_len = strlen (old_content);
	updated_content = g_string_new ("");

	for (i = 0; i < old_content_len; i ++)
	{
		if (IS_SNIPPET_VAR_START (old_content, i))
		{
			/* We add the snippet var start -- "${" and continue with the variable name */
			//j = i;
			i += strlen (SNIPPET_VAR_START) - 1;
			j = i + 1;
			g_string_append (updated_content, SNIPPET_VAR_START);
			cur_var_name = g_string_new ("");

			/* We add all the chars until we got to the mark of the variable end or
			   to the end of the text */
			while (!IS_SNIPPET_VAR_END (old_content, j) && j < old_content_len)
				g_string_append_c (cur_var_name, old_content[j ++]);

			/* If we found a valid variable and it's the variable we want to replace */
			if  (IS_SNIPPET_VAR_END (old_content, j) && 
			     !g_strcmp0 (cur_var_name->str, old_var_name))
			{
				g_string_append (updated_content, new_var_name);
				g_string_append (updated_content, SNIPPET_VAR_END);
				i = j;
			}

			g_string_free (cur_var_name, TRUE);
			
		}
		else
		{
			g_string_append_c (updated_content, old_content[i]);
		}
	}

	/* We update the content */
	snippet_set_content (priv->snippet, updated_content->str);

	/* And update the content text view if neccesary */
	if (!gtk_toggle_button_get_active (priv->preview_button))
	{
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (priv->content_text_view);
		gtk_text_buffer_set_text (buffer, updated_content->str, -1);
	}

	g_string_free (updated_content, TRUE);
	g_free (old_content);
}

static void
variables_view_name_combo_data_func (GtkTreeViewColumn *column,
                                     GtkCellRenderer *cell,
                                     GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gpointer user_data)
{
	gboolean in_snippet = FALSE, undefined = FALSE;
	gchar *name = NULL, *name_with_markup = NULL;
	SnippetVariableType type;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_NAME, &name,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	if (in_snippet)
		name_with_markup = g_strconcat ("<b>", name, "</b>", NULL);
	else
		name_with_markup = g_strdup (name);

	g_object_set (cell, "editable", in_snippet, NULL);
	g_object_set (cell, "markup", name_with_markup, NULL);

	set_cell_colors (cell, type, undefined);

	g_free (name);
	g_free (name_with_markup);
}

static void
variables_view_type_combo_data_func (GtkTreeViewColumn *column,
                                     GtkCellRenderer *cell,
                                     GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gpointer user_data)
{
	SnippetVariableType type;
	gboolean in_snippet = FALSE;
	gboolean undefined = FALSE;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_TYPE, &type,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    -1);

	if (type == SNIPPET_VAR_TYPE_LOCAL)
		g_object_set (cell, "text", LOCAL_TYPE_STR, NULL);
	else
	if (type == SNIPPET_VAR_TYPE_GLOBAL)
		g_object_set (cell, "text", GLOBAL_TYPE_STR, NULL);
	else
		g_return_if_reached ();

	set_cell_colors (cell, type, undefined);

	g_object_set (cell, "sensitive", in_snippet, NULL);
	g_object_set (cell, "editable", in_snippet, NULL);
}

static void
variables_view_type_pixbuf_data_func (GtkTreeViewColumn *column,
                                      GtkCellRenderer *cell,
                                      GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      gpointer user_data)
{
	SnippetVariableType type;
	gboolean undefined = FALSE;

	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_TYPE, &type,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    -1);

	if (type == SNIPPET_VAR_TYPE_GLOBAL && undefined)
		g_object_set (cell, "visible", TRUE, NULL);
	else
		g_object_set (cell, "visible", FALSE, NULL);

	set_cell_colors (cell, type, undefined);
}

static void
variables_view_default_text_data_func (GtkTreeViewColumn *column,
                                       GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       gpointer user_data)
{
	gchar *default_value = NULL;
	gboolean in_snippet = FALSE, undefined = FALSE;
	SnippetVariableType type;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_DEFAULT_VALUE, &default_value,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	g_object_set (cell, "text", default_value, NULL);
	g_object_set (cell, "editable", in_snippet, NULL);
	
	set_cell_colors (cell, type, undefined);

	g_free (default_value);
}

static void
variables_view_instant_text_data_func (GtkTreeViewColumn *column,
                                       GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       gpointer user_data)
{
	gboolean undefined = FALSE;
	SnippetVariableType type;
	
	gtk_tree_model_get (tree_model, iter,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	set_cell_colors (cell, type, undefined);

}

static void
init_variables_view (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Initialize the type model */
	priv->type_model = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_list_store_append (priv->type_model, &iter);
	gtk_list_store_set (priv->type_model, &iter,
	                    0, LOCAL_TYPE_STR,
	                    -1);
	gtk_list_store_append (priv->type_model, &iter);
	gtk_list_store_set (priv->type_model, &iter,
	                    0, GLOBAL_TYPE_STR,
	                    -1);
	                       

	/* Initialize the sorted model */
	priv->vars_store_sorted = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (priv->vars_store));
	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (priv->vars_store_sorted),
	                                         vars_store_sort_func,
	                                         NULL, NULL);
	gtk_tree_view_set_model (priv->variables_view, GTK_TREE_MODEL (priv->vars_store_sorted));

	/* Column 1 - Name */
	col = gtk_tree_view_column_new ();
	priv->name_combo_cell = gtk_cell_renderer_combo_new ();
	gtk_tree_view_column_set_title (col, NAME_COL_TITLE);
	gtk_tree_view_column_pack_start (col, priv->name_combo_cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, priv->name_combo_cell,
	                                         variables_view_name_combo_data_func,
	                                         snippets_editor, NULL);
	g_object_set (col, "resizable", TRUE, NULL);
	g_object_set (col, "min-width", MIN_NAME_COL_WIDTH, NULL);
	g_object_set (priv->name_combo_cell, "has-entry", TRUE, NULL);
	g_object_set (priv->name_combo_cell, "model", 
	              snippets_db_get_global_vars_model (priv->snippets_db), NULL);
	g_object_set (priv->name_combo_cell, "text-column",
	              GLOBAL_VARS_MODEL_COL_NAME, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);

	/* Column 2 - Type */
	col = gtk_tree_view_column_new ();
	priv->type_combo_cell = gtk_cell_renderer_combo_new ();
	priv->type_pixbuf_cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_set_title (col, TYPE_COL_TITLE);
	gtk_tree_view_column_pack_start (col, priv->type_combo_cell, FALSE);
	gtk_tree_view_column_pack_end (col, priv->type_pixbuf_cell, FALSE);
	g_object_set (priv->type_combo_cell, "model", priv->type_model, NULL);
	g_object_set (priv->type_combo_cell, "text-column", 0, NULL);
	g_object_set (priv->type_combo_cell, "has-entry", FALSE, NULL);
	gtk_tree_view_column_set_cell_data_func (col, priv->type_combo_cell,
	                                         variables_view_type_combo_data_func,
	                                         snippets_editor, NULL);
	g_object_set (priv->type_pixbuf_cell, "stock-id", GTK_STOCK_DIALOG_WARNING, NULL);
	gtk_tree_view_column_set_cell_data_func (col, priv->type_pixbuf_cell,
	                                         variables_view_type_pixbuf_data_func,
	                                         snippets_editor, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);
	
	/* Column 3 - Default Value (just for those variables that are in the snippet) */
	col = gtk_tree_view_column_new ();
	priv->default_text_cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_title (col, DEFAULT_COL_TITLE);
	gtk_tree_view_column_pack_start (col, priv->default_text_cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, priv->default_text_cell,
	                                         variables_view_default_text_data_func,
	                                         snippets_editor, NULL);
	g_object_set (col, "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);

	/* Column 4 - Instant value */
	priv->instant_text_cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (INSTANT_COL_TITLE,
	                                                priv->instant_text_cell,
	                                                "text", VARS_STORE_COL_INSTANT_VALUE,
	                                                NULL);
	gtk_tree_view_column_set_cell_data_func (col, priv->instant_text_cell,
	                                         variables_view_instant_text_data_func,
	                                         snippets_editor, NULL);
	g_object_set (col, "resizable", TRUE, NULL);
	g_object_set (G_OBJECT (priv->instant_text_cell), "editable", FALSE, NULL);
	gtk_tree_view_insert_column (priv->variables_view, col, -1);

	/* Initialize the buttons as insensitive */
	g_object_set (priv->variable_add_button, "sensitive", FALSE, NULL);
	g_object_set (priv->variable_remove_button, "sensitive", FALSE, NULL);
	g_object_set (priv->variable_insert_button, "sensitive", FALSE, NULL);

}

static void
focus_snippets_group_combo_box (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	AnjutaSnippetsGroup *parent_snippets_group = NULL;
	const gchar *parent_snippets_group_name = NULL;
	gchar *cur_group_name = NULL;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* We initialize it */
	g_object_set (priv->snippets_group_combo_box, "active", -1, NULL);

	/* If we have a snippet and it has a snippets group. */
	if (ANJUTA_IS_SNIPPET (priv->snippet) && 
	    ANJUTA_IS_SNIPPETS_GROUP (priv->snippet->parent_snippets_group))
	{
		parent_snippets_group = ANJUTA_SNIPPETS_GROUP (priv->snippet->parent_snippets_group);
		parent_snippets_group_name = snippets_group_get_name (parent_snippets_group);

		if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->group_store), &iter))
			return;

		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (priv->group_store), &iter,
			                    GROUPS_COL_NAME, &cur_group_name,
			                    -1);

			if (!g_strcmp0 (cur_group_name, parent_snippets_group_name))
			{
				gtk_combo_box_set_active_iter (priv->snippets_group_combo_box, &iter);

				g_free (cur_group_name);
				return;
			}

			g_free (cur_group_name);
			
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->group_store), &iter));
	}

}

static void
init_snippets_group_combo_box (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkCellRenderer *cell = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Init the tree model */
	priv->group_store = gtk_list_store_new (1, G_TYPE_STRING);

	/* Set the tree model to the combo-box */
	gtk_combo_box_set_model (priv->snippets_group_combo_box, 
	                         GTK_TREE_MODEL (priv->group_store));
	g_object_unref (priv->group_store);

	/* Add the cell renderer */
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->snippets_group_combo_box),
	                            cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->snippets_group_combo_box),
	                                cell, "text", GROUPS_COL_NAME, NULL);
	
}

static void
reload_snippets_group_combo_box (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeIter iter;
	gchar *cur_group_name = NULL, *parent_group_name = NULL;
	gint i = 0;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Unref the old model if there is one */
	gtk_list_store_clear (priv->group_store);

	/* If we have a snippet loaded, we should re-check it in the combo-box */
	if (ANJUTA_IS_SNIPPET (priv->snippet) &&
	    ANJUTA_IS_SNIPPETS_GROUP (priv->snippet->parent_snippets_group))
	{
		AnjutaSnippetsGroup *group = ANJUTA_SNIPPETS_GROUP (priv->snippet->parent_snippets_group);

		parent_group_name = g_strdup (snippets_group_get_name (group));
	}
	
	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->snippets_db), &iter))
		return;

	/* Add the groups in the database to the list store */
	do
	{
		/* Get the current group name from the database ... */
		gtk_tree_model_get (GTK_TREE_MODEL (priv->snippets_db), &iter,
		                    SNIPPETS_DB_MODEL_COL_NAME, &cur_group_name,
		                    -1);

		gtk_combo_box_append_text (priv->snippets_group_combo_box, cur_group_name);

		/* If we have a snippet loaded, we search for the row. */
		if (parent_group_name != NULL)
		{
			if (!g_strcmp0 (parent_group_name, cur_group_name))
				g_object_set (priv->snippets_group_combo_box, "active", i, NULL);

			i ++;
		}
		
		g_free (cur_group_name);

	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->snippets_db), &iter));	

}

static void
init_languages_combo_box (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkCellRenderer *cell = NULL;
	GtkTreeIter tree_iter;
	GList *lang_ids = NULL, *iter = NULL;
	gint cur_lang_id = 0;
	const gchar *cur_lang = NULL;
	IAnjutaLanguage *language = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Initialize the model which will be used for the combo-box */
	priv->lang_store = gtk_list_store_new (LANG_MODEL_COL_N, G_TYPE_BOOLEAN, G_TYPE_STRING);

	/* Add the items */
	language = anjuta_shell_get_interface (priv->snippets_db->anjuta_shell, 
	                                       IAnjutaLanguage, NULL);
	lang_ids = ianjuta_language_get_languages (language, NULL);
	for (iter = g_list_first (lang_ids); iter != NULL; iter = g_list_next (iter))
	{
		cur_lang_id = GPOINTER_TO_INT (iter->data);
		cur_lang = ianjuta_language_get_name (language, cur_lang_id, NULL);
		gtk_list_store_append (priv->lang_store, &tree_iter);
		gtk_list_store_set (priv->lang_store, &tree_iter,
		                    LANG_MODEL_COL_IN_SNIPPET, FALSE,
		                    LANG_MODEL_COL_NAME, cur_lang,
		                    -1);
	}
	g_list_free (lang_ids);

	/* Set it as the model of the combo-box */
	gtk_combo_box_set_model (priv->languages_combo_box, 
	                         GTK_TREE_MODEL (priv->lang_store));
	g_object_unref (priv->lang_store);

	/* Add the cell renderers */
	cell = gtk_cell_renderer_toggle_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->languages_combo_box),
	                            cell, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (priv->languages_combo_box),
	                               cell, "active", LANG_MODEL_COL_IN_SNIPPET);
	
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->languages_combo_box),
	                            cell, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (priv->languages_combo_box),
	                               cell, "text", LANG_MODEL_COL_NAME);
}

static void
load_languages_combo_box (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeIter iter;
	gchar *cur_lang = NULL;
	gboolean has_language = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Add the new selection or clear it */
	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->lang_store), &iter))
		g_return_if_reached ();

	do
	{
		/* Clear it */
		gtk_list_store_set (priv->lang_store, &iter,
		                    LANG_MODEL_COL_IN_SNIPPET, FALSE,
		                    -1);

		/* If we have a snippet loaded, we also populate the checklist */
		if (ANJUTA_IS_SNIPPET (priv->snippet))
		{
			gtk_tree_model_get (GTK_TREE_MODEL (priv->lang_store), &iter,
			                    LANG_MODEL_COL_NAME, &cur_lang,
			                    -1);

			has_language = snippet_has_language (priv->snippet, cur_lang);
			gtk_list_store_set (priv->lang_store, &iter,
			                    LANG_MODEL_COL_IN_SNIPPET, has_language,
			                    -1);
	
			g_free (cur_lang);
		}
		
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->lang_store), &iter));

	/* The combo box should be sensitive only if there is a snippet loaded */
	g_object_set (priv->languages_combo_box, "sensitive", 
	              ANJUTA_IS_SNIPPET (priv->snippet), NULL);
}

static void
load_keywords_entry (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GList *keywords = NULL, *iter = NULL;
	const gchar *cur_keyword = NULL;
	GString *keywords_string = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Clear the text entry */
	gtk_entry_set_text (priv->keywords_entry, "");

	/* If there is a snippet loaded, add the keywords */
	if (ANJUTA_IS_SNIPPET (priv->snippet))
	{
		keywords = snippet_get_keywords_list (priv->snippet);
		keywords_string = g_string_new ("");
		
		for (iter = g_list_first (keywords); iter != NULL; iter = g_list_next (iter))
		{
			cur_keyword = (const gchar *)iter->data;
			g_string_append (keywords_string, cur_keyword);
			g_string_append (keywords_string, " ");
		}

		gtk_entry_set_text (priv->keywords_entry, keywords_string->str);
		
		g_string_free (keywords_string, TRUE);
		g_list_free (keywords);
	}
}

static void
save_keywords_entry (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GList *keywords = NULL;
	gchar **keywords_array = NULL;
	gint i = 0;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	keywords_array = g_strsplit (gtk_entry_get_text (priv->keywords_entry), " ", -1);

	/* Add each item of the array to the list */
	while (keywords_array[i])
	{
		/* In case the user entered more than one space between keywords */
		if (g_strcmp0 (keywords_array[i], ""))
			keywords = g_list_append (keywords, keywords_array[i]);
		
		i ++;
	}

	snippet_set_keywords_list (priv->snippet, keywords);
	g_strfreev (keywords_array);
	g_list_free (keywords);
	
}

static gboolean
check_trigger_entry (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	gboolean valid = TRUE;
	guint16 i = 0, text_length = 0;
	const gchar *text = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor), FALSE);
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Check the text is valid only if there is a snippet loaded */
	if (ANJUTA_IS_SNIPPET (priv->snippet))
	{
		text        = gtk_entry_get_text (priv->trigger_entry);
		text_length = gtk_entry_get_text_length (priv->trigger_entry);

		for (i = 0; i < text_length; i ++)
			if (!g_ascii_isalnum (text[i]) && text[i] != '_')
			{
				/* Set as invalid and set the according error message */
				g_object_set (priv->trigger_notify, "tooltip-markup", ERROR_TRIGGER_NOT_VALID, NULL);
				valid = FALSE;
				break;
			}

		/* If there isn't a trigger-key entered, we also show an error message, but a
		   different one */
		if (text_length == 0)
		{
			g_object_set (priv->trigger_notify, "tooltip-markup", ERROR_TRIGGER_NULL, NULL);
			valid = FALSE;
		}

	}

	/* Show or hide */
	g_object_set (priv->trigger_notify, "visible", !valid, NULL);

	return valid;
}

static gboolean
check_languages_combo_box (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeIter iter;
	gchar *lang_name = NULL;
	const gchar *trigger = NULL;
	gboolean no_lang_selected = TRUE;
	AnjutaSnippet *conflicting_snippet = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor), FALSE);
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	trigger = gtk_entry_get_text (priv->trigger_entry);

	/* We should always have this tree model filled */
	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->lang_store), &iter))
		g_return_val_if_reached (FALSE);

	/* We initialize the error image by hiding it */
	g_object_set (priv->languages_notify, "visible", FALSE, NULL);

	if (!ANJUTA_IS_SNIPPET (priv->snippet))
		return TRUE;

	/* We check each language to see if it doesen't cause a conflict */
	do
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->lang_store), &iter,
		                    LANG_MODEL_COL_NAME, &lang_name,
		                    -1);

		if (snippet_has_language (priv->snippet, lang_name))
		{
			conflicting_snippet = snippets_db_get_snippet (priv->snippets_db, trigger, lang_name);

			no_lang_selected = FALSE;

			/* If there is a conflicting snippet and it isn't the one we had backed-up,
			   we make visible the error icon. */
			if (ANJUTA_IS_SNIPPET (conflicting_snippet) && 
			    priv->backup_snippet != conflicting_snippet)
			{
				g_object_set (priv->languages_notify, "tooltip-markup", ERROR_LANG_CONFLICT, NULL);
				g_object_set (priv->languages_notify, "visible", TRUE, NULL);

				g_free (lang_name);
				return FALSE;
			}

		}

		g_free (lang_name);
			
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->lang_store), &iter));

	/* If the user doesn't have any language selected, we show a warning. */
	if (no_lang_selected)
	{
		g_object_set (priv->languages_notify, "tooltip-markup", ERROR_LANG_NULL, NULL);
		g_object_set (priv->languages_notify, "visible", TRUE, NULL);

		return FALSE;
	}

	return TRUE;
}

static gboolean
check_group_combo_box (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	gboolean has_selection = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor), FALSE);
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	has_selection = (gtk_combo_box_get_active (priv->snippets_group_combo_box) >= 0);
	g_object_set (priv->group_notify, "visible", 
	              !has_selection && ANJUTA_IS_SNIPPET (priv->snippet), 
	              NULL);

	return has_selection;
}

static void
check_all_inputs (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	gboolean no_errors = FALSE;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* We check we don't have any errors */
	no_errors = (!priv->languages_error && !priv->group_error && !priv->trigger_error);

	g_object_set (priv->save_button, "sensitive", no_errors, NULL);
}

static void
check_name_entry (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	guint16 text_length = 0;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Initialize the warning icon */
	g_object_set (priv->name_notify, "visible", FALSE, NULL);

	if (ANJUTA_IS_SNIPPET (priv->snippet))
	{
		text_length = gtk_entry_get_text_length (priv->name_entry);
		g_object_set (priv->name_notify, "visible", text_length == 0, NULL);
		
	}
}

static void
init_input_errors (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	g_object_set (priv->group_notify, "visible", FALSE, NULL);
	g_object_set (priv->languages_notify, "visible", FALSE, NULL);
	g_object_set (priv->trigger_notify, "visible", FALSE, NULL);
	
	priv->group_error     = !check_languages_combo_box (snippets_editor);
	priv->languages_error = !check_group_combo_box (snippets_editor);
	priv->trigger_error   = !check_trigger_entry (snippets_editor);
	check_name_entry (snippets_editor);

	check_all_inputs (snippets_editor);
}

static void
init_sensitivity (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	gboolean has_snippet = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	has_snippet = ANJUTA_IS_SNIPPET (priv->snippet);
	g_object_set (priv->save_button, "sensitive", has_snippet, NULL);
	g_object_set (priv->variable_add_button, "sensitive", has_snippet, NULL);
	g_object_set (priv->languages_combo_box, "sensitive", has_snippet, NULL);
	g_object_set (priv->snippets_group_combo_box, "sensitive", has_snippet, NULL);
	g_object_set (priv->name_entry, "sensitive", has_snippet, NULL);
	g_object_set (priv->trigger_entry, "sensitive", has_snippet, NULL);
	g_object_set (priv->keywords_entry, "sensitive", has_snippet, NULL);
	g_object_set (priv->content_text_view, "sensitive", has_snippet, NULL);

}

static void
init_editor_handlers (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	g_signal_connect (GTK_OBJECT (priv->preview_button),
	                  "toggled",
	                  GTK_SIGNAL_FUNC (on_preview_button_toggled),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->save_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_save_button_clicked),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->close_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_close_button_clicked),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->name_combo_cell),
	                  "edited",
	                  G_CALLBACK (on_name_combo_cell_edited),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->type_combo_cell),
	                  "changed",
	                  G_CALLBACK (on_type_combo_cell_changed),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->default_text_cell),
	                  "edited",
	                  G_CALLBACK (on_default_text_cell_edited),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->variables_view),
	                  "row-activated",
	                  GTK_SIGNAL_FUNC (on_variables_view_row_activated),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->variable_add_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_variable_add_button_clicked),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->variable_remove_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_variable_remove_button_clicked),
	                  snippets_editor);
	g_signal_connect (GTK_OBJECT (priv->variable_insert_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_variable_insert_button_clicked),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (priv->variables_view)),
	                  "changed",
	                  G_CALLBACK (on_variables_view_selection_changed),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->snippets_group_combo_box),
	                  "changed",
	                  G_CALLBACK (on_snippets_group_combo_box_changed),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->languages_combo_box),
	                  "changed",
	                  G_CALLBACK (on_languages_combo_box_changed),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->trigger_entry),
	                  "notify::text",
	                  G_CALLBACK (on_trigger_entry_text_changed),
	                  snippets_editor);
	g_signal_connect (G_OBJECT (priv->name_entry),
	                  "notify::text",
	                  G_CALLBACK (on_name_entry_text_changed),
	                  snippets_editor);
}

SnippetsEditor *
snippets_editor_new (SnippetsDB *snippets_db)
{
	SnippetsEditor *snippets_editor = 
	          ANJUTA_SNIPPETS_EDITOR (g_object_new (snippets_editor_get_type (), NULL));
	SnippetsEditorPrivate *priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), snippets_editor);

	priv->snippets_db = snippets_db;

	/* Load the variables tree model */
	priv->vars_store = snippet_vars_store_new ();

	/* Load the UI for snippets-editor.ui */
	load_snippets_editor_ui (snippets_editor);

	/* Initialize the variables tree view */
	init_variables_view (snippets_editor);

	/* Initialize the snippets-group combo box */
	init_snippets_group_combo_box (snippets_editor);
	reload_snippets_group_combo_box (snippets_editor);

	/* Initialize the languages combo box */
	init_languages_combo_box (snippets_editor);
	
	/* Connect the handlers */
	init_editor_handlers (snippets_editor);

	/* Initialize the buttons as insensitive */
	g_object_set (priv->save_button, "sensitive", FALSE, NULL);
	g_object_set (priv->languages_combo_box, "sensitive", FALSE, NULL);
	g_object_set (priv->snippets_group_combo_box, "sensitive", FALSE, NULL);
	
	return snippets_editor;
}

static void
load_content_to_editor (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	gchar *text = NULL;
	GtkTextBuffer *content_buffer = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* If we don't have a snippet loaded we don't do anything */
	if (!ANJUTA_IS_SNIPPET (priv->snippet))
	{
		text = g_strdup ("");
	}
	else
	if (gtk_toggle_button_get_active (priv->preview_button))
	{
		text = snippet_get_default_content (priv->snippet,
		                                    G_OBJECT (priv->snippets_db),
		                                    "");
	}
	else
	{
		text = g_strdup (snippet_get_content (priv->snippet));
	}
	
	content_buffer = gtk_text_view_get_buffer (priv->content_text_view);
	gtk_text_buffer_set_text (content_buffer, text, -1);
	g_free (text);
}

/* Warning: this will take the text as it is. It won't check if it's a preview. */
static void
save_content_from_editor (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTextIter start_iter, end_iter;
	gchar *text = NULL;
	GtkTextBuffer *content_buffer = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* If we don't have a snippet loaded, don't do anything */
	if (!ANJUTA_IS_SNIPPET (priv->snippet))
		return;

	/* Get the text in the GtkTextBuffer */
	content_buffer = gtk_text_view_get_buffer (priv->content_text_view);
	gtk_text_buffer_get_start_iter (content_buffer, &start_iter);
	gtk_text_buffer_get_end_iter (content_buffer, &end_iter);
	text = gtk_text_buffer_get_text (content_buffer, &start_iter, &end_iter, FALSE);

	/* Save it to the snippet */
	snippet_set_content (priv->snippet, text);
	
	g_free (text);
}

void 
snippets_editor_set_snippet (SnippetsEditor *snippets_editor, 
                             AnjutaSnippet *snippet)
{
	SnippetsEditorPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* If we are saving the snippet, we need to guard entering this method because
	   it will be called by the selection changed handler in the browser */
	if (priv->saving_snippet)
		return;

	/* Delete the old snippet */
	if (ANJUTA_IS_SNIPPET (priv->snippet))
		g_object_unref (priv->snippet);

	/* Set the current snippet */
	priv->backup_snippet = snippet;
	if (ANJUTA_IS_SNIPPET (snippet))
		priv->snippet = snippet_copy (snippet);
	else
		priv->snippet = NULL;

	/* Set the sensitive property of the widgets */
	init_sensitivity (snippets_editor);

	/* Initialize the snippet content editor */
	load_content_to_editor (snippets_editor);

	/* Initialize the name property */
	if (ANJUTA_IS_SNIPPET (snippet))
		gtk_entry_set_text (priv->name_entry, snippet_get_name (snippet));
	else
		gtk_entry_set_text (priv->name_entry, "");
	
	/* Initialize the trigger-key property */
	if (ANJUTA_IS_SNIPPET (snippet))
		gtk_entry_set_text (priv->trigger_entry, snippet_get_trigger_key (snippet));
	else
		gtk_entry_set_text (priv->trigger_entry, "");

	/* Initialize the snippets group combo-box property */
	reload_snippets_group_combo_box (snippets_editor);
	focus_snippets_group_combo_box (snippets_editor);

	/* Initialize the language combo-box property */
	load_languages_combo_box (snippets_editor);
	
	/* Initialize the keywords text-view property */
	load_keywords_entry (snippets_editor);

	/* Initialize the variables tree-view - load the variables tree model with the variables
	   from the current snippet*/
	snippet_vars_store_unload (priv->vars_store);
	if (ANJUTA_IS_SNIPPET (priv->snippet))
		snippet_vars_store_load (priv->vars_store, priv->snippets_db, priv->snippet);

	/* We initialize the errors/warnings */
	init_input_errors (snippets_editor);

}

void 
snippets_editor_set_snippet_new (SnippetsEditor *snippets_editor)
{
	SnippetsEditorPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (snippets_editor));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);

	/* Delete the old snippet */
	if (ANJUTA_IS_SNIPPET (priv->snippet))
		g_object_unref (priv->snippet);
	priv->backup_snippet = NULL;

	/* Initialize a new empty snippet */
	priv->snippet = snippet_new ("", NULL, "", "", NULL, NULL, NULL, NULL);

	init_sensitivity (snippets_editor);

	/* Initialize the entries and content */
	gtk_entry_set_text (priv->name_entry, "");
	gtk_entry_set_text (priv->trigger_entry, "");
	gtk_entry_set_text (priv->keywords_entry, "");
	load_content_to_editor (snippets_editor);


	reload_snippets_group_combo_box (snippets_editor);
	focus_snippets_group_combo_box (snippets_editor);

	load_languages_combo_box (snippets_editor);

	snippet_vars_store_unload (priv->vars_store);
	if (ANJUTA_IS_SNIPPET (priv->snippet))
		snippet_vars_store_load (priv->vars_store, priv->snippets_db, priv->snippet);

	init_input_errors (snippets_editor);
	
}

static void  
on_preview_button_toggled (GtkToggleButton *preview_button,
                           gpointer user_data)
{
	SnippetsEditor *snippets_editor = NULL;
	SnippetsEditorPrivate *priv = NULL;
	gboolean preview_mode = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	snippets_editor = ANJUTA_SNIPPETS_EDITOR (user_data);
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);
	
	/* If we go in the preview mode, we should save the content and disallow removing
	   and inserting variables */
	preview_mode = gtk_toggle_button_get_active (preview_button);
	if (preview_mode)
		save_content_from_editor (snippets_editor);

	g_object_set (priv->variable_insert_button, "sensitive", !preview_mode, NULL);
	g_object_set (priv->content_text_view, "editable", !preview_mode, NULL);

	load_content_to_editor (snippets_editor);
}

static void  
on_save_button_clicked (GtkButton *save_button,
                        gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	SnippetsEditor *snippets_editor = NULL;
	AnjutaSnippetsGroup *parent_snippets_group = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	snippets_editor = ANJUTA_SNIPPETS_EDITOR (user_data);
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (snippets_editor);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));

	/* If there isn't a snippet editing */
	if (!ANJUTA_IS_SNIPPET (priv->snippet))
		return;

	/* The user should have a snippets group selected */
	if (!ANJUTA_IS_SNIPPETS_GROUP (priv->snippet->parent_snippets_group))
		return;

	/* Copy over the name, trigger and keywords */
	snippet_set_name (priv->snippet, gtk_entry_get_text (priv->name_entry));
	snippet_set_trigger_key (priv->snippet, gtk_entry_get_text (priv->trigger_entry));
	save_keywords_entry (snippets_editor);
	
	/* Save the content */
	if (!gtk_toggle_button_get_active (priv->preview_button))
		save_content_from_editor (snippets_editor);

	/* Delete the back-up snippet if there is one (we don't have one if it's a new snippet)*/
	priv->saving_snippet = TRUE;
	if (ANJUTA_IS_SNIPPET (priv->backup_snippet))
		snippets_db_remove_snippet (priv->snippets_db, 
			                        snippet_get_trigger_key (priv->backup_snippet),
			                        snippet_get_any_language (priv->backup_snippet),
			                        TRUE);
	/* Add the new snippet */
	parent_snippets_group = ANJUTA_SNIPPETS_GROUP (priv->snippet->parent_snippets_group);
	snippets_db_add_snippet (priv->snippets_db,
	                         priv->snippet,
	                         snippets_group_get_name (parent_snippets_group));

	/* Move the new snippet as the back-up one */
	priv->backup_snippet = priv->snippet;
	priv->snippet = snippet_copy (priv->backup_snippet);

	/* Emit the signal that the snippet was saved */
	g_signal_emit_by_name (snippets_editor, "snippet-saved", priv->backup_snippet);

	priv->saving_snippet = FALSE;
}

static void
on_close_button_clicked (GtkButton *close_button,
                         gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));

	g_signal_emit_by_name (ANJUTA_SNIPPETS_EDITOR (user_data), "close-request");
}

static void 
on_name_combo_cell_edited (GtkCellRendererText *cell,
                           gchar *path_string,
                           gchar *new_string,
                           gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	gchar *old_name = NULL;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* We don't accept empty strings as variables names */
	if (!g_strcmp0 (new_string, ""))
		return;

	/* Get the old name at the given path */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->vars_store_sorted), &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->vars_store_sorted), &iter,
	                    VARS_STORE_COL_NAME, &old_name,
	                    -1);

	/* If the name wasn't changed we don't do anything */
	if (!g_strcmp0 (old_name, new_string))
	{
		g_free (old_name);
		return;
	}

	/* Set the new name */
	snippet_vars_store_set_variable_name (priv->vars_store, old_name, new_string);

	/* If there is a global variable with the given name, treat it as global */
	if (snippets_db_has_global_variable (priv->snippets_db, new_string))
		snippet_vars_store_set_variable_type (priv->vars_store, 
		                                      new_string, 
		                                      SNIPPET_VAR_TYPE_GLOBAL);

	focus_on_in_snippet_variable (priv->variables_view, 
	                              GTK_TREE_MODEL (priv->vars_store_sorted), 
	                              new_string,
	                              NULL, FALSE);

	change_snippet_variable_name_in_content (ANJUTA_SNIPPETS_EDITOR (user_data), 
	                                         old_name, new_string);
	
	g_free (old_name);
}


static void
on_type_combo_cell_changed (GtkCellRendererCombo *cell,
                            gchar *path_string,
                            gchar *new_string,
                            gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	gchar *name = NULL;
	GtkTreeIter iter;
	SnippetVariableType type;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* Get the name at the given path */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->vars_store_sorted), &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->vars_store_sorted), &iter,
	                    VARS_STORE_COL_NAME, &name,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);

	if (type == SNIPPET_VAR_TYPE_LOCAL)
		snippet_vars_store_set_variable_type (priv->vars_store, name, SNIPPET_VAR_TYPE_GLOBAL);
	else
		snippet_vars_store_set_variable_type (priv->vars_store, name, SNIPPET_VAR_TYPE_LOCAL);

	focus_on_in_snippet_variable (priv->variables_view, 
	                              GTK_TREE_MODEL (priv->vars_store_sorted), 
	                              name,
	                              NULL, FALSE);

	g_free (name);
}

static void
on_default_text_cell_edited (GtkCellRendererText *cell,
                             gchar *path_string,
                             gchar *new_string,
                             gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	gchar *name = NULL;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* Get the name at the given path */
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->vars_store_sorted), &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->vars_store_sorted), &iter,
	                    VARS_STORE_COL_NAME, &name,
	                    -1);

	snippet_vars_store_set_variable_default (priv->vars_store, name, new_string);

	g_free (name);

}


static void
on_variables_view_row_activated (GtkTreeView *tree_view,
                                 GtkTreePath *path,
                                 GtkTreeViewColumn *col,
                                 gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* TODO -- still to be decided :) */
	
}

static void
on_variable_add_button_clicked (GtkButton *variable_add_button,
                                gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeViewColumn *col = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* Add the variable to the vars_store */
	snippet_vars_store_add_variable_to_snippet (priv->vars_store,
	                                            NEW_VAR_NAME, FALSE);

	/* Focus on it */
	col = gtk_tree_view_get_column (priv->variables_view, VARS_VIEW_COL_NAME);
	focus_on_in_snippet_variable (priv->variables_view, 
	                              GTK_TREE_MODEL (priv->vars_store_sorted),
	                              NEW_VAR_NAME,
	                              col, TRUE);
}

static void
on_variable_remove_button_clicked (GtkButton *variable_remove_button,
                                   gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeIter iter;
	gboolean has_selection = FALSE;
	GtkTreeSelection *selection = NULL;
	GtkTreeModel *model = NULL;
	gchar *name = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* Get the selected variable */
	selection = gtk_tree_view_get_selection (priv->variables_view);
	model = GTK_TREE_MODEL (priv->vars_store_sorted);
	has_selection = gtk_tree_selection_get_selected (selection, &model, &iter);

	/* We should always have a selection if the remove button is sensitive */
	g_return_if_fail (has_selection);

	/* Remove the variable from the vars_store */
	gtk_tree_model_get (model, &iter,
	                    VARS_STORE_COL_NAME, &name,
	                    -1);
	snippet_vars_store_remove_variable_from_snippet (priv->vars_store, name);
	
	g_free (name);
}

static void
on_variable_insert_button_clicked (GtkButton *variable_insert_button,
                                   gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTextBuffer *content_buffer = NULL;
	gchar *var_name = NULL, *var_name_formated = NULL;
	GtkTreeSelection *selection = NULL;
	GtkTreeIter iter;
	gboolean in_snippet = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* Get the name of the selected variable */
	selection = gtk_tree_view_get_selection (priv->variables_view);
	if (!gtk_tree_selection_get_selected (selection, &priv->vars_store_sorted, &iter))
		g_return_if_reached ();

	gtk_tree_model_get (priv->vars_store_sorted, &iter,
	                    VARS_STORE_COL_NAME, &var_name,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    -1);

	/* We insert the variable in the content text buffer and add it to the snippet
	   if necessary	*/
	var_name_formated = g_strconcat (SNIPPET_VAR_START, var_name, SNIPPET_VAR_END, NULL);
	content_buffer = gtk_text_view_get_buffer (priv->content_text_view);
	gtk_text_buffer_insert_at_cursor (content_buffer, var_name_formated, -1);

	if (!in_snippet)
	{
		snippet_vars_store_add_variable_to_snippet (priv->vars_store, var_name, TRUE);
		g_object_set (priv->variable_remove_button, "sensitive", TRUE, NULL);
	}
	
	g_free (var_name_formated);
	g_free (var_name);	
}


static void
on_variables_view_selection_changed (GtkTreeSelection *selection,
                                     gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeIter iter;
	GtkTreeModel *vars_store_sorted_model = NULL;
	gboolean in_snippet = FALSE, has_selection = FALSE;
		
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	vars_store_sorted_model = GTK_TREE_MODEL (priv->vars_store_sorted);
	has_selection = gtk_tree_selection_get_selected (selection,
	                                                 &vars_store_sorted_model,
	                                                 &iter);	

	/* If there isn't a selection, the remove and insert button won't be sensitive */
	g_object_set (priv->variable_remove_button, "sensitive", has_selection, NULL);
	g_object_set (priv->variable_insert_button, "sensitive", has_selection, NULL);

	if (!has_selection)
		return;
	
	/* Check if the selected variable is in the snippet. If not, the remove button
	   won't be sensitive. */
	gtk_tree_model_get (vars_store_sorted_model, &iter,
	                    VARS_STORE_COL_IN_SNIPPET, &in_snippet,
	                    -1);
	g_object_set (priv->variable_remove_button, "sensitive", in_snippet, NULL);

}

static void
on_snippets_group_combo_box_changed (GtkComboBox *combo_box,
                                     gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeIter iter;
	gchar *group_name = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));	
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* If we have a snippet loaded, we change his parent */
	if (ANJUTA_IS_SNIPPET (priv->snippet))
	{
		if (!gtk_combo_box_get_active_iter (priv->snippets_group_combo_box, &iter))
		{	
			priv->group_error = !check_group_combo_box (ANJUTA_SNIPPETS_EDITOR (user_data));
			check_all_inputs (ANJUTA_SNIPPETS_EDITOR (user_data));
			return;
		}
		
		gtk_tree_model_get (GTK_TREE_MODEL (priv->group_store), &iter,
		                    GROUPS_COL_NAME, &group_name,
		                    -1);

		snippets_group = snippets_db_get_snippets_group (priv->snippets_db, group_name);
		g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group));

		priv->snippet->parent_snippets_group = G_OBJECT (snippets_group);

		g_free (group_name);
	}

	priv->group_error = !check_group_combo_box (ANJUTA_SNIPPETS_EDITOR (user_data));
	check_all_inputs (ANJUTA_SNIPPETS_EDITOR (user_data));
}


static void
on_languages_combo_box_changed (GtkComboBox *combo_box,
                                gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	GtkTreeIter iter;
	gboolean in_snippet = FALSE;
	gchar *lang_name = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	/* This will happen because we set at the end of this function -1 as active */
	if (gtk_combo_box_get_active (combo_box) < 0)
		return;

	if (!gtk_combo_box_get_active_iter (combo_box, &iter))
		g_return_if_reached ();

	/* We reverse the setting */
	gtk_tree_model_get (GTK_TREE_MODEL (priv->lang_store), &iter,
	                    LANG_MODEL_COL_IN_SNIPPET, &in_snippet,
	                    LANG_MODEL_COL_NAME, &lang_name,
	                    -1);
	gtk_list_store_set (priv->lang_store, &iter,
	                    LANG_MODEL_COL_IN_SNIPPET, !in_snippet,
	                    -1);
	
	if (!in_snippet)
		snippet_add_language (priv->snippet, lang_name);
	else
		snippet_remove_language (priv->snippet, lang_name);

	g_free (lang_name);

	/* We don't want anything to show when there isn't a popup */
	gtk_combo_box_set_active (combo_box, -1);

	priv->languages_error = !check_languages_combo_box (ANJUTA_SNIPPETS_EDITOR (user_data));

	check_all_inputs (ANJUTA_SNIPPETS_EDITOR (user_data));
}

static void
on_trigger_entry_text_changed (GObject *entry_obj,
                               GParamSpec *param_spec,
                               gpointer user_data)
{
	SnippetsEditorPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));
	priv = ANJUTA_SNIPPETS_EDITOR_GET_PRIVATE (user_data);

	priv->trigger_error = !check_trigger_entry (ANJUTA_SNIPPETS_EDITOR (user_data));
	priv->languages_error = !check_languages_combo_box (ANJUTA_SNIPPETS_EDITOR (user_data));

	check_all_inputs (ANJUTA_SNIPPETS_EDITOR (user_data));
}

static void
on_name_entry_text_changed (GObject *entry_obj,
                            GParamSpec *param_spec,
                            gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (user_data));

	check_name_entry (ANJUTA_SNIPPETS_EDITOR (user_data));
}
