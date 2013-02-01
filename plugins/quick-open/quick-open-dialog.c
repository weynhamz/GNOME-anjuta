/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * quick-open-dialog.c
 * Copyright (C) 2013 Carl-Anton Ingmarsson <carlantoni@src.gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <glib/gi18n.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <string.h>

#include "quick-open-dialog.h"

struct _QuickOpenDialogPrivate
{
    GFile* project_root;

    GtkEntry* filter_entry;
    guint filter_changed_timeout;
    char** filter_strings;

    GtkNotebook* tree_view_notebook;
    GtkTreeView* tree_view;

    GtkListStore* store;
    GtkTreeModelFilter* filter_model;
    GHashTable* project_files_hash;

    GSList* documents;
    GHashTable* document_files_hash;
};

enum {
    COLUMN_IS_SEPARATOR = 0,
    COLUMN_TITLE,
    COLUMN_IS_DOCUMENT,
    COLUMN_OBJECT
};

enum {
    NOTEBOOK_PAGE_LOADING = 0,
    NOTEBOOK_PAGE_VIEW    = 1
};

#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/anjuta-quick-open.ui"

#define FILTER_CHANGED_TIMEOUT 150


G_DEFINE_TYPE (QuickOpenDialog, quick_open_dialog, GTK_TYPE_DIALOG);

static gboolean
iter_next_skip_separator(GtkTreeModel* model, GtkTreeIter* iter)
{
    while (gtk_tree_model_iter_next(model, iter))
    {
        gboolean  is_separator;

        gtk_tree_model_get(model, iter, COLUMN_IS_SEPARATOR, &is_separator, -1);
        if (!is_separator)
            return TRUE;
    }

    return FALSE;
}

static gboolean
iter_first_skip_separator(GtkTreeModel* model, GtkTreeIter* iter)
{
    gboolean is_separator;

    if (!gtk_tree_model_get_iter_first(model, iter))
        return FALSE;

    gtk_tree_model_get(model, iter, COLUMN_IS_SEPARATOR, &is_separator, -1);
    if (!is_separator)
        return TRUE;

    return iter_next_skip_separator(model, iter);
}

static void
quick_open_dialog_select_path(QuickOpenDialog* self, GtkTreePath* path)
{
    GtkTreeSelection* selection;
    selection = gtk_tree_view_get_selection(self->priv->tree_view);

    gtk_tree_selection_select_path(selection, path);
    gtk_tree_view_scroll_to_cell(self->priv->tree_view, path, NULL, TRUE, 0.5, 0);
}

typedef enum
{
    MOVE_SELECTION_FIRST,
    MOVE_SELECTION_END,
    MOVE_SELECTION_DIFF
} MoveSelection;

static void
quick_open_dialog_move_selection(QuickOpenDialog* self, MoveSelection mode, gint howmany)
{
    QuickOpenDialogPrivate* priv = self->priv;

    gint n_items;
    GtkTreeSelection* selection;
    GtkTreeIter iter;
    GtkTreePath* path;
    gboolean is_separator;

    if (!gtk_tree_view_get_model(priv->tree_view))
        return;

    n_items = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->filter_model), NULL);
    if (n_items == 0)
        return;

    selection = gtk_tree_view_get_selection(priv->tree_view);

    if (mode == MOVE_SELECTION_FIRST)
        path = gtk_tree_path_new_first();

    else if (mode == MOVE_SELECTION_END)
        path = gtk_tree_path_new_from_indices(n_items - 1, -1);

    else if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
    {
        if (howmany > 0)
        {
            mode = MOVE_SELECTION_FIRST;
            path = gtk_tree_path_new_first();
        }
        else
        {
            mode = MOVE_SELECTION_END;
            path = gtk_tree_path_new_from_indices(n_items - 1, -1);
        }
    }
    else
    {
        gint* indices;
        gint selected_index;

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->filter_model), &iter);
        indices = gtk_tree_path_get_indices(path);
        selected_index = indices[0];
        gtk_tree_path_free(path);

        selected_index += howmany;
        if (selected_index < 0)
            selected_index = 0;
        else if (selected_index > (n_items - 1))
            selected_index = n_items - 1;

        path = gtk_tree_path_new_from_indices(selected_index, -1);
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->filter_model), &iter, path))
        return;

    gtk_tree_model_get(GTK_TREE_MODEL(priv->filter_model), &iter,
        COLUMN_IS_SEPARATOR, &is_separator, -1);

    if (is_separator)
    {
        gboolean res;

        gtk_tree_path_free(path);

        switch (mode)
        {
            case MOVE_SELECTION_FIRST:
                res = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->filter_model), &iter);
                break;
            case MOVE_SELECTION_END:
                res = gtk_tree_model_iter_previous(GTK_TREE_MODEL(priv->filter_model), &iter);
                break;

            case MOVE_SELECTION_DIFF:
            {
                if (howmany > 0)
                {
                    res = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->filter_model), &iter);
                    if (!res && howmany > 1)
                        res = gtk_tree_model_iter_previous(GTK_TREE_MODEL(priv->filter_model), &iter);
                }
                else
                {
                    res = gtk_tree_model_iter_previous(GTK_TREE_MODEL(priv->filter_model), &iter);
                    if (!res && howmany < -1)
                        res = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->filter_model), &iter);
                }
                break;
            }

            default:
                g_assert_not_reached();
        }

        if (!res)
            return;

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->filter_model), &iter);
    }

    quick_open_dialog_select_path(self, path);
    gtk_tree_path_free(path);
}

static gboolean
quick_open_dialog_filter_item(QuickOpenDialog* self, const char* title, GFile* file)
{
    QuickOpenDialogPrivate* priv = self->priv;
    char** iter;

    /* Don't show project files that are already shown as an open document. */
    if (file && g_hash_table_lookup(priv->document_files_hash, file))
        return FALSE;

    if (!priv->filter_strings)
        return TRUE;

    for (iter = priv->filter_strings; *iter; iter++)
    {
        const char* filter_string = *iter;

        if (!g_strstr_len(title, -1, filter_string))
            return FALSE;
    }

    return TRUE;
}

static gboolean
quick_open_dialog_tree_visible_func(GtkTreeModel *model, GtkTreeIter *iter,
                                    gpointer data)
{
    QuickOpenDialog* self = data;

    gboolean is_separator, is_document, visible;
    char* title;
    GFile* file = NULL;

    gtk_tree_model_get(model, iter, COLUMN_IS_SEPARATOR, &is_separator,
        COLUMN_TITLE, &title, COLUMN_IS_DOCUMENT, &is_document, -1);
    if (is_separator)
        return TRUE;

    if (!is_document)
        gtk_tree_model_get(model, iter, COLUMN_OBJECT, &file, -1);

    visible = quick_open_dialog_filter_item(self, title, file);

    g_free(title);
    if (file)
        g_object_unref(file);

    return visible;
}

static gboolean
quick_open_dialog_row_separator_func(GtkTreeModel* model, GtkTreeIter* iter,
                                     gpointer user_data)
{
    gboolean is_separator;

    gtk_tree_model_get(model, iter, COLUMN_IS_SEPARATOR, &is_separator, -1);
    if (is_separator)
        return TRUE;

    return FALSE;
}


static gint
quick_open_dialog_tree_sort_func(GtkTreeModel* model, GtkTreeIter* a,
                                 GtkTreeIter* b, gpointer user_data)
{
    gboolean is_separator;
    gboolean is_document1, is_document2;
    char* title1, *title2;
    gboolean res;

    gtk_tree_model_get(model, a, COLUMN_IS_SEPARATOR, &is_separator, -1);
    /* a is separator. */
    if (is_separator)
    {
        gtk_tree_model_get(model, b, COLUMN_IS_DOCUMENT, &is_document2, -1);
        if (is_document2)
            return 1;
        else
            return -1;
    }

    gtk_tree_model_get(model, b, COLUMN_IS_SEPARATOR, &is_separator, -1);
    /* b is separator. */
    if (is_separator)
    {
        gtk_tree_model_get(model, a, COLUMN_IS_DOCUMENT, &is_document1, -1);
        if (is_document1)
            return -1;
        else
            return 1;
    }

    gtk_tree_model_get(model, a, COLUMN_IS_DOCUMENT, &is_document1, -1);
    gtk_tree_model_get(model, b, COLUMN_IS_DOCUMENT, &is_document2, -1);

    if (is_document1 && !is_document2)
        return -1;
    if (!is_document1 && is_document2)
        return 1;


    gtk_tree_model_get(model, a, COLUMN_TITLE, &title1, -1);
    gtk_tree_model_get(model, b, COLUMN_TITLE, &title2, -1);

    res = strcmp(title1, title2);
    g_free(title1);
    g_free(title2);

    return res;
}

static void
on_tree_view_row_activated(GtkWidget* widget, GtkTreePath* path,
                           GtkTreeViewColumn* column, gpointer user_data)
{
    QuickOpenDialog* self = user_data;

    gtk_dialog_response(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT);
}

static gboolean
on_filter_entry_key_press_event(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
    QuickOpenDialog* self = user_data;

    if (!(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
    {
        switch (event->keyval)
        {
            case GDK_KEY_Up:
                quick_open_dialog_move_selection(self, MOVE_SELECTION_DIFF, -1);
                return TRUE;
            case GDK_KEY_Down:
                quick_open_dialog_move_selection(self, MOVE_SELECTION_DIFF, 1);
                return TRUE;
            case GDK_KEY_Page_Up:
                quick_open_dialog_move_selection(self, MOVE_SELECTION_DIFF, -5);
                return TRUE;
            case GDK_KEY_Page_Down:
                quick_open_dialog_move_selection(self, MOVE_SELECTION_DIFF, 5);
                return TRUE;
            case GDK_KEY_Home:
                quick_open_dialog_move_selection(self, MOVE_SELECTION_FIRST, 0);
                return TRUE;
            case GDK_KEY_End:
                quick_open_dialog_move_selection(self, MOVE_SELECTION_END, 0);
                return TRUE;

            case GDK_KEY_Return:
            case GDK_KEY_KP_Enter:
                gtk_dialog_response(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT);
                return TRUE;
        }
    }

    return FALSE;
}

static gboolean
filter_changed_timeout(gpointer user_data)
{
    QuickOpenDialog* self = user_data;
    QuickOpenDialogPrivate* priv = self->priv;

    const char* filter_text;

    filter_text = gtk_entry_get_text(priv->filter_entry);

    g_strfreev(priv->filter_strings);

    if (!filter_text || *filter_text == '\0')
        priv->filter_strings = NULL;

    else
        priv->filter_strings = g_strsplit(filter_text, " ", -1);

    gtk_tree_model_filter_refilter(priv->filter_model);

    /* Select the first item. */
    quick_open_dialog_move_selection(self, MOVE_SELECTION_FIRST, 0);

    return G_SOURCE_REMOVE;
}

static void
on_filter_changed(GtkEntry* filter_entry, gpointer user_data)
{
    QuickOpenDialog* self = user_data;
    QuickOpenDialogPrivate* priv = self->priv;

    if (priv->filter_changed_timeout)
        g_source_remove(priv->filter_changed_timeout);

    priv->filter_changed_timeout = g_timeout_add(FILTER_CHANGED_TIMEOUT,
        filter_changed_timeout, self);
}

static void
on_dialog_show(GtkWidget* widget, gpointer user_data)
{
    QuickOpenDialog* self = user_data;
    QuickOpenDialogPrivate* priv = self->priv;

    gtk_widget_grab_focus(GTK_WIDGET(priv->filter_entry));
}

GObject*
quick_open_dialog_get_selected_object(QuickOpenDialog* self)
{
    QuickOpenDialogPrivate* priv = self->priv;

    GtkTreeSelection* selection;
    GtkTreeModel* model;
    GtkTreeIter iter;
    GObject* object;

    selection = gtk_tree_view_get_selection(priv->tree_view);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return NULL;

    gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, -1);
    return object;
}

void
quick_open_dialog_add_project_file(QuickOpenDialog* self, GFile* file)
{
    QuickOpenDialogPrivate* priv = self->priv;

    gboolean in_project_hash;
    char* path;

    if (!g_file_has_prefix(file, self->priv->project_root))
        return;

    in_project_hash = !!g_hash_table_lookup(priv->project_files_hash, file);
    /* Project file already added. */
    if (in_project_hash)
        return;

    if (priv->project_root && g_file_has_prefix(file, priv->project_root))
        path = g_file_get_relative_path(priv->project_root, file);
    else
        path = g_file_get_path(file);

    gtk_list_store_insert_with_values(priv->store, NULL, -1, COLUMN_TITLE, path,
        COLUMN_OBJECT, file, -1);

    g_free(path);

    g_hash_table_add(priv->project_files_hash, g_object_ref(file));
}

void
quick_open_dialog_add_project_files(QuickOpenDialog* self, GSList* files)
{
    QuickOpenDialogPrivate* priv = self->priv;

    GSList* l;

    g_return_if_fail(QUICK_IS_OPEN_DIALOG(self));

    priv = self->priv;


    /* Unset model to keep the tree view from doing unecessary stuff while we
     * insert the files. */
    gtk_tree_view_set_model(priv->tree_view, NULL);

    /* Set sort column id to GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID so that
     * every insert doesn't trigger a resort. It is faster to sort the model once
     * when all files are inserted. */
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->store),
        GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

    /* Add the files. */
    for (l = files; l; l = l->next)
    {
        GFile* file = l->data;
        quick_open_dialog_add_project_file(self, file);
    }

    /* Now we sort the model. */
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->store),
        GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

    /* Set the model again. */
    gtk_tree_view_set_model (priv->tree_view, GTK_TREE_MODEL(priv->filter_model));

    /* Select the first item. */
    quick_open_dialog_move_selection(self, MOVE_SELECTION_FIRST, 0);

    gtk_notebook_set_current_page (priv->tree_view_notebook, NOTEBOOK_PAGE_VIEW);
}

static void
quick_open_dialog_clear_project_files(QuickOpenDialog* self)
{
    QuickOpenDialogPrivate* priv = self->priv;

    GSList* documents;

    gtk_list_store_clear(priv->store);
    g_hash_table_remove_all(priv->project_files_hash);
    g_hash_table_remove_all(priv->document_files_hash);

    /* Insert separator */
    gtk_list_store_insert_with_values(priv->store, NULL, -1,
        COLUMN_IS_SEPARATOR, TRUE, -1);

    /* Readd all documents. */
    documents = priv->documents;
    if (priv->documents)
    {
        g_slist_free(priv->documents);
        priv->documents = NULL;
    }
    for (; documents; documents = documents->next)
    {
        IAnjutaDocument* doc = documents->data;
        quick_open_dialog_add_document(self, doc);
    }
}

void
quick_open_dialog_set_project_root(QuickOpenDialog* self, GFile* project_root)
{
    QuickOpenDialogPrivate* priv = self->priv;

    g_clear_object(&priv->project_root);
    if (project_root)
        priv->project_root = g_object_ref(project_root);

    quick_open_dialog_clear_project_files(self);

    if (project_root)
        gtk_notebook_set_current_page (priv->tree_view_notebook, NOTEBOOK_PAGE_LOADING);
}

static gboolean
remove_matching_value_func(gpointer key, gpointer value, gpointer user_data)
{
    return value == user_data;
}

static void
quick_open_dialog_document_file_changed(QuickOpenDialog* self,
                                        IAnjutaDocument* document)
{
    QuickOpenDialogPrivate* priv = self->priv;

    GFile* file, *old_file;
    char* title;
    GtkTreeModel* model;
    gboolean res;
    GtkTreeIter iter;

    file = ianjuta_file_get_file(IANJUTA_FILE(document), NULL);
    old_file = g_object_get_data(G_OBJECT(document), "quickopen_oldfile");

    /* Don't do anything if the file hasn't changed. */
    if ((file == old_file) || (file && old_file && g_file_equal(file, old_file)))
    {
        if (file)
            g_object_unref(file);
        return;
    }

    /* Remove the old file from the document_files_hash */
    g_hash_table_foreach_remove(priv->document_files_hash,
        remove_matching_value_func, document);

    if (file)
    {
        if (priv->project_root && g_file_has_prefix(file, priv->project_root))
            title = g_file_get_relative_path(priv->project_root, file);
        else
            title = g_file_get_path(file);

        g_hash_table_add(priv->document_files_hash, file); // Takes the ref.
        g_object_set_data_full(G_OBJECT(document), "quickopen_oldfile",
            g_object_ref(file), g_object_unref);
    }
    else
    {
        title = g_strdup(ianjuta_document_get_filename(document, NULL));
        g_object_set_data(G_OBJECT(document), "quickopen_oldfile", NULL);
    }


    /* Find it in the store and update the title. */
    model = GTK_TREE_MODEL(priv->store);
    for (res = iter_first_skip_separator(model, &iter);
         res;
         res = iter_next_skip_separator(model, &iter))
    {
        IAnjutaDocument* doc;
        gboolean found;

        gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &doc, -1);
        if (!doc)
            /* All documents are sorted first so we can stop iterating if it's
             * not been found yet. */
            break;

        found = (doc == document);
        g_object_unref(doc);

        if (found)
        {
            gtk_list_store_set(priv->store, &iter, COLUMN_TITLE, title, -1);
            break;
        }
    }

    g_free(title);
}

static void
on_document_saved(IAnjutaFile* afile, GFile* file, gpointer user_data)
{
    QuickOpenDialog* self = user_data;

    quick_open_dialog_document_file_changed(self, IANJUTA_DOCUMENT(afile));
}

static void
on_document_opened(IAnjutaFile* afile, GError* error, gpointer user_data)
{
    QuickOpenDialog* self = user_data;

    if (error)
        return;

    quick_open_dialog_document_file_changed(self, IANJUTA_DOCUMENT(afile));
}

void
quick_open_dialog_remove_document(QuickOpenDialog* self, IAnjutaDocument* document)
{
    QuickOpenDialogPrivate* priv = self->priv;

    GFile* file;
    GtkTreeModel* model;
    gboolean res;
    GtkTreeIter iter;

    if (!IANJUTA_IS_FILE(document))
        return;

    priv->documents = g_slist_remove (priv->documents, document);

    file = ianjuta_file_get_file(IANJUTA_FILE(document), NULL);
    if (file)
    {
        g_hash_table_remove(priv->document_files_hash, file);
        g_object_unref(file);
    }

    g_signal_handlers_disconnect_by_func(document, on_document_opened, self);
    g_signal_handlers_disconnect_by_func(document, on_document_saved, self);

    /* Find it in the store and remove it */
    model = GTK_TREE_MODEL(priv->store);
    for (res = iter_first_skip_separator(model, &iter);
         res;
         res = iter_next_skip_separator(model, &iter))
    {
        IAnjutaDocument* doc;
        gboolean found;

        gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &doc, -1);
        if (!doc)
            /* All documents are sorted first so we can stop iterating if it's
             * not been found yet. */
            break;

        found = (doc == document);
        g_object_unref(doc);

        if (found)
        {
            gtk_list_store_remove(priv->store, &iter);
            break;
        }
    }
}

void
quick_open_dialog_add_document(QuickOpenDialog* self, IAnjutaDocument* document)
{
    QuickOpenDialogPrivate* priv = self->priv;

    GFile* file;
    char* title;

    if (!IANJUTA_IS_FILE(document))
        return;

    file = ianjuta_file_get_file(IANJUTA_FILE(document), NULL);
    if (file)
    {
        if (priv->project_root && g_file_has_prefix(file, priv->project_root))
            title = g_file_get_relative_path(priv->project_root, file);
        else
            title = g_file_get_path(file);

        g_hash_table_add(priv->document_files_hash, file);
        g_object_set_data_full(G_OBJECT(document), "quickopen_oldfile",
            g_object_ref(file), g_object_unref);
    }
    else
    {
        title = g_strdup(ianjuta_document_get_filename(document, NULL));
        g_object_set_data(G_OBJECT(document), "quickopen_oldfile", NULL);
    }

    gtk_list_store_insert_with_values(priv->store, NULL, -1, COLUMN_TITLE, title,
        COLUMN_IS_DOCUMENT, TRUE, COLUMN_OBJECT, document, -1);

    g_free(title);

    g_signal_connect(document, "opened", G_CALLBACK(on_document_opened), self);

    if (IANJUTA_IS_FILE_SAVABLE(document))
        g_signal_connect(document, "saved", G_CALLBACK(on_document_saved), self);

    priv->documents = g_slist_prepend(priv->documents, document);
}

QuickOpenDialog*
quick_open_dialog_new(void)
{
    return g_object_new(QUICK_TYPE_OPEN_DIALOG, NULL);
}

static void
quick_open_dialog_init (QuickOpenDialog* self)
{
    QuickOpenDialogPrivate* priv;
    GtkBuilder* builder;
    GError* err = NULL;
    GtkGrid* grid;
    GtkCellRenderer* pixbuf_renderer, *text_renderer;

    self->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QUICK_TYPE_OPEN_DIALOG,
        QuickOpenDialogPrivate);

    gtk_window_set_title(GTK_WINDOW(self), _("Quick Open"));
    gtk_window_set_modal(GTK_WINDOW(self), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(self), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(self), 400, 300);

    gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT);

    g_signal_connect(self, "show", G_CALLBACK(on_dialog_show), self);

    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, BUILDER_FILE, &err))
        g_error("Couldn't load builder file: %s", err->message);

    grid = GTK_GRID(gtk_builder_get_object(builder, "grid"));
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))),
        GTK_WIDGET(grid));

#if GTK_CHECK_VERSION(3, 6, 0)
    /* Add search entry here since glade doesn't support it yet. */
    priv->filter_entry = GTK_ENTRY(gtk_search_entry_new());
#else
    priv->filter_entry = gtk_entry_new();
#endif
    gtk_widget_show(GTK_WIDGET(priv->filter_entry));
    gtk_grid_attach(grid, GTK_WIDGET(priv->filter_entry), 0, 0, 1, 1);
    g_signal_connect(priv->filter_entry, "changed",
        G_CALLBACK(on_filter_changed), self);
    g_signal_connect(priv->filter_entry, "key-press-event",
        G_CALLBACK(on_filter_entry_key_press_event), self);

    priv->tree_view_notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder, "treeview_notebook"));
    priv->tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
    g_signal_connect(priv->tree_view, "row-activated",
        G_CALLBACK(on_tree_view_row_activated), self);

    pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(pixbuf_renderer, "icon-name", "text-x-generic", NULL);
    gtk_tree_view_insert_column_with_attributes(priv->tree_view, 0, NULL,
        pixbuf_renderer, "visible", COLUMN_IS_DOCUMENT, NULL);

    text_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(priv->tree_view, 1, NULL,
        text_renderer, "text", COLUMN_TITLE, NULL);

    gtk_tree_view_set_row_separator_func(priv->tree_view,
        quick_open_dialog_row_separator_func, NULL, NULL);

    priv->store = GTK_LIST_STORE(g_object_ref(gtk_builder_get_object(builder, "liststore")));
    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(priv->store),
        quick_open_dialog_tree_sort_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->store),
        GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

    priv->filter_model = GTK_TREE_MODEL_FILTER(
        gtk_tree_model_filter_new(GTK_TREE_MODEL(priv->store), NULL));
    gtk_tree_model_filter_set_visible_func(priv->filter_model,
        quick_open_dialog_tree_visible_func, self, NULL);

    priv->project_files_hash = g_hash_table_new_full(g_file_hash, (GEqualFunc)g_file_equal,
        g_object_unref, NULL);
    priv->document_files_hash = g_hash_table_new_full(g_file_hash, (GEqualFunc)g_file_equal,
        g_object_unref, NULL);

    quick_open_dialog_clear_project_files(self);
}

static void
quick_open_dialog_finalize (GObject* object)
{
    QuickOpenDialog* self = (QuickOpenDialog*)object;
    QuickOpenDialogPrivate* priv = self->priv;

    GSList* l;

    if (priv->filter_changed_timeout)
    {
        g_source_remove(priv->filter_changed_timeout);
        priv->filter_changed_timeout = 0;
    }

    g_hash_table_unref(priv->project_files_hash);
    g_hash_table_unref(priv->document_files_hash);

    for (l = priv->documents; l; l = l->next)
    {
        IAnjutaDocument* doc = l->data;

        g_signal_handlers_disconnect_by_func(doc, on_document_opened, self);
        g_signal_handlers_disconnect_by_func(doc, on_document_saved, self);
    }
    g_slist_free(priv->documents);

    g_clear_object(&priv->project_root);
    g_clear_object(&priv->store);


    G_OBJECT_CLASS (quick_open_dialog_parent_class)->finalize (object);
}

static void
quick_open_dialog_class_init (QuickOpenDialogClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = quick_open_dialog_finalize;

    g_type_class_add_private(klass, sizeof(QuickOpenDialogPrivate));
}
