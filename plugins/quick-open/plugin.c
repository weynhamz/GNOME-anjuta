/* -*- Mode: C; indent-spaces-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) 2012 Carl-Anton Ingmarsson <carlantoni@src.gnome.org>
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

#include <config.h>

#include "plugin.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include <signal.h>

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-quick-open.xml"

struct _QuickOpenPluginClass
{
    AnjutaPluginClass parent_class;
};

static void
on_dialog_response(GtkDialog* dialog, gint response_id, gpointer user_data)
{
    QuickOpenPlugin* self = user_data;

    gtk_widget_hide(GTK_WIDGET(dialog));

    if (response_id == GTK_RESPONSE_ACCEPT)
    {
        GObject* object;

        object = quick_open_dialog_get_selected_object(self->dialog);
        if (!object)
            return;

        if (IANJUTA_IS_DOCUMENT(object))
        {
            ianjuta_document_manager_set_current_document(self->docman,
                IANJUTA_DOCUMENT(object), NULL);
        }
        else if (G_IS_FILE(object))
        {
            IAnjutaFileLoader* loader;

            loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (self)->shell,
                IAnjutaFileLoader, NULL);
            g_return_if_fail (loader != NULL);

            ianjuta_file_loader_load (loader, G_FILE(object), FALSE, NULL);
        }

        g_object_unref(object);
    }
}

static void
on_quick_open_activate(GtkAction *action, QuickOpenPlugin *self)
{
    gtk_widget_show(GTK_WIDGET(self->dialog));
}

static void
project_node_foreach_func(AnjutaProjectNode *node, gpointer data)
{
    GSList** project_files = data;
    AnjutaProjectNodeType type;

    type = anjuta_project_node_get_node_type(node);

    if (type == ANJUTA_PROJECT_SOURCE)
    {
        GFile* file;

        file = anjuta_project_node_get_file(node);
        *project_files = g_slist_prepend(*project_files, file);
    }
}

static void
quick_open_plugin_load_project_files(QuickOpenPlugin* self, IAnjutaProject* project)
{
    AnjutaProjectNode* root;
    GSList* project_files = NULL;

    root = ianjuta_project_get_root (project, NULL);

    anjuta_project_node_foreach(root, G_POST_ORDER, project_node_foreach_func, &project_files);
    quick_open_dialog_add_project_files(self->dialog, project_files);
    g_slist_free(project_files);
}

static void
on_project_loaded(IAnjutaProjectManager *project_manager, GError *error, gpointer user_data)
{
    QuickOpenPlugin* self = user_data;

    IAnjutaProject* project;

    project = ianjuta_project_manager_get_current_project(project_manager, NULL);
    quick_open_plugin_load_project_files(self, project);
}

static void
quick_open_plugin_project_added(QuickOpenPlugin* self, IAnjutaProject* project)
{
    AnjutaProjectNode* root;
    GFile* project_root;

    root = ianjuta_project_get_root(project, NULL);
    project_root = anjuta_project_node_get_file(root);
    quick_open_dialog_set_project_root(self->dialog, project_root);

    if (ianjuta_project_is_loaded(project, NULL))
        quick_open_plugin_load_project_files(self, project);
}

static void
current_project_added(AnjutaPlugin *plugin, const char *name, const GValue *value, gpointer user_data)
{
    QuickOpenPlugin* self = ANJUTA_PLUGIN_QUICK_OPEN(plugin);

    IAnjutaProject* project = g_value_get_object(value);
    quick_open_plugin_project_added(self, project);
}

static void
current_project_removed(AnjutaPlugin *plugin, const char *name, gpointer user_data)
{
    QuickOpenPlugin* self = ANJUTA_PLUGIN_QUICK_OPEN(plugin);

    quick_open_dialog_set_project_root(self->dialog, NULL);
}

static void
quick_open_plugin_setup_project_handling(QuickOpenPlugin* self)
{
    IAnjutaProject* project;

    self->project_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN(self)->shell,
        IAnjutaProjectManager, NULL);
    g_return_if_fail(self->project_manager);

    g_object_add_weak_pointer(G_OBJECT(self->project_manager), (void**)&self->project_manager);

    /* Connect to project manager events. */
    self->project_watch_id = anjuta_plugin_add_watch(ANJUTA_PLUGIN(self),
        IANJUTA_PROJECT_MANAGER_CURRENT_PROJECT,
        current_project_added, current_project_removed, self);

    g_signal_connect(self->project_manager, "project-loaded",
        G_CALLBACK (on_project_loaded), self);

    project = ianjuta_project_manager_get_current_project (self->project_manager, NULL);
    if (project)
        quick_open_plugin_project_added(self, project);
}

static void
on_document_removed(IAnjutaDocumentManager* docman, IAnjutaDocument* doc, gpointer user_data)
{
    QuickOpenPlugin* self = user_data;

    quick_open_dialog_remove_document(self->dialog, doc);
}

static void
on_document_added(IAnjutaDocumentManager* docman, IAnjutaDocument* doc, gpointer user_data)
{
    QuickOpenPlugin* self = user_data;

    quick_open_dialog_add_document(self->dialog, doc);
}

static void
quick_open_plugin_setup_document_handling(QuickOpenPlugin* self)
{
    GList* documents, *l;

    self->docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(self)->shell,
        IAnjutaDocumentManager, NULL);
    g_return_if_fail(self->docman);

    g_object_add_weak_pointer(G_OBJECT(self->docman), (void**)&self->docman);

    documents = ianjuta_document_manager_get_doc_widgets(self->docman, NULL);
    for (l = documents; l; l = l->next)
    {
        IAnjutaDocument* doc = IANJUTA_DOCUMENT(l->data);
        quick_open_dialog_add_document(self->dialog, doc);
    }
    g_list_free(documents);

    g_signal_connect(self->docman, "document-added", G_CALLBACK(on_document_added), self);
    g_signal_connect(self->docman, "document-removed", G_CALLBACK(on_document_removed), self);
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_quick_open[] = {
    {
        "ActionQuickOpen",
        GTK_STOCK_OPEN,
        N_("Quick open"),
        "<Control><Alt>o",
        N_("Quickly open a file in the current project."),
        G_CALLBACK(on_quick_open_activate)
    }
};

/* AnjutaPlugin functions
 *---------------------------------------------------------------------------*/

static gboolean
quick_open_plugin_activate(AnjutaPlugin *plugin)
{
    QuickOpenPlugin *self = ANJUTA_PLUGIN_QUICK_OPEN(plugin);

    AnjutaUI* ui;

    DEBUG_PRINT("%s", "Quick Open Plugin: Activating plugin…");

    /* Add actions */
    ui = anjuta_shell_get_ui(plugin->shell, NULL);
    self->action_group = anjuta_ui_add_action_group_entries(ui,
        "ActionsQuickOpen", _("Quick open operations"),
        actions_quick_open, G_N_ELEMENTS(actions_quick_open),
        GETTEXT_PACKAGE, TRUE, self);

    self->uiid = anjuta_ui_merge(ui, UI_FILE);


    /* Create the dialog. */
    self->dialog = quick_open_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(self->dialog), GTK_WINDOW(plugin->shell));

    g_signal_connect(self->dialog, "delete-event",
        G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(self->dialog, "response",
        G_CALLBACK(on_dialog_response), self);

    quick_open_plugin_setup_project_handling(self);
    quick_open_plugin_setup_document_handling(self);


    return TRUE;
}

static gboolean
quick_open_plugin_deactivate(AnjutaPlugin *plugin)
{
    QuickOpenPlugin *self = ANJUTA_PLUGIN_QUICK_OPEN(plugin);

    AnjutaUI *ui;

    DEBUG_PRINT("%s", "Quick Open Plugin: Deactivating plugin…");

    ui = anjuta_shell_get_ui(plugin->shell, NULL);
    anjuta_ui_remove_action_group(ui, self->action_group);

    anjuta_ui_unmerge(ui, self->uiid);

    anjuta_plugin_remove_watch(plugin, self->project_watch_id, FALSE);

    /* Disconnect signals. */
    if (self->project_manager)
        g_signal_handlers_disconnect_by_func(self->project_manager, on_project_loaded, self);

    if (self->docman)
    {
        g_signal_handlers_disconnect_by_func(self->docman, on_document_added, self);
        g_signal_handlers_disconnect_by_func(self->docman, on_document_removed, self);
    }

    gtk_widget_destroy(GTK_WIDGET(self->dialog));

    return TRUE;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

static void
quick_open_plugin_instance_init(GObject *obj)
{
}

static void
quick_open_plugin_finalize(GObject *obj)
{
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void
quick_open_plugin_class_init(GObjectClass *klass)
{
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = quick_open_plugin_activate;
    plugin_class->deactivate = quick_open_plugin_deactivate;
    klass->finalize = quick_open_plugin_finalize;
}

/* AnjutaPlugin declaration
 *---------------------------------------------------------------------------*/

ANJUTA_PLUGIN_BEGIN(QuickOpenPlugin, quick_open_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN(QuickOpenPlugin, quick_open_plugin);
