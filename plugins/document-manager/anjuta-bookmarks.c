/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-trunk
 * Copyright (C) Johannes Schmid 2008 <jhs@gnome.org>
 * 
 * anjuta-trunk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta-trunk is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define DEBUG 1

#include "anjuta-bookmarks.h"
#include "anjuta-docman.h"
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/anjuta-debug.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>

#define BOOKMARKS_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANJUTA_TYPE_BOOKMARKS, AnjutaBookmarksPrivate))

#define ANJUTA_STOCK_BOOKMARK_TOGGLE          "anjuta-bookmark-toggle"

#define ENCODING "UTF-8"


typedef struct _AnjutaBookmarksPrivate AnjutaBookmarksPrivate;

struct _AnjutaBookmarksPrivate 
{
	GtkWidget* window;
	GtkWidget* tree;
	GtkTreeModel* model;
	GtkCellRenderer* renderer;
	GtkTreeViewColumn* column;
	
	GtkWidget* button_add;
	GtkWidget* button_remove;
	GtkWidget* grip;

	IAnjutaSymbolQuery* query_scope;
	GtkWidget* menu;
	DocmanPlugin* docman;
};

enum
{
	COLUMN_TEXT = 0,
	COLUMN_FILE,
	COLUMN_LINE,
	COLUMN_HANDLE,
	N_COLUMNS
};

G_DEFINE_TYPE (AnjutaBookmarks, anjuta_bookmarks, G_TYPE_OBJECT);

static gchar*
anjuta_bookmarks_get_text (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor, gint line, gboolean use_selection);

static void
on_document_changed (AnjutaDocman *docman, IAnjutaDocument *doc,
					 AnjutaBookmarks *bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	gboolean status =  IANJUTA_IS_EDITOR(doc);
	gtk_widget_set_sensitive (GTK_WIDGET(priv->button_add), status);
}

static void
on_add_clicked (GtkWidget* button, AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	IAnjutaDocument* doc =
		anjuta_docman_get_current_document (ANJUTA_DOCMAN(priv->docman->docman));
	g_return_if_fail (IANJUTA_IS_EDITOR(doc));
	IAnjutaEditor* editor = IANJUTA_EDITOR(doc);
	anjuta_bookmarks_add (bookmarks, editor, 
						  ianjuta_editor_get_lineno (editor, NULL), NULL,TRUE);						 
}

static void
on_remove_clicked (GtkWidget* button, AnjutaBookmarks* bookmarks)
{					 
	anjuta_bookmarks_remove (bookmarks);
}

static void
on_rename (GtkWidget* menuitem, AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkTreeSelection* selection = 
			gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));

	GList* rows =
		gtk_tree_selection_get_selected_rows (selection, NULL);

	GtkTreePath* path;

	// We shouldn't get here otherwise
	g_assert (g_list_length(rows) == 1);
	
	g_object_set (G_OBJECT(priv->renderer), "editable", TRUE, NULL);

	path = rows->data; // First and only row
	anjuta_shell_present_widget (ANJUTA_PLUGIN(priv->docman)->shell,
	                             priv->window, NULL);
	gtk_widget_grab_focus (priv->tree);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->tree), path,
	                              priv->column, FALSE, 0.0, 0.0);

	gtk_tree_view_set_cursor_on_cell (GTK_TREE_VIEW (priv->tree), path,
	                                  priv->column,
	                                  priv->renderer,
	                                  TRUE);
	g_list_foreach (rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (rows);
}

static void
on_row_activate (GtkTreeView* view, GtkTreePath* path,
				 GtkTreeViewColumn* column, AnjutaBookmarks* bookmarks)
{
	GtkTreeIter iter;
	GFile* file;
	gint line;
	gint handle;
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	IAnjutaEditor* editor;
	gtk_tree_model_get_iter (priv->model, &iter, path);
	gtk_tree_model_get (priv->model, &iter,
						COLUMN_FILE, &file,
						COLUMN_LINE, &line,
	                    COLUMN_HANDLE, &handle,
						-1);
	
	editor = anjuta_docman_goto_file_line (ANJUTA_DOCMAN(priv->docman->docman), file, line);
	if (editor)
	{
		line = ianjuta_markable_location_from_handle (IANJUTA_MARKABLE (editor),
		                                                        handle, NULL);
		if (line >= 0)
		{
			gchar* new_title = anjuta_bookmarks_get_text (bookmarks,
			                                              editor,
			                                              line,
			                                              FALSE);
			anjuta_docman_goto_file_line (ANJUTA_DOCMAN(priv->docman->docman), file, line);
			gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter,
			                    COLUMN_TEXT, new_title,
			                    COLUMN_LINE, line, -1);
			g_free (new_title);
		}
	}
	g_object_unref (file);
}

static gboolean
on_button_press_event (GtkWidget *widget, GdkEventButton *event, AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	/* Ignore double-clicks and triple-clicks */
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
	{
		gint button;
		gint event_time;
		
		GtkTreeSelection* selection = 
			gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

		if (gtk_tree_selection_count_selected_rows (selection) == 1)
		{
			/*g_signal_connect (menu, "deactivate", 
			                  G_CALLBACK (gtk_widget_destroy), NULL);*/
			
			if (event)
			{
				button = event->button;
				event_time = event->time;
			}
			else
			{
				button = 0;
				event_time = gtk_get_current_event_time ();
			}
			gtk_menu_popup (GTK_MENU(priv->menu), NULL, NULL, NULL, NULL, button, event_time);

			return TRUE;
		}
	}

	return FALSE;
}

static void
on_document_added (AnjutaDocman* docman, IAnjutaDocument* doc,
				   AnjutaBookmarks* bookmarks)
{
	IAnjutaMarkable* markable;
	GtkTreeIter iter;
	GFile* editor_file;
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	
	if (!IANJUTA_IS_MARKABLE(doc))
		return;
	
	markable = IANJUTA_MARKABLE(doc);
	if (!gtk_tree_model_get_iter_first (priv->model, &iter))
		return;
	
	editor_file = ianjuta_file_get_file (IANJUTA_FILE(doc), NULL);
	if (editor_file == NULL)
		return;
	
	do
	{
		GFile* file;
		gint line;
		
		gtk_tree_model_get (priv->model, &iter,
							COLUMN_FILE, &file,
							COLUMN_LINE, &line,
							-1);
		if (g_file_equal (file, editor_file))
		{
			if (!ianjuta_markable_is_marker_set (markable,
												 line,
												 IANJUTA_MARKABLE_BOOKMARK,
												 NULL))
			{
				int handle = ianjuta_markable_mark (markable, line,
												   IANJUTA_MARKABLE_BOOKMARK,
												   NULL, NULL);
				gtk_list_store_set (GTK_LIST_STORE(priv->model),
									&iter,
									COLUMN_HANDLE,
									handle, -1);
			}
		}
		g_object_unref (file);		
	}
	while (gtk_tree_model_iter_next (priv->model, &iter));
	g_object_unref (editor_file);
}

static void
on_selection_changed (GtkTreeSelection* selection, AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	gboolean status = (gtk_tree_selection_count_selected_rows (selection) > 0);
	gtk_widget_set_sensitive (priv->button_remove, status);
}

static void
on_title_edited (GtkCellRendererText *cell,
				 gchar *path_string,
				 gchar *new_text,
				 AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkTreeModel *model = priv->model;
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_TEXT, new_text, -1);

	g_object_set (G_OBJECT(priv->renderer), "editable", FALSE, NULL);
	gtk_tree_path_free (path);
}

static void
anjuta_bookmarks_init (AnjutaBookmarks *bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkWidget* scrolled_window;
	GtkTreeSelection* selection;

	GtkWidget* item_rename;
	GtkWidget* item_remove;

	priv->window = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window),
										 GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (priv->window), 
						scrolled_window,
						TRUE, TRUE, 0);
	
	priv->model = GTK_TREE_MODEL(gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, 
													 G_TYPE_OBJECT, G_TYPE_INT, G_TYPE_INT));
	priv->tree = gtk_tree_view_new_with_model (priv->model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(priv->tree), FALSE);
	priv->renderer = gtk_cell_renderer_text_new();
	g_signal_connect (G_OBJECT(priv->renderer), "edited", G_CALLBACK(on_title_edited),
					  bookmarks);
	priv->column = gtk_tree_view_column_new_with_attributes ("Bookmark", priv->renderer,
													   "text", COLUMN_TEXT, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->tree), priv->column);
	gtk_container_add (GTK_CONTAINER(scrolled_window),
					   priv->tree);
	
	g_signal_connect (G_OBJECT(priv->tree), "row-activated", G_CALLBACK(on_row_activate),
					  bookmarks);
	g_signal_connect (G_OBJECT(priv->tree), "button-press-event", G_CALLBACK(on_button_press_event),
					  bookmarks);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT(selection), "changed", G_CALLBACK(on_selection_changed),
					  bookmarks);
	
	priv->grip = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	priv->button_add = gtk_button_new();
	gtk_widget_set_tooltip_text (priv->button_add,
	                             _("Add bookmark"));
	gtk_container_add (GTK_CONTAINER (priv->button_add),
	                   gtk_image_new_from_stock (GTK_STOCK_ADD,
	                                             GTK_ICON_SIZE_MENU));
	priv->button_remove = gtk_button_new();
	gtk_widget_set_tooltip_text (priv->button_remove,
	                             _("Remove bookmark"));
	gtk_container_add (GTK_CONTAINER (priv->button_remove),
	                   gtk_image_new_from_stock (GTK_STOCK_REMOVE,
	                                             GTK_ICON_SIZE_MENU));
	g_signal_connect (G_OBJECT(priv->button_add), "clicked", G_CALLBACK(on_add_clicked), bookmarks);
	g_signal_connect (G_OBJECT(priv->button_remove), "clicked", G_CALLBACK(on_remove_clicked), bookmarks);
	gtk_widget_set_sensitive (GTK_WIDGET(priv->button_add), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET(priv->button_remove), FALSE);

	gtk_box_pack_start (GTK_BOX(priv->grip),
	                    gtk_image_new_from_stock (ANJUTA_STOCK_BOOKMARK_TOGGLE,
	                                              GTK_ICON_SIZE_MENU),
	                    FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX(priv->grip),
	                    gtk_label_new (_("Bookmarks")),
	                    FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX(priv->grip), priv->button_add, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(priv->grip), priv->button_remove, FALSE, FALSE, 0);
	gtk_widget_show_all (priv->grip);

	/* Create popup menu */
	priv->menu = gtk_menu_new ();
	item_rename = gtk_menu_item_new_with_label (_("Rename"));
	item_remove = gtk_menu_item_new_with_label (_("Remove"));

	g_signal_connect (item_rename, "activate", G_CALLBACK(on_rename), bookmarks);
	g_signal_connect (item_remove, "activate", G_CALLBACK(on_remove_clicked), bookmarks);			

	gtk_menu_shell_append (GTK_MENU_SHELL(priv->menu), item_rename);
	gtk_menu_shell_append (GTK_MENU_SHELL(priv->menu), item_remove);

	gtk_widget_show_all (priv->menu);
	gtk_menu_attach_to_widget (GTK_MENU (priv->menu), priv->tree, NULL);
	
	gtk_widget_show_all (priv->window);
}

static void
anjuta_bookmarks_finalize (GObject *object)
{
	AnjutaBookmarks* bookmarks = ANJUTA_BOOKMARKS (object);
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	
	gtk_widget_destroy (priv->menu);
	
	anjuta_shell_remove_widget (ANJUTA_PLUGIN(priv->docman)->shell,
								priv->window,
								NULL);
	
	G_OBJECT_CLASS (anjuta_bookmarks_parent_class)->finalize (object);
}

static void
anjuta_bookmarks_class_init (AnjutaBookmarksClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_bookmarks_finalize;
	
	g_type_class_add_private (klass, sizeof (AnjutaBookmarksPrivate));
}


AnjutaBookmarks*
anjuta_bookmarks_new (DocmanPlugin* docman)
{
	AnjutaBookmarks* bookmarks = ANJUTA_BOOKMARKS (g_object_new (ANJUTA_TYPE_BOOKMARKS, 
																 NULL));
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	priv->docman = docman;

	anjuta_shell_add_widget_custom (ANJUTA_PLUGIN(docman)->shell,
	                                priv->window,
	                                "bookmarks",
	                                _("Bookmarks"),
	                                ANJUTA_STOCK_BOOKMARK_TOGGLE,
	                                priv->grip,
	                                ANJUTA_SHELL_PLACEMENT_RIGHT,
	                                NULL);
	
	g_signal_connect (G_OBJECT(docman->docman), "document-changed",
					  G_CALLBACK(on_document_changed), bookmarks);
	g_signal_connect (G_OBJECT(docman->docman), "document-added",
					  G_CALLBACK(on_document_added), bookmarks);	
	return bookmarks;
}

static IAnjutaSymbolQuery*
anjuta_bookmarks_create_query (AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	IAnjutaSymbolManager* sym_manager;
	static IAnjutaSymbolField fields[] = {IANJUTA_SYMBOL_FIELD_NAME};
	
	sym_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN(priv->docman)->shell,
											  IAnjutaSymbolManager, NULL);
	
	IAnjutaSymbolQuery * query_scope =
		ianjuta_symbol_manager_create_query (sym_manager,
											 IANJUTA_SYMBOL_QUERY_SEARCH_SCOPE,
											  IANJUTA_SYMBOL_QUERY_DB_PROJECT,
											  NULL);
	ianjuta_symbol_query_set_fields (query_scope, 1, fields, NULL);
	
	return query_scope;
}

static gchar*
anjuta_bookmarks_get_text_from_file (AnjutaBookmarks* bookmarks, GFile* file, gint line)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	gchar* text;
	
	
	if (priv->query_scope != NULL)
	{
		gchar* path = g_file_get_path (file);
		IAnjutaIterable* iter = 
			ianjuta_symbol_query_search_scope (priv->query_scope,
											   path, line, NULL);
		g_free (path);
		if (iter)
		{
			const gchar* symbol_name;
			symbol_name = ianjuta_symbol_get_string (IANJUTA_SYMBOL(iter),
			                                         IANJUTA_SYMBOL_FIELD_NAME,
			                                         NULL);
			text = g_strdup_printf ("%s:%d", symbol_name,
			                        line);
			g_object_unref (iter);			
			return text;
		}
	}
	{
		GFileInfo* info;
		/* As last chance, take file + line */
		info = g_file_query_info (file,
								  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
								  G_FILE_QUERY_INFO_NONE,
								  NULL,
								  NULL);
		text = g_strdup_printf ("%s:%d",  g_file_info_get_display_name (info),
								line);
		g_object_unref (info);
		return text;
	}
}

static gchar*
anjuta_bookmarks_get_text (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor, gint line, gboolean use_selection)
{	
	/* If we have a (short) selection, take this */
	if (IANJUTA_IS_EDITOR_SELECTION(editor) && use_selection)
	{
		IAnjutaEditorSelection* selection = IANJUTA_EDITOR_SELECTION(editor);
		if (ianjuta_editor_selection_has_selection (selection, NULL))
		{
			gchar* text = ianjuta_editor_selection_get (selection, NULL);
			if (strlen (text) < 100)
				return text;
			g_free (text);
		}
	}
	{
		GFile* file = ianjuta_file_get_file (IANJUTA_FILE(editor), NULL);
		gchar* text =  anjuta_bookmarks_get_text_from_file (bookmarks,
															file,
															line);
		g_object_unref (file);
		return text;
	}								  
}

static void
anjuta_bookmarks_check_selection (AnjutaBookmarks* bookmarks,
                                  GtkTreePath* path)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->tree));
	GList* selected = gtk_tree_selection_get_selected_rows (selection,
															NULL);
	if (!selected)
	{
		GtkTreeIter iter;
		if (!path || !gtk_tree_model_get_iter (priv->model, &iter, path))
		{
			if (!gtk_tree_model_get_iter_first (priv->model, &iter))
				return;
		}
		gtk_tree_selection_select_iter (selection, &iter);
	}	
}

void
anjuta_bookmarks_add (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor, gint line, 
					  const gchar* title, gboolean use_selection)
{
	g_return_if_fail (IANJUTA_IS_MARKABLE(editor));
	IAnjutaMarkable* markable = IANJUTA_MARKABLE(editor);
	GtkTreeIter iter;
	gint handle;
	gchar* text;
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GFile* file;
	
	/* If there is already a marker -> do nothing */
	if (ianjuta_markable_is_marker_set (markable, line, IANJUTA_MARKABLE_BOOKMARK, NULL))
		return;
	
	handle = ianjuta_markable_mark (markable, line, IANJUTA_MARKABLE_BOOKMARK, NULL, NULL);
	
	gtk_list_store_append (GTK_LIST_STORE(priv->model), &iter);
	if (title == NULL)
		text = anjuta_bookmarks_get_text (bookmarks, editor, line, use_selection);
	else
		text = g_strdup(title);
	file = ianjuta_file_get_file(IANJUTA_FILE(editor), NULL);
	/* The buffer is not saved yet -> do nothing */
	if (file == NULL)
		return;
	gtk_list_store_set (GTK_LIST_STORE(priv->model), &iter, 
						COLUMN_TEXT, text,
						COLUMN_FILE, file,
						COLUMN_LINE, line,
						COLUMN_HANDLE, handle,
						-1);
	g_free(text);
	g_object_unref (file);
}

void
anjuta_bookmarks_remove (AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->tree));
	GList* selected = gtk_tree_selection_get_selected_rows (selection,
															NULL);
	GList* node;
	GList* refs = NULL;
	GtkTreePath* first_path = NULL;
	for (node = selected; node != NULL; node = g_list_next (node))
	{
		GtkTreeRowReference* ref = gtk_tree_row_reference_new (priv->model,
															   node->data);
		refs = g_list_append (refs, ref);
	}
	g_list_foreach (selected, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (selected);
	for (node = refs; node != NULL; node = g_list_next (node))
	{
		GFile* file;
		gint line;
		IAnjutaEditor* editor;
		GtkTreeIter iter;
		GtkTreeRowReference* ref = node->data;
		GtkTreePath* path = gtk_tree_row_reference_get_path(ref);
		gtk_tree_model_get_iter (priv->model,
								 &iter,
								 path);
		if (!first_path)
		{
			first_path = path;
		}
		else
		{
			gtk_tree_path_free (path);
		}
		gtk_tree_model_get (priv->model, &iter, 
							COLUMN_FILE, &file,
							COLUMN_LINE, &line,
							-1);
		editor = IANJUTA_EDITOR(anjuta_docman_get_document_for_file (ANJUTA_DOCMAN(priv->docman->docman),
																	 file));
		if (editor)
		{
			if (ianjuta_markable_is_marker_set (IANJUTA_MARKABLE(editor),
												line, IANJUTA_MARKABLE_BOOKMARK, NULL))
			{
				ianjuta_markable_unmark (IANJUTA_MARKABLE(editor), line,
										 IANJUTA_MARKABLE_BOOKMARK, NULL);
			}
		}
		g_object_unref (file);
		
		gtk_list_store_remove (GTK_LIST_STORE (priv->model), &iter);
	}

	anjuta_bookmarks_check_selection (bookmarks, first_path);
	if (first_path)
		gtk_tree_path_free (first_path);
	
	g_list_foreach (refs, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free (refs);
}

void 
anjuta_bookmarks_add_file (AnjutaBookmarks* bookmarks,
						   GFile* file,
						   gint line,
						   const gchar* title)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	IAnjutaDocument* doc;
	GtkTreeIter iter;
	
	if ((doc = anjuta_docman_get_document_for_file (ANJUTA_DOCMAN(priv->docman->docman), file)))
	{
		anjuta_bookmarks_add (bookmarks, IANJUTA_EDITOR(doc), line, title, FALSE);
	}
	else
	{
		gchar* text;
		gtk_list_store_append (GTK_LIST_STORE(priv->model), &iter);
		if (title == NULL)
			text = anjuta_bookmarks_get_text_from_file (bookmarks, file, line);
		else
			text = g_strdup(title);
		gtk_list_store_set (GTK_LIST_STORE(priv->model), &iter, 
							COLUMN_TEXT, text,
							COLUMN_FILE, file,
							COLUMN_LINE, line,
							COLUMN_HANDLE, -1,
							-1);
		g_free (text);
	}
}

void
anjuta_bookmarks_session_save (AnjutaBookmarks* bookmarks, AnjutaSession* session)
{
	LIBXML_TEST_VERSION;
	int rc;
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	GtkTreeIter iter;
	
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	
	/* Create a new XML buffer, to which the XML document will be
	 * written */
	buf = xmlBufferCreate();
	if (buf == NULL) 
	{
		DEBUG_PRINT ("%s", "XmlwriterMemory: Error creating the xml buffer\n");
		return;
	}
	
	/* Create a new XmlWriter for memory, with no compression.
	 * Remark: there is no compression for this kind of xmlTextWriter */
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) 
	{
		DEBUG_PRINT ("%s", "XmlwriterMemory: Error creating the xml writer\n");
		return;
	}
	
	rc = xmlTextWriterStartDocument(writer, NULL, ENCODING, NULL);
	if (rc < 0) {
		DEBUG_PRINT ("%s", 
					 "XmlwriterMemory: Error at xmlTextWriterStartDocument\n");
					 return;
	}
    rc = xmlTextWriterStartElement(writer, BAD_CAST "bookmarks");
    if (rc < 0) {
        DEBUG_PRINT ("%s", 
            "XmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }
	
	if (gtk_tree_model_get_iter_first (priv->model,
									   &iter))
	{
		do
		{
			gchar* title;
			GFile* file;
			gint line;
			gchar* line_text;
			gchar* uri;
			
			gtk_tree_model_get (priv->model,
								&iter,
								COLUMN_TEXT, &title,
								COLUMN_FILE, &file,
								COLUMN_LINE, &line,
								-1);
			uri = g_file_get_uri (file);
			g_object_unref (file);
			
			rc = xmlTextWriterStartElement(writer, BAD_CAST "bookmark");
			if (rc < 0) {
				DEBUG_PRINT ("%s",
					"XmlwriterMemory: Error at xmlTextWriterStartElement\n");
				return;
			}
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "title",
											 BAD_CAST title);
			g_free (title);
			if (rc < 0) {
				DEBUG_PRINT ("%s",
					"XmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return;
			}
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "uri",
											 BAD_CAST uri);
			g_free (uri);
			if (rc < 0) {
				DEBUG_PRINT ("%s",
					"XmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return;
			}
			line_text = g_strdup_printf ("%d", line);
			rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "line",
											 BAD_CAST line_text);
			g_free (line_text);
			if (rc < 0) {
				DEBUG_PRINT ("%s",
					"XmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
				return;
			}
			/* Close the element named bookmark. */
			rc = xmlTextWriterEndElement(writer);
			if (rc < 0) {
				DEBUG_PRINT ("%s", "XmlwriterMemory: Error at xmlTextWriterEndElement\n");
				return;
			}
		}
		while (gtk_tree_model_iter_next (priv->model, &iter));
	}
	rc = xmlTextWriterEndDocument(writer);
	if (rc < 0) {
		DEBUG_PRINT ("%s", "testXmlwriterMemory: Error at xmlTextWriterEndDocument\n");
		return;
	}
	xmlFreeTextWriter(writer);

	anjuta_session_set_string (session,
							   "Document Manager",
							   "bookmarks",
							   (const gchar*) buf->content);
	xmlBufferFree(buf);
	
	/* Clear the model */
	gtk_list_store_clear (GTK_LIST_STORE (priv->model));

	if (priv->query_scope)
	{
		g_object_unref (priv->query_scope);
		priv->query_scope = NULL;
	}
}

void 
anjuta_bookmarks_toggle (AnjutaBookmarks* bookmarks, 
                         IAnjutaEditor* editor, 
                         gint line)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	
	g_return_if_fail (bookmarks != NULL);
	g_return_if_fail (editor != NULL);

	/* Check if there is a bookmark in that line already */
	if (ianjuta_markable_is_marker_set (IANJUTA_MARKABLE (editor),
	                                    line, IANJUTA_MARKABLE_BOOKMARK, NULL))
	{
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (priv->model, &iter))
		{
			do
			{
				gint handle;
				gint location;
				gtk_tree_model_get (priv->model, &iter,
				                    COLUMN_HANDLE, &handle, -1);

				/* Update location if necessary */
				location = ianjuta_markable_location_from_handle (IANJUTA_MARKABLE (editor),
				                                                  handle, NULL);
				gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter,
					                    COLUMN_LINE, location, -1 );
				if (line == location)
				{
					GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
					gtk_tree_selection_select_iter (selection, &iter);
					anjuta_bookmarks_remove (bookmarks);
				}
			}
			while (gtk_tree_model_iter_next (priv->model, &iter));
		}
	}
	else
	{
		anjuta_bookmarks_add (bookmarks, editor, line, NULL, TRUE);
	}
}

static void
read_bookmarks (AnjutaBookmarks* bookmarks, xmlNodePtr marks)
{
	xmlNodePtr cur;
	for (cur = marks; cur != NULL; cur = cur->next)
	{
		DEBUG_PRINT ("Reading bookmark: %s", cur->name);
		if (xmlStrcmp (cur->name, BAD_CAST "bookmark") == 0)
		{
			gchar* title;
			gchar* line_text;
			gint line;
			gchar* uri;
			GFile* file;
			
			title = (gchar*) xmlGetProp(cur, BAD_CAST "title");
			uri = (gchar*) xmlGetProp(cur, BAD_CAST "uri");
			line_text = (gchar*) xmlGetProp(cur, BAD_CAST "line");
			
			DEBUG_PRINT ("Reading bookmark real: %s", title);

			
			line = atoi(line_text);
			file = g_file_new_for_uri (uri);
			
			anjuta_bookmarks_add_file (bookmarks, file, line, title);
			g_free(uri);
			g_free (title);
		}
	}
}

void
anjuta_bookmarks_session_load (AnjutaBookmarks* bookmarks, AnjutaSession* session)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);	
	gchar* xml_string = anjuta_session_get_string (session,
												   "Document Manager",
												   "bookmarks");
	DEBUG_PRINT("Session load");
	
	if (!xml_string || !strlen(xml_string))
		return;
	xmlDocPtr doc = xmlParseMemory (xml_string,
									strlen (xml_string));
	g_free(xml_string);
	
	xmlNodePtr cur = xmlDocGetRootElement (doc);

	if (cur == NULL)
	{
		xmlFreeDoc (doc);
		return;
	}

	if (xmlStrcmp (cur->name, BAD_CAST "bookmarks") == 0)
		read_bookmarks (bookmarks, cur->children);
	
	xmlFreeDoc (doc);

	priv->query_scope = anjuta_bookmarks_create_query (bookmarks);
}

static gint
line_compare (gconstpointer line1, gconstpointer line2)
{
	gint l1 = GPOINTER_TO_INT(line1);
	gint l2 = GPOINTER_TO_INT(line2);
	return l2 - l1;
}

static GList*
get_bookmarks_for_editor (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GList* marks = NULL;
	GtkTreeIter iter;
	GFile* file;
	if (!gtk_tree_model_get_iter_first (priv->model, &iter))
		return NULL;
	file = ianjuta_file_get_file (IANJUTA_FILE(editor), NULL);
	if (!file)
		return NULL;
	do
	{
		GFile* bookmark_file;
		gint line;
		gtk_tree_model_get (priv->model,
							&iter,
							COLUMN_FILE, &bookmark_file,
							COLUMN_LINE, &line,
							-1);
		if (g_file_equal (file, bookmark_file))
			marks = g_list_insert_sorted (marks, GINT_TO_POINTER(line), line_compare);
		g_object_unref (bookmark_file);
	}
	while (gtk_tree_model_iter_next (priv->model, &iter));
	g_object_unref (file);
	
	return marks;
}

void 
anjuta_bookmarks_next (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor,
					   gint line)
{
	GList* marks = get_bookmarks_for_editor (bookmarks, editor);
	GList* node;

	for (node = marks; node != NULL; node = g_list_next (node))
	{
		gint node_line = GPOINTER_TO_INT (node->data);
		if (node_line > line)
		{
			ianjuta_editor_goto_line (editor, node_line, NULL);
		}
	}
	g_list_free (marks);
}

void 
anjuta_bookmarks_prev (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor,
					   gint line)
{
	GList* marks = get_bookmarks_for_editor (bookmarks, editor);
	GList* node;

	marks = g_list_reverse (marks);
	
	for (node = marks; node != NULL; node = g_list_next (node))
	{
		gint node_line = GPOINTER_TO_INT (node->data);
		if (node_line < line)
		{
			ianjuta_editor_goto_line (editor, node_line, NULL);
		}
	}
	g_list_free (marks);
}

void anjuta_bookmarks_clear (AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->tree));
	gtk_tree_selection_select_all (selection);
	anjuta_bookmarks_remove (bookmarks);
}
