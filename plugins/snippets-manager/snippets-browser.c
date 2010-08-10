/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-browser.c
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

#include "snippets-browser.h"
#include "snippets-group.h"
#include "snippets-editor.h"
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#define BROWSER_UI               PACKAGE_DATA_DIR"/glade/snippets-browser.ui"
#define TOOLTIP_SIZE             200
#define NEW_SNIPPETS_GROUP_NAME  "New Snippets Group"

#define ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_BROWSER, SnippetsBrowserPrivate))

struct _SnippetsBrowserPrivate
{
	SnippetsEditor *snippets_editor;
	GtkTreeView *snippets_view;

	SnippetsDB *snippets_db;

	GtkButton *add_button;
	GtkButton *delete_button;
	GtkButton *insert_button;
	GtkToggleButton *edit_button;

	GtkVBox *snippets_view_vbox;
	GtkScrolledWindow *snippets_view_cont;
	
	GtkWidget *browser_hpaned;

	GtkTreeModel *filter;

	gboolean maximized;

	SnippetsInteraction *snippets_interaction;

};

enum
{
	SNIPPETS_VIEW_COL_NAME = 0,
	SNIPPETS_VIEW_COL_TRIGGER,
	SNIPPETS_VIEW_COL_LANGUAGES
};


G_DEFINE_TYPE (SnippetsBrowser, snippets_browser, GTK_TYPE_HBOX);

static void
snippets_browser_class_init (SnippetsBrowserClass* klass)
{

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER_CLASS (klass));

	/* The SnippetsBrowser asks for a maximize. If a maximize is possible,
	   the snippets_browser_show_editor should be called. */
	g_signal_new ("maximize-request",
	              ANJUTA_TYPE_SNIPPETS_BROWSER,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsBrowserClass, maximize_request),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE,
	              0,
	              NULL);

	/* Like above, the SnippetsBrowser asks for a unmaximize. */
	g_signal_new ("unmaximize-request",
	              ANJUTA_TYPE_SNIPPETS_BROWSER,
	              G_SIGNAL_RUN_LAST,
	              G_STRUCT_OFFSET (SnippetsBrowserClass, unmaximize_request),
	              NULL, NULL,
	              g_cclosure_marshal_VOID__VOID,
	              G_TYPE_NONE,
	              0,
	              NULL);
	
	g_type_class_add_private (klass, sizeof (SnippetsBrowserPrivate));
}

static void
snippets_browser_init (SnippetsBrowser* snippets_browser)
{
	SnippetsBrowserPrivate* priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	snippets_browser->priv = priv;
	snippets_browser->show_only_document_language_snippets = TRUE;
	snippets_browser->anjuta_shell = NULL;
	
	/* Initialize the private field */
	priv->snippets_view = NULL;
	priv->snippets_db = NULL;
	priv->add_button = NULL;
	priv->delete_button = NULL;
	priv->insert_button = NULL;
	priv->edit_button = NULL;
	priv->snippets_view_cont = NULL;
	priv->snippets_view_vbox = NULL;
	priv->browser_hpaned = NULL;
	priv->filter = NULL;
	priv->maximized = FALSE;
	priv->snippets_interaction = NULL;
	
}

/* Handlers */

static void     on_add_button_clicked                     (GtkButton *add_button,
                                                           gpointer user_data);
static void     on_delete_button_clicked                  (GtkButton *delete_button,
                                                           gpointer user_data);
static void     on_insert_button_clicked                  (GtkButton *insert_button,
                                                           gpointer user_data);
static void     on_edit_button_toggled                    (GtkToggleButton *edit_button,
                                                           gpointer user_data);
static void     on_snippets_view_row_activated            (GtkTreeView *snippets_view,
                                                           GtkTreePath *path,
                                                           GtkTreeViewColumn *col,
                                                           gpointer user_data);
static gboolean on_snippets_view_query_tooltip            (GtkWidget *snippets_view,
                                                           gint x, 
                                                           gint y,
                                                           gboolean keyboard_mode,
                                                           GtkTooltip *tooltip,
                                                           gpointer user_data);
static void     on_name_changed                           (GtkCellRendererText *renderer,
                                                           gchar *path_string,
                                                           gchar *new_text,
                                                           gpointer user_data);
static void     on_add_snippet_menu_item_activated        (GtkMenuItem *menu_item,
                                                           gpointer user_data);
static void     on_add_snippets_group_menu_item_activated (GtkMenuItem *menu_item,
                                                           gpointer user_data);
static void     on_snippets_view_selection_changed        (GtkTreeSelection *tree_selection,
                                                           gpointer user_data);
static void     on_snippets_editor_snippet_saved          (SnippetsEditor *snippets_editor,
                                                           GObject *snippet,
                                                           gpointer user_data);
static void     on_snippets_editor_close_request          (SnippetsEditor *snippets_editor,
                                                           gpointer user_data);

/* Private methods */

static void
init_browser_layout (SnippetsBrowser *snippets_browser)
{
	GError *error = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkBuilder *bxml = NULL;
	GObject *window = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	/* Load the UI file */
	bxml = gtk_builder_new ();
	if (!gtk_builder_add_from_file (bxml, BROWSER_UI, &error))
	{
		g_warning ("Couldn't load browser ui file: %s", error->message);
		g_error_free (error);
	}

	/* Get the Gtk objects */
	priv->add_button      = GTK_BUTTON (gtk_builder_get_object (bxml, "add_button"));
	priv->delete_button   = GTK_BUTTON (gtk_builder_get_object (bxml, "delete_button"));
	priv->insert_button   = GTK_BUTTON (gtk_builder_get_object (bxml, "insert_button"));
	priv->edit_button     = GTK_TOGGLE_BUTTON (gtk_builder_get_object (bxml, "edit_button"));
	priv->snippets_view_cont = GTK_SCROLLED_WINDOW (gtk_builder_get_object (bxml, "snippets_view_cont"));
	priv->snippets_view_vbox = GTK_VBOX (gtk_builder_get_object (bxml, "snippets_view_vbox"));

	/* Assert the objects */
	g_return_if_fail (GTK_IS_BUTTON (priv->add_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->delete_button));
	g_return_if_fail (GTK_IS_BUTTON (priv->insert_button));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (priv->edit_button));
	g_return_if_fail (GTK_IS_SCROLLED_WINDOW (priv->snippets_view_cont));
	g_return_if_fail (GTK_IS_VBOX (priv->snippets_view_vbox));

	/* Add the Snippets View to the scrolled window */
	gtk_container_add (GTK_CONTAINER (priv->snippets_view_cont),
	                   GTK_WIDGET (priv->snippets_view));
	                   
	/* Add the hbox as the child of the snippets browser */
	window = gtk_builder_get_object (bxml, "builder_window");
	g_object_ref (priv->snippets_view_vbox);
	gtk_container_remove (GTK_CONTAINER (window), GTK_WIDGET (priv->snippets_view_vbox));
	gtk_box_pack_start (GTK_BOX (snippets_browser),
	                    GTK_WIDGET (priv->snippets_view_vbox),
	                    TRUE,
	                    TRUE,
	                    0);
	g_object_unref (priv->snippets_view_vbox);

	/* Initialize the snippets editor */
	priv->snippets_editor = snippets_editor_new (priv->snippets_db);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_EDITOR (priv->snippets_editor));

	/* Add the editor to the HPaned */	
	priv->browser_hpaned = gtk_hpaned_new ();
	gtk_paned_pack2 (GTK_PANED (priv->browser_hpaned),
	                 GTK_WIDGET (priv->snippets_editor),
	                 TRUE, FALSE);
	g_object_ref_sink (priv->browser_hpaned);
              
	g_object_unref (bxml);
}

static void
init_browser_handlers (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	g_signal_connect (GTK_OBJECT (priv->snippets_view),
	                  "row-activated",
	                  GTK_SIGNAL_FUNC (on_snippets_view_row_activated),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->snippets_view),
	                  "query-tooltip",
	                  GTK_SIGNAL_FUNC (on_snippets_view_query_tooltip),
	                  snippets_browser);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (priv->snippets_view)),
	                  "changed",
	                  G_CALLBACK (on_snippets_view_selection_changed),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->add_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_add_button_clicked),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->delete_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_delete_button_clicked),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->insert_button),
	                  "clicked",
	                  GTK_SIGNAL_FUNC (on_insert_button_clicked),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->edit_button),
	                  "toggled",
	                  GTK_SIGNAL_FUNC (on_edit_button_toggled),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->snippets_editor),
	                  "snippet-saved",
	                  GTK_SIGNAL_FUNC (on_snippets_editor_snippet_saved),
	                  snippets_browser);
	g_signal_connect (GTK_OBJECT (priv->snippets_editor),
	                  "close-request",
	                  GTK_SIGNAL_FUNC (on_snippets_editor_close_request),
	                  snippets_browser);

	/* Set the has-tooltip property for the query-tooltip signal */
	g_object_set (priv->snippets_view, "has-tooltip", TRUE, NULL);
}

static void
snippets_view_name_text_data_func (GtkTreeViewColumn *column,
                                   GtkCellRenderer *renderer,
                                   GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gpointer user_data)
{
	gchar *name = NULL;
	GObject *cur_object = NULL;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));
	
	/* Get the name */
	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_NAME, &name,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);
	                    
	g_object_set (renderer, "text", name, NULL);
	g_free (name);

	if (ANJUTA_IS_SNIPPETS_GROUP (cur_object))
	{
		g_object_set (renderer, "editable", TRUE, NULL);
	}
	else
	{
		g_return_if_fail (ANJUTA_IS_SNIPPET (cur_object));
		g_object_set (renderer, "editable", FALSE, NULL);
	}
	
	g_object_unref (cur_object);
	
}

static void
snippets_view_name_pixbuf_data_func (GtkTreeViewColumn *column,
                                     GtkCellRenderer *renderer,
                                     GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gpointer user_data)
{
	GObject *cur_object = NULL;
	gchar *stock_id = NULL;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_PIXBUF (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);

	if (ANJUTA_IS_SNIPPET (cur_object))
	{
		stock_id = GTK_STOCK_FILE;
	}
	else
	{
		g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (cur_object));
		stock_id = GTK_STOCK_DIRECTORY;
	}

	g_object_unref (cur_object);
	g_object_set (renderer, "stock-id", stock_id, NULL);
}

static void
snippets_view_trigger_data_func (GtkTreeViewColumn *column,
                                 GtkCellRenderer *renderer,
                                 GtkTreeModel *tree_model,
                                 GtkTreeIter *iter,
                                 gpointer user_data)
{
	gchar *trigger = NULL, *trigger_markup = NULL;
	
	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_TRIGGER, &trigger,
	                    -1);
	trigger_markup = g_strconcat ("<b>", trigger, "</b>", NULL);
	g_object_set (renderer, "markup", trigger_markup, NULL);

	g_free (trigger);
	g_free (trigger_markup);
}

static void
snippets_view_languages_data_func (GtkTreeViewColumn *column,
                                   GtkCellRenderer *renderer,
                                   GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gpointer user_data)
{ 
	gchar *languages = NULL;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_LANGUAGES, &languages,
	                    -1);

	g_object_set (renderer, "text", languages, NULL);

	g_free (languages);
}

static gboolean
snippets_db_language_filter_func (GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaDocument *doc = NULL;
	IAnjutaLanguage *ilanguage = NULL;
	const gchar *language = NULL;
	GObject *cur_object = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	gboolean has_language = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data), FALSE);
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	
	/* If we have the setting to show all snippets or if we are editing,
	    we just return TRUE */
	if (!snippets_browser->show_only_document_language_snippets ||
	    priv->maximized)
	{
		return TRUE;
	}

	/* Get the current object */
	gtk_tree_model_get (tree_model, iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);

	/* If it's a SnippetsGroup we render it */
	if (ANJUTA_IS_SNIPPETS_GROUP (cur_object))
	{
		g_object_unref (cur_object);
		return TRUE;
	}
	else 
	if (!ANJUTA_IS_SNIPPET (cur_object))
	{
		g_return_val_if_reached (FALSE);
	}
	
	/* Get the current document manager. If it doesn't exist we show all snippets */
	docman = anjuta_shell_get_interface (snippets_browser->anjuta_shell, 
	                                     IAnjutaDocumentManager, 
	                                     NULL);
	if (!IANJUTA_IS_DOCUMENT_MANAGER (docman))
	{
		g_object_unref (cur_object);
		return TRUE;
	}

	ilanguage = anjuta_shell_get_interface (snippets_browser->anjuta_shell,
	                                        IAnjutaLanguage,
	                                        NULL);
	if (!IANJUTA_IS_LANGUAGE (ilanguage))
	{
		g_object_unref (cur_object);
		return TRUE;
	}
	
	/* Get the current doc and if it isn't an editor show all snippets */
	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	if (!IANJUTA_IS_EDITOR (doc))
	{
		g_object_unref (cur_object);
		return TRUE;
	}

	/* Get the language */
	language = ianjuta_language_get_name_from_editor (ilanguage,
	                                                  IANJUTA_EDITOR_LANGUAGE (doc),
	                                                  NULL);
	if (language == NULL)
	{
		g_object_unref (cur_object);
		return TRUE;
	}

	has_language = snippet_has_language (ANJUTA_SNIPPET (cur_object), language);

	g_object_unref (cur_object);
	return has_language;
}

static void
init_snippets_view (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	GtkCellRenderer *text_renderer = NULL, *pixbuf_renderer = NULL;
	GtkTreeViewColumn *column = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (GTK_IS_TREE_VIEW (priv->snippets_view));
	g_return_if_fail (GTK_IS_TREE_MODEL (priv->snippets_db));

	/* Set up the filter */
	priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->snippets_db), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter),
	                                        snippets_db_language_filter_func,
	                                        snippets_browser,
	                                        NULL);
	gtk_tree_view_set_model (priv->snippets_view, priv->filter);


	/* Column 1 - Name */
	column          = gtk_tree_view_column_new ();
	text_renderer   = gtk_cell_renderer_text_new ();
	pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_set_title (column, _("Name"));
	gtk_tree_view_column_pack_start (column, pixbuf_renderer, FALSE);
	gtk_tree_view_column_pack_end (column, text_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, pixbuf_renderer,
	                                         snippets_view_name_pixbuf_data_func,
	                                         snippets_browser, NULL);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
	                                         snippets_view_name_text_data_func,
	                                         snippets_browser, NULL);
	g_signal_connect (GTK_OBJECT (text_renderer),
	                  "edited",
	                  GTK_SIGNAL_FUNC (on_name_changed),
	                  snippets_browser);
	g_object_set (G_OBJECT (column), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->snippets_view, column, -1);

	/* Column 2 - Trigger */
	column        = gtk_tree_view_column_new ();
	text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_title (column, _("Trigger"));
	gtk_tree_view_column_pack_start (column, text_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
	                                         snippets_view_trigger_data_func,
	                                         snippets_browser, NULL);
	g_object_set (G_OBJECT (column), "resizable", TRUE, NULL);
	gtk_tree_view_insert_column (priv->snippets_view, column, -1);

	/* Column 3- Languages */
	column        = gtk_tree_view_column_new ();
	text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_title (column, _("Languages"));
	gtk_tree_view_column_pack_start (column, text_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer, 
	                                         snippets_view_languages_data_func,
	                                         snippets_browser, NULL);
	g_object_set (G_OBJECT (column), "resizable", TRUE, NULL);
	g_object_set (G_OBJECT (column), "visible", FALSE, NULL);
	gtk_tree_view_insert_column (priv->snippets_view, column, -1);

}


/* Public methods */

/**
 * snippets_browser_new:
 *
 * Returns: A new #SnippetsBrowser object.
 */
SnippetsBrowser*
snippets_browser_new (void)
{
	return ANJUTA_SNIPPETS_BROWSER (g_object_new (snippets_browser_get_type (), NULL));
}

/**
 * snippets_browser_load:
 * @snippets_browser: A #SnippetsBrowser object.
 * @snippets_db: A #SnippetsDB object from which the browser should be loaded.
 * @snippets_interaction: A #SnippetsInteraction object which is used for interacting with the editor.
 *
 * Loads the #SnippetsBrowser with snippets that are found in the given database.
 */
void                       
snippets_browser_load (SnippetsBrowser *snippets_browser,
                       SnippetsDB *snippets_db,
                       SnippetsInteraction *snippets_interaction)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	priv->snippets_db = snippets_db;
	priv->snippets_interaction = snippets_interaction;
	g_object_ref (priv->snippets_db);
	g_object_ref (priv->snippets_interaction);
	
	/* Set up the Snippets View */
	priv->snippets_view = GTK_TREE_VIEW (gtk_tree_view_new ());
	init_snippets_view (snippets_browser);

	/* Set up the layout */
	init_browser_layout (snippets_browser);

	/* Initialize the snippet handlers */
	init_browser_handlers (snippets_browser);

	priv->maximized = FALSE;
}

/**
 * snippets_browser_unload:
 * @snippets_browser: A #SnippetsBrowser object.
 * 
 * Unloads the current #SnippetsBrowser object.
 */
void                       
snippets_browser_unload (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	g_object_unref (priv->snippets_db);
	g_object_unref (priv->snippets_interaction);
	priv->snippets_db = NULL;
	priv->snippets_interaction = NULL;

	if (priv->maximized)
	{
		gtk_container_remove (GTK_CONTAINER (snippets_browser),
		                      GTK_WIDGET (priv->browser_hpaned));
	}
	else
	{
		gtk_container_remove (GTK_CONTAINER (snippets_browser),
		                      GTK_WIDGET (priv->snippets_view_vbox));
		g_object_unref (priv->browser_hpaned);
	}

	g_object_unref (priv->filter);
}

/**
 * snippets_browser_show_editor:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Shows the editor attached to the browser. Warning: This will take up considerably
 * more space than just having the browser shown.
 */
void                       
snippets_browser_show_editor (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreeSelection *selection = NULL;
	GtkTreeViewColumn *col = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	if (priv->maximized)
		return;

	/* Unparent the SnippetsView from the SnippetsBrowser */
	g_object_ref (priv->snippets_view_vbox);
	gtk_container_remove (GTK_CONTAINER (snippets_browser),
	                      GTK_WIDGET (priv->snippets_view_vbox));

	/* Add the SnippetsView to the HPaned */
	gtk_paned_pack1 (GTK_PANED (priv->browser_hpaned),
	                 GTK_WIDGET (priv->snippets_view_vbox),
	                 TRUE, FALSE);
	g_object_unref (priv->snippets_view_vbox);

	/* Add the HPaned in the SnippetsBrowser */
	gtk_box_pack_start (GTK_BOX (snippets_browser),
	                    priv->browser_hpaned,
	                    TRUE,
	                    TRUE,
	                    0);
	
	/* Show the editor widgets */
	gtk_widget_show (priv->browser_hpaned);
	gtk_widget_show (GTK_WIDGET (priv->snippets_editor)); 
	
	priv->maximized = TRUE;

	snippets_browser_refilter_snippets_view (snippets_browser);
	
	gtk_widget_set_sensitive (GTK_WIDGET (priv->insert_button), FALSE);

	/* Set the current snippet for the editor */
	selection = gtk_tree_view_get_selection (priv->snippets_view);
	on_snippets_view_selection_changed (selection, snippets_browser);

	col = gtk_tree_view_get_column (priv->snippets_view, SNIPPETS_VIEW_COL_LANGUAGES);
	g_object_set (col, "visible", TRUE, NULL);
	
}

/**
 * snippets_browser_hide_editor:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Hides the editor attached to the browser. 
 */
void                       
snippets_browser_hide_editor (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreeViewColumn *col = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	if (!priv->maximized)
		return;
	
	/* Hide the editor widgets */
	gtk_widget_hide (GTK_WIDGET (priv->snippets_editor)); 
	gtk_widget_hide (priv->browser_hpaned);

	/* Remove the SnippetsView from the HPaned */
	g_object_ref (priv->snippets_view_vbox);
	gtk_container_remove (GTK_CONTAINER (priv->browser_hpaned),
	                      GTK_WIDGET (priv->snippets_view_vbox));
	
	/* Remove the HPaned from the SnippetsBrowser */
	g_object_ref (priv->browser_hpaned);
	gtk_container_remove (GTK_CONTAINER (snippets_browser),
	                      GTK_WIDGET (priv->browser_hpaned));
	g_object_unref (priv->browser_hpaned);
	
	/* Add the SnippetsView to the SnippetsBrowser */
	gtk_box_pack_start (GTK_BOX (snippets_browser),
	                    GTK_WIDGET (priv->snippets_view_vbox),
	                    TRUE,
	                    TRUE,
	                    0);
	g_object_unref (priv->snippets_view_vbox);
	
	priv->maximized = FALSE;

	snippets_browser_refilter_snippets_view (snippets_browser);

	gtk_widget_set_sensitive (GTK_WIDGET (priv->insert_button), TRUE);

	col = gtk_tree_view_get_column (priv->snippets_view, SNIPPETS_VIEW_COL_LANGUAGES);
	g_object_set (col, "visible", FALSE, NULL);
}

/**
 * snippets_browser_refilter_snippets_view:
 * @snippets_browser: A #SnippetsBrowser object.
 *
 * Refilters the Snippets View (if snippets_browser->show_only_document_language_snippets
 * is TRUE), showing just the snippets of the current document. If the Snippets Browser
 * has the editor shown, it will show all the snippets, regardless of the option.
 */
void                       
snippets_browser_refilter_snippets_view (SnippetsBrowser *snippets_browser)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (snippets_browser));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

/* Handlers definitions */

static void    
on_add_button_clicked (GtkButton *add_button,
                       gpointer user_data)
{
	GtkWidget *menu = NULL, *add_snippet_menu_item = NULL, 
	          *add_snippets_group_menu_item = NULL;
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	menu = gtk_menu_new ();

	/* Insert the Add Snippet menu item */
	add_snippet_menu_item = gtk_menu_item_new_with_label (_("Add Snippet …"));
	g_signal_connect (GTK_OBJECT (add_snippet_menu_item),
	                  "activate",
	                  GTK_SIGNAL_FUNC (on_add_snippet_menu_item_activated),
	                  snippets_browser);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), 
	                       GTK_WIDGET (add_snippet_menu_item));
	gtk_widget_show (GTK_WIDGET (add_snippet_menu_item));
	
	/* Insert the Add Snippets Group menu item */
	add_snippets_group_menu_item = gtk_menu_item_new_with_label (_("Add Snippets Group …"));
	g_signal_connect (GTK_OBJECT (add_snippets_group_menu_item),
	                  "activate",
	                  GTK_SIGNAL_FUNC (on_add_snippets_group_menu_item_activated),
	                  snippets_browser);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), 
	                       GTK_WIDGET (add_snippets_group_menu_item));
	gtk_widget_show (GTK_WIDGET (add_snippets_group_menu_item));
	
	gtk_menu_popup (GTK_MENU (menu),
	                NULL, NULL, NULL, NULL, 0,
	                gtk_get_current_event_time ());

}

static void    
on_delete_button_clicked (GtkButton *delete_button,
                          gpointer user_data)
{
	GtkTreeIter iter;
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	gboolean has_selection = FALSE;
	GObject *cur_object = NULL;
	GtkTreeSelection *selection = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (GTK_IS_TREE_MODEL (priv->filter));
	
	selection = gtk_tree_view_get_selection (priv->snippets_view);
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	has_selection = gtk_tree_selection_get_selected (selection,
	                                                 &priv->filter,
	                                                 &iter);
	if (has_selection)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->filter), &iter,
		                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
		                    -1);

		if (ANJUTA_IS_SNIPPET (cur_object))
		{
			const gchar *trigger_key = NULL, *language = NULL;

			trigger_key = snippet_get_trigger_key (ANJUTA_SNIPPET (cur_object));
			language = snippet_get_any_language (ANJUTA_SNIPPET (cur_object));
			g_return_if_fail (trigger_key != NULL);
			
			snippets_db_remove_snippet (priv->snippets_db, trigger_key, language, TRUE);
		}
		else
		{
			/* It's a SnippetsGroup object */
			const gchar *name = NULL;

			name = snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (cur_object));
			g_return_if_fail (name != NULL);
			snippets_db_remove_snippets_group (priv->snippets_db, name);
		}
		
		g_object_unref (cur_object);
	}

	snippets_db_save_snippets (priv->snippets_db);
	
}

static void    
on_insert_button_clicked (GtkButton *insert_button,
                          gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreeSelection *selection = NULL;
	GObject *cur_object = NULL;
	GtkTreeIter iter;
	gboolean has_selection = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (priv->snippets_interaction));

	selection = gtk_tree_view_get_selection (priv->snippets_view);
	has_selection = gtk_tree_selection_get_selected (selection,
	                                                 &priv->filter,
	                                                 &iter);
	if (has_selection)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->filter), &iter,
		                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
		                    -1);

		if (!ANJUTA_IS_SNIPPET (cur_object))
			return;

		snippets_interaction_insert_snippet (priv->snippets_interaction,
		                                     priv->snippets_db,
		                                     ANJUTA_SNIPPET (cur_object));
	}

}

static void    
on_edit_button_toggled (GtkToggleButton *edit_button,
                        gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	/* Request a maximize/unmaximize (which should be caught by the plugin) */
	if (!priv->maximized)
		g_signal_emit_by_name (G_OBJECT (snippets_browser),
		                       "maximize-request");
	else
		g_signal_emit_by_name (G_OBJECT (snippets_browser),
		                       "unmaximize-request");
}

static void    
on_snippets_view_row_activated (GtkTreeView *snippets_view,
                                GtkTreePath *path,
                                GtkTreeViewColumn *col,
                                gpointer user_data)
{
	GtkTreeIter iter;
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GObject *cur_object = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (priv->snippets_interaction));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));
	
	gtk_tree_model_get_iter (priv->filter, &iter, path);
	gtk_tree_model_get (priv->filter, &iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);

	if (ANJUTA_IS_SNIPPET (cur_object))
		snippets_interaction_insert_snippet (priv->snippets_interaction,
		                                     priv->snippets_db,
		                                     ANJUTA_SNIPPET (cur_object));

	g_object_unref (cur_object);
}


static gboolean    
on_snippets_view_query_tooltip (GtkWidget *snippets_view,
                                gint x, 
                                gint y,
                                gboolean keyboard_mode,
                                GtkTooltip *tooltip,
                                gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreeIter iter;
	GObject *cur_object = NULL;
		
	/* Assertions */
	g_return_val_if_fail (GTK_IS_TREE_VIEW (snippets_view), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data), FALSE);
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (user_data);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db), FALSE);
	g_return_val_if_fail (GTK_IS_TREE_MODEL (priv->filter), FALSE);

	/* Get the object at the current row */
	if (!gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (snippets_view),
	                                        &x, &y, keyboard_mode,
	                                        NULL, NULL,
	                                        &iter))
		return FALSE;
	gtk_tree_model_get (GTK_TREE_MODEL (priv->filter), &iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);

	if (ANJUTA_IS_SNIPPET (cur_object))
	{
		gchar *default_content = NULL, *default_content_preview = NULL, *tooltip_text = NULL;

		default_content = snippet_get_default_content (ANJUTA_SNIPPET (cur_object),
		                                               G_OBJECT (priv->snippets_db),
		                                               "");

		default_content_preview = g_strndup (default_content, TOOLTIP_SIZE);
		tooltip_text = g_strconcat (default_content_preview, " …", NULL);
		gtk_tooltip_set_text (tooltip, tooltip_text);
		
		g_free (default_content);
		g_free (default_content_preview);
		g_free (tooltip_text);
		g_object_unref (cur_object);
		
		return TRUE;
	}

	g_object_unref (cur_object);
	return FALSE;
}


static void     
on_name_changed (GtkCellRendererText *renderer,
                 gchar *path_string,
                 gchar *new_text,
                 gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gchar *old_name = NULL;
		
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);

	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (priv->filter, &iter, path);
	gtk_tree_model_get (priv->filter, &iter,
	                    SNIPPETS_DB_MODEL_COL_NAME, &old_name,
	                    -1);
	
	snippets_db_set_snippets_group_name (priv->snippets_db, old_name, new_text);
	snippets_browser_refilter_snippets_view (snippets_browser);

	snippets_db_save_snippets (priv->snippets_db);
	
	gtk_tree_path_free (path);
	g_free (old_name);
}


static void     
on_add_snippet_menu_item_activated (GtkMenuItem *menu_item,
                                    gpointer user_data)
{
	SnippetsBrowserPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (user_data);

	/* Request a maximize/unmaximize (which should be caught by the plugin) */
	if (!priv->maximized)
		g_signal_emit_by_name (G_OBJECT (user_data),
		                       "maximize-request");
		                       
	snippets_editor_set_snippet_new (priv->snippets_editor);
}

static void     
on_add_snippets_group_menu_item_activated (GtkMenuItem *menu_item,
                                           gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreeIter iter;
	AnjutaSnippetsGroup *snippets_group = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));

	snippets_group = snippets_group_new (NEW_SNIPPETS_GROUP_NAME);
	snippets_db_add_snippets_group (priv->snippets_db, snippets_group, FALSE);

	/* The snippets database shouldn't be empty here */
	if (!gtk_tree_model_get_iter_first (priv->filter, &iter))
		g_return_if_reached ();

	do
	{
		gchar *name = NULL;
		
		gtk_tree_model_get (priv->filter, &iter,
		                    SNIPPETS_DB_MODEL_COL_NAME, &name,
		                    -1);
		if (!g_strcmp0 (name, NEW_SNIPPETS_GROUP_NAME))
		{
			GtkTreePath *path = gtk_tree_model_get_path (priv->filter, &iter);
			GtkTreeViewColumn *col = gtk_tree_view_get_column (priv->snippets_view, 
			                                                   SNIPPETS_VIEW_COL_NAME);
			gtk_tree_view_set_cursor (priv->snippets_view, path, col, TRUE);

			/* We save the database after the cursor was set. */
			snippets_db_save_snippets (priv->snippets_db);

			gtk_tree_path_free (path);
			g_free (name);
			return;
		}
		
		g_free (name);
		
	} while (gtk_tree_model_iter_next (priv->filter, &iter));

	/* We should have found the newly added group */
	g_return_if_reached ();
}


static void
on_snippets_view_selection_changed (GtkTreeSelection *tree_selection,
                                    gpointer user_data)
{
	SnippetsBrowser *snippets_browser = NULL;
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreeIter iter;
	GObject *cur_object = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	snippets_browser = ANJUTA_SNIPPETS_BROWSER (user_data);
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (snippets_browser);
	g_return_if_fail (GTK_IS_TREE_MODEL (priv->filter));
	
	if (!gtk_tree_selection_get_selected (tree_selection, &priv->filter, &iter))
	{
		snippets_editor_set_snippet (priv->snippets_editor, NULL);
		return;
	}
	
	gtk_tree_model_get (priv->filter, &iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
	                    -1);

	/* We only change the snippet of the editor if the browser has the editor shown */
	if (ANJUTA_IS_SNIPPET (cur_object) && priv->maximized)
		snippets_editor_set_snippet (priv->snippets_editor, 
		                             ANJUTA_SNIPPET (cur_object));
	else if (priv->maximized)
		snippets_editor_set_snippet (priv->snippets_editor, NULL);

	g_object_unref (cur_object);
}


static void
on_snippets_editor_snippet_saved (SnippetsEditor *snippets_editor,
                                  GObject *snippet,
                                  gpointer user_data)
{
	SnippetsBrowserPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (user_data);

	/* Focus on the newly inserted snippet (the path is valid because when the editor
	   is shown, all snippets are visibile.)*/
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter)); 
	path = snippets_db_get_path_at_object (priv->snippets_db, snippet);
	gtk_tree_view_set_cursor (priv->snippets_view, path, NULL, FALSE);

	/* We save the database after we set the cursor */
	snippets_db_save_snippets (priv->snippets_db);
}


static void
on_snippets_editor_close_request (SnippetsEditor *snippets_editor,
                                  gpointer user_data)
{
	SnippetsBrowserPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_BROWSER (user_data));
	priv = ANJUTA_SNIPPETS_BROWSER_GET_PRIVATE (user_data);

	gtk_toggle_button_set_active (priv->edit_button, FALSE);
}
