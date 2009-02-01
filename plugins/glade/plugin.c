/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "plugin.h"
#include "anjuta-design-document.h"
#include "designer-associations.h"
#include <ctype.h>

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-glade.ui"
#define GLADE_PLUGIN_GLADE_UI_FILE PACKAGE_DATA_DIR"/glade/anjuta-glade.glade"
#define ICON_FILE "anjuta-glade-plugin-48.png"

static gpointer parent_class;

/* This is variable required from libdevhelp */
gchar *geometry = NULL;

typedef enum
{
	GLADE_DESIGNER_DEFAULT,
	GLADE_DESIGNER_DESIGN,
	GLADE_DESIGNER_PREVIEW
} GladeDesignerMode;

#define DIALOG_OPTIONS_COUNT 3
#define DO_SPEC_REGEXP 0
#define DO_WIDGET_NAME 1
#define DO_POSITION_TYPE 2

typedef struct
{
	gboolean updating;
	gulong associations_signal;
	DesignerAssociationsItem *current_item;
	gboolean fields_changed[DIALOG_OPTIONS_COUNT];
	GtkWidget *options_entries[DIALOG_OPTIONS_COUNT];
	GtkWidget *options_checkboxes[DIALOG_OPTIONS_COUNT-1];
	GtkWidget *options_button_save, *options_button_revert;
	GtkTreeView *treeview;
	GtkTable *options_table;
} AssociationsDialogData;

struct _GladePluginPriv
{
	gboolean destroying;
	gboolean deactivating;
	gint uiid;
	GtkActionGroup *action_group;
	GladeApp  *gpw;
	GtkWidget *inspector;
	GtkWidget *view_box;
	GtkWidget *projects_combo;
	gint editor_watch_id, project_watch_id, pm_current_uri_watch_id;
	GtkBuilder *xml;

	GtkWindow *dialog;
	AssociationsDialogData *dialog_data;

	GtkWidget *prefs;

	GFile *project_root;
	DesignerAssociations *associations;

	gboolean insert_handler_on_edit;
	gint default_handler_template;
	gchar *default_resource_target;
	gboolean auto_add_resource;

	GFile *last_editor;
	GFile *last_designer;
	gchar *last_signal_name;
	gchar *last_object_name;
	GType last_object_type;
	gchar *last_handler_name;
	gchar *last_toplevel_name;

	GtkWidget *new_container;
	GtkWidget *designer_layout_box;
	GtkWidget *designer_layout_box_child;
	gboolean separated_designer_layout; /* Don't set directly */
	GtkWidget *designer_toolbar;
	GtkToolItem *button_undo, *button_redo;

	GladeSignalEditor *last_gse;
#ifdef GLADE_SIGNAL_EDITOR_EXT
	GList *gse_list;
#endif
};

enum {
	NAME_COL,
	PROJECT_COL,
	N_COLUMNS
};

#define PLUGIN_GLADE_ERROR plugin_glade_error_quark()

typedef enum
{
	PLUGIN_GLADE_ERROR_GENERIC
} PluginGladeError;

GQuark
plugin_glade_error_quark (void);
GQuark
plugin_glade_error_quark (void)
{
  return g_quark_from_static_string ("plugin-glade-error-quark");
}

static void
designer_associations_raise_editor_priority (DesignerAssociations *self,
                                             GFile *editor,
                                             GFile *project_root)
{
	DesignerAssociationsItem *item;
	GList *node;
	GList *new_list = NULL;

	if (!editor)
		return;
	node = self->associations;
	while (node)
	{
		GList *old_node = NULL;

		item = DESIGNER_ASSOCIATIONS_ITEM (node->data);
		if (g_file_equal (item->editor, editor))
			old_node = node;

		node = node->next;

		if (old_node)
		{
			item = DESIGNER_ASSOCIATIONS_ITEM (old_node->data);
			self->associations = g_list_delete_link (self->associations, old_node);
			new_list = g_list_prepend (new_list, item);
		}
	}
	new_list = g_list_reverse (new_list);
	self->associations = g_list_concat (new_list, self->associations);
	designer_associations_notify_loaded (self);
}

static void
designer_associations_raise_designer_priority (DesignerAssociations *self,
                                               GFile *designer,
                                               const gchar *widget_name,
                                               GFile *project_root)
{
	DesignerAssociationsItem *item;
	GList *node;
	GList *new_list = NULL;

	if (!designer)
		return;
	node = self->associations;
	while (node)
	{
		GList *old_node = NULL;

		item = DESIGNER_ASSOCIATIONS_ITEM (node->data);
		if (g_file_equal (item->designer, designer))
			old_node = node;

		node = node->next;

		if (old_node)
		{
			item = DESIGNER_ASSOCIATIONS_ITEM (old_node->data);
			self->associations = g_list_delete_link (self->associations, old_node);
			new_list = g_list_prepend (new_list, item);
		}
	}
	new_list = g_list_reverse (new_list);
	self->associations = g_list_concat (new_list, self->associations);
	designer_associations_notify_loaded (self);
}

static void
doc_list_changed (AnjutaPlugin *anjuta_plugin);

static IAnjutaEditor *
find_editor_by_file (IAnjutaDocumentManager *docman, GFile *editor);

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
                            const GValue *value, gpointer data)
{
	GladePlugin* glade_plugin = ANJUTA_PLUGIN_GLADE(plugin);
	GObject *editor;
	GFile *file;

	editor = g_value_get_object (value);
	file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
	if (ANJUTA_IS_DESIGN_DOCUMENT(editor))
	{
		AnjutaDesignDocument *doc = ANJUTA_DESIGN_DOCUMENT(editor);
		GladeDesignView *design_view =
			anjuta_design_document_get_design_view (doc);
		GladeProject *project =
			glade_design_view_get_project (design_view);

		if (!doc->is_project_added)
		{
			glade_app_add_project (project);
			doc->is_project_added = TRUE;
		}
		glade_app_set_project (project);
	}
	else if (IANJUTA_IS_EDITOR (editor))
	{
		if (!(file && glade_plugin->priv->last_editor &&
		    g_file_equal (glade_plugin->priv->last_editor, file)))
		{
			if (glade_plugin->priv->last_editor)
				g_object_unref (glade_plugin->priv->last_editor);
			glade_plugin->priv->last_editor = g_object_ref (file);
			designer_associations_raise_editor_priority (glade_plugin->priv->associations,
			                                             glade_plugin->priv->last_editor,
			                                             glade_plugin->priv->project_root);
		}
	}
	if (file)
		g_object_unref (file);

	doc_list_changed (plugin);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
                              const char *name, gpointer data)
{
	doc_list_changed (plugin);
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
                              const GValue *value, gpointer user_data)
{
	const gchar *root_uri;
	GladePlugin *glade_plugin = ANJUTA_PLUGIN_GLADE (plugin);

	if (glade_plugin->priv->project_root)
		g_object_unref (glade_plugin->priv->project_root);
	root_uri = g_value_get_string (value);
	glade_plugin->priv->project_root = g_file_new_for_uri (root_uri);
	DEBUG_PRINT ("Added project root \"%s\"", root_uri);
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
                                gpointer user_data)
{
	GladePlugin *glade_plugin = ANJUTA_PLUGIN_GLADE (plugin);

	if (glade_plugin->priv->project_root)
		g_object_unref (glade_plugin->priv->project_root);
	glade_plugin->priv->project_root = NULL;
	DEBUG_PRINT ("Removed project root");
}

static void
value_added_pm_current_uri (AnjutaPlugin *plugin, const char *name,
                            const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	gchar *selected_id;
	IAnjutaProjectManager *projman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
		                            IAnjutaProjectManager, NULL);

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupGlade", "ActionSetDefaultTarget");
	selected_id = ianjuta_project_manager_get_selected_id (projman,
	                                                       IANJUTA_PROJECT_MANAGER_TARGET,
	                                                       NULL);
	gtk_action_set_sensitive (action, selected_id != NULL);
}

static void
value_removed_pm_current_uri (AnjutaPlugin *plugin,
	                      const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupGlade", "ActionSetDefaultTarget");
	gtk_action_set_sensitive (action, FALSE);
}

static void
update_current_project (GtkComboBox *projects_combo,
                        GladeProject* project)
{
	GtkTreeIter iter;
	GladeProject *cur_project;
	GtkTreeModel* model = gtk_combo_box_get_model (projects_combo);

	if (gtk_tree_model_get_iter_first (model, &iter))
		do
		{
			gtk_tree_model_get (model, &iter, PROJECT_COL, &cur_project, -1);
			if (project == cur_project)
			{
				gtk_combo_box_set_active_iter (projects_combo, &iter);
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
}

static void
glade_update_ui (GladeApp *app, GladePlugin *plugin)
{
	IAnjutaDocument* doc;
	IAnjutaDocumentManager* docman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
		                            IAnjutaDocumentManager, NULL);

	update_current_project (GTK_COMBO_BOX (plugin->priv->projects_combo), glade_app_get_project ());
	if (!plugin->priv->separated_designer_layout)
	{
		/* Emit IAnjutaDocument signal */
		doc = ianjuta_document_manager_get_current_document(docman, NULL);
		if (doc && ANJUTA_IS_DESIGN_DOCUMENT(doc))
		{
			gboolean dirty = ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(doc), NULL);
			g_signal_emit_by_name (G_OBJECT(doc), "update_ui");
			g_signal_emit_by_name (G_OBJECT(doc), "update-save-ui");
		}
	}

	{
		GtkAction *save_action, *undo_action, *redo_action;
		GladeProject *project;

		save_action = gtk_action_group_get_action (plugin->priv->action_group, "ActionGladeSave");
		undo_action = gtk_action_group_get_action (plugin->priv->action_group, "ActionGladeUndo");
		redo_action = gtk_action_group_get_action (plugin->priv->action_group, "ActionGladeRedo");
		project = glade_app_get_project ();
		if (project)
		{
			gtk_action_set_sensitive (save_action, glade_project_get_modified (project));
			gtk_action_set_sensitive (undo_action, glade_project_next_undo_item (project) != NULL);
			gtk_action_set_sensitive (redo_action, glade_project_next_redo_item (project) != NULL);
			gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (plugin->priv->button_undo),
			                               glade_project_undo_items (project));
			gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (plugin->priv->button_redo),
			                               glade_project_redo_items (project));
		}
	}
}

static void
on_api_help (GladeEditor* editor,
             const gchar* book,
             const gchar* page,
             const gchar* search,
             GladePlugin* plugin)
{
	gchar *book_comm = NULL, *page_comm = NULL;
	gchar *string;

	AnjutaPlugin* aplugin = ANJUTA_PLUGIN(plugin);
	AnjutaShell* shell = aplugin->shell;
	IAnjutaHelp* help;

	help = anjuta_shell_get_interface(shell, IAnjutaHelp, NULL);

	/* No API Help Plugin */
	if (help == NULL)
		return;

	if (book) book_comm = g_strdup_printf ("book:%s ", book);
	if (page) page_comm = g_strdup_printf ("page:%s ", page);

	string = g_strdup_printf ("%s%s%s",
		book_comm ? book_comm : "",
		page_comm ? page_comm : "",
		search ? search : "");

	ianjuta_help_search(help, string, NULL);

	g_free (string);
}

static void
glade_do_close (GladePlugin *plugin, GladeProject *project)
{
	glade_app_remove_project (project);
}

static gint
get_page_num_for_design_view (GladeDesignView *design_view, GladePlugin *plugin)
{
	if (!design_view)
		return -1;
	gint page_num =
		gtk_notebook_page_num (GTK_NOTEBOOK (plugin->priv->new_container),
		                       gtk_widget_get_parent (GTK_WIDGET (design_view)));
	return page_num;
}

static void
on_document_destroy (GladeDesignView* design_view, GladePlugin *plugin)
{
	GladeProject *project;
	GtkTreeModel *model;
	GtkTreeIter iter;

	DEBUG_PRINT ("%s", "Destroying Document");

	project = glade_design_view_get_project (design_view);

	if (plugin->priv->destroying)
	{
		return;
	}

	if (!project)
	{
		return;
	}

	/* Remove project from our list */
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			GladeProject *project_node;

			gtk_tree_model_get (model, &iter, PROJECT_COL, &project_node, -1);
			if (project == project_node)
			{
				gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	glade_do_close (plugin, project);

	if (!plugin->priv->deactivating && gtk_tree_model_iter_n_children (model, NULL) <= 0)
		anjuta_plugin_deactivate (ANJUTA_PLUGIN (plugin));
}

static void
on_shell_destroy (AnjutaShell* shell, GladePlugin *glade_plugin)
{
	DEBUG_PRINT ("Shell destroy");
	glade_plugin->priv->destroying = TRUE;
}

static AnjutaDesignDocument *
get_design_document_from_project (GladeProject *project)
{
	return ANJUTA_DESIGN_DOCUMENT (g_object_get_data (G_OBJECT (project), "design_document"));
}

static GladeProject *
get_project_from_design_document (AnjutaDesignDocument *design_document)
{
	GladeDesignView *design_view =
		anjuta_design_document_get_design_view (design_document);
	return glade_design_view_get_project (design_view);
}

static void
on_glade_project_changed (GtkComboBox *combo, GladePlugin *plugin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GFile *file;
	IAnjutaDocumentManager* docman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
		                           IAnjutaDocumentManager, NULL);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_combo_box_get_active_iter (combo, &iter))
	{
		GladeProject *project;
		AnjutaDesignDocument *design_document;
		GladeDesignView *design_view;
		gtk_tree_model_get (model, &iter, PROJECT_COL, &project, -1);
		glade_app_set_project (project);

		if (plugin->priv->associations)
		{
			design_document = get_design_document_from_project (project);
			design_view = anjuta_design_document_get_design_view (design_document);
			file = ianjuta_file_get_file (IANJUTA_FILE (design_document), NULL);

			if (plugin->priv->last_designer)
				g_object_unref (plugin->priv->last_designer);
			plugin->priv->last_designer = file;
			designer_associations_raise_designer_priority (plugin->priv->associations,
			                                               plugin->priv->last_designer,
			                                               NULL,
			                                               plugin->priv->project_root);
			doc_list_changed (ANJUTA_PLUGIN (plugin));
		}

		if (plugin->priv->separated_designer_layout)
		{
			/* Cannot avoid duplicated setting of current page while
			 * user selects a page, bacause we don't have a new current
			 * page number here
			 */
			gint page_num = get_page_num_for_design_view (design_view, plugin);
			if (page_num >= 0)
				gtk_notebook_set_current_page (GTK_NOTEBOOK (plugin->priv->new_container),
				                               page_num);
		}
		else
		{
			ianjuta_document_manager_set_current_document(docman, IANJUTA_DOCUMENT(design_document), NULL);
		}

#  if (GLADEUI_VERSION >= 330)
        glade_inspector_set_project (GLADE_INSPECTOR (plugin->priv->inspector), project);
#  endif

	}
}

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON ("anjuta-glade-plugin-48.png",
				   "glade-plugin-icon");
	END_REGISTER_ICON;
}

static void
switch_back_to_editor (GladePlugin *plugin)
{
	IAnjutaDocumentManager *docman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
		                            IAnjutaDocumentManager, NULL);
	IAnjutaDocument *doc = ianjuta_document_manager_get_current_document (docman, NULL);
	IAnjutaEditor *editor;
	if (ANJUTA_IS_DESIGN_DOCUMENT (doc) && plugin->priv->last_editor)
	{
		editor = find_editor_by_file (docman, plugin->priv->last_editor);
		if (editor)
			ianjuta_document_manager_set_current_document (docman, IANJUTA_DOCUMENT(editor), NULL);
	}
}

static gboolean
on_designer_notebook_enter_notify (GtkWidget *widget, GdkEvent *event, GladePlugin *plugin)
{
	switch_back_to_editor (plugin);
	return FALSE;
}

static void
on_designer_layout_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
                                guint page_num, GladePlugin *plugin)
{
	GladeProject *project;
	GladeDesignView *design_view;
	GtkBin *bin;
	AnjutaDesignDocument *doc;

	DEBUG_PRINT ("Page of design view switched to %d", page_num);
	g_return_if_fail (page);
	/* yeap that's a bit hacky */
	bin = GTK_BIN (*(GtkWidget**)page);
	g_return_if_fail (bin);
	design_view = GLADE_DESIGN_VIEW (gtk_bin_get_child (bin));
	if (!design_view)
	{
		DEBUG_PRINT ("The notebook tab doesn't contains a design_view");
		return;
	}
	project = glade_design_view_get_project (design_view);
	if (project)
	{
		doc = get_design_document_from_project (project);
		if (doc && doc->is_project_added)
			glade_app_set_project (project);
	}
}

static void
on_designer_layout_page_child_remove (GtkContainer *container, GtkWidget *widget,
                                    GladePlugin *plugin)
{
	gint page_num =
		gtk_notebook_page_num (GTK_NOTEBOOK (plugin->priv->new_container),
		                       GTK_WIDGET (container));
	if (page_num >= 0)
		gtk_notebook_remove_page (GTK_NOTEBOOK (plugin->priv->new_container), page_num);
	else
	{
		DEBUG_PRINT ("The page has already been removed");
	}
}

static void
designer_layout_add_doc (AnjutaDesignDocument *doc,
                         GtkContainer *container,
                         GladePlugin *plugin,
                         gboolean prepend)
{
	gint page_num;
	GladeDesignView *view = anjuta_design_document_get_design_view (doc);
	GladeProject *project = glade_design_view_get_project (view);
	if (!container)
	{
		container = GTK_CONTAINER (gtk_event_box_new());
		anjuta_design_document_set_design_view_parent (doc, container);
	}
	/* Remove the page when the design view removed */
	g_signal_connect (G_OBJECT (container), "remove",
	                  G_CALLBACK (on_designer_layout_page_child_remove), plugin);
	if (prepend)
	{
		page_num = gtk_notebook_prepend_page (GTK_NOTEBOOK (plugin->priv->new_container),
		                                      GTK_WIDGET (container),
		                                      gtk_label_new (glade_project_get_name (project)));
	}
	else
	{
		page_num = gtk_notebook_append_page (GTK_NOTEBOOK (plugin->priv->new_container),
		                                     GTK_WIDGET (container),
		                                     gtk_label_new (glade_project_get_name (project)));
	}
	DEBUG_PRINT ("Adding page #%d", page_num);
	gtk_widget_show_all (GTK_WIDGET (container));
}

static void
desinger_layout_add_all_docs (GladePlugin *plugin)
{
	GList *docwids, *node;
	IAnjutaDocumentManager *docman;

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				AnjutaDesignDocument *doc = ANJUTA_DESIGN_DOCUMENT (node->data);
				designer_layout_add_doc (doc, NULL, plugin, FALSE);
			}
		}
		g_list_free (docwids);
	}
}

static void
designer_layout_remove_doc (AnjutaDesignDocument *doc,
                            GladePlugin *plugin)
{
	/* The tab will be removed automatically while unparenting the design view */
	anjuta_design_document_set_design_view_parent (doc, GTK_CONTAINER (doc));
}

static void
desinger_layout_remove_all_docs (GladePlugin *plugin)
{
	GList *docwids, *node;
	IAnjutaDocumentManager *docman;

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				AnjutaDesignDocument *doc = ANJUTA_DESIGN_DOCUMENT (node->data);
				designer_layout_remove_doc (doc, plugin);
				anjuta_design_document_set_design_view_parent (doc, GTK_CONTAINER (doc));
			}
		}
		g_list_free (docwids);
	}
}

static void
update_separated_designer_layout (gboolean init, GladePlugin *plugin)
{
	if (init != plugin->priv->separated_designer_layout)
	{
		GladePluginPriv *priv = plugin->priv;
		plugin->priv->separated_designer_layout = init;
		if (init)
		{
			gint page_num;
			GladeProject *project;
			GladeDesignView *design_view;
			/*gtk_container_add (GTK_CONTAINER (priv->designer_layout_box),
							   priv->designer_layout_box_child);*/
			desinger_layout_add_all_docs (plugin);
			g_signal_connect (G_OBJECT (priv->new_container), "switch-page",
							  G_CALLBACK (on_designer_layout_switch_page), plugin);
			project = glade_app_get_project ();
			if (project)
			{
				design_view = glade_design_view_get_from_project (project);
				page_num = get_page_num_for_design_view (design_view, plugin);
				if (page_num >= 0)
					gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->new_container),
					                               page_num);
			}
			g_signal_connect (G_OBJECT (priv->designer_layout_box), "enter-notify-event",
							  G_CALLBACK(on_designer_notebook_enter_notify), plugin);
			gtk_widget_show_all (priv->designer_layout_box);
		}
		else /* uninitialization */
		{
			g_signal_handlers_disconnect_by_func (G_OBJECT (priv->new_container),
			                  G_CALLBACK (on_designer_layout_switch_page), plugin);
			g_signal_handlers_disconnect_by_func (G_OBJECT (priv->designer_layout_box),
			                     G_CALLBACK(on_designer_notebook_enter_notify), plugin);
			desinger_layout_remove_all_docs (plugin);
			/*gtk_container_remove (GTK_CONTAINER (priv->designer_layout_box),
							      priv->designer_layout_box_child);*/
		}
	}
}

#define GLADE_PREFERENCES_TAG "preferences"
#define GLADE_DEFAULT_HANDLER_TEMPLATE_INDEX_TAG "handler-template-index"
#define GLADE_INSERT_HANDLER_ON_EDIT_TAG "insert-signal-on-edit"
#define GLADE_DEFAULT_RESOURCE_TARGET "default-resource-target"
#define GLADE_AUTO_ADD_RESOURCE "auto-add-resource"
#define GLADE_SEPARATED_DESIGNER_LAYOUT "separated-designer-layout"

static void
glade_plugin_save_preferences (GladePlugin *plugin, xmlDocPtr xml_doc, xmlNodePtr node)
{
	xmlNodePtr child_node;
	gchar *value;

	child_node = xmlNewDocNode (xml_doc, NULL,
	                            BAD_CAST (GLADE_PREFERENCES_TAG), NULL);
	xmlAddChild (node, child_node);

	value = g_strdup_printf ("%d", plugin->priv->default_handler_template);
	xmlSetProp (child_node, BAD_CAST (GLADE_DEFAULT_HANDLER_TEMPLATE_INDEX_TAG),
	            BAD_CAST (value));
	g_free (value);

	value = g_strdup_printf ("%d", plugin->priv->insert_handler_on_edit);
	xmlSetProp (child_node, BAD_CAST (GLADE_INSERT_HANDLER_ON_EDIT_TAG),
	            BAD_CAST (value));
	g_free (value);

	value = g_strdup_printf ("%d", plugin->priv->auto_add_resource);
	xmlSetProp (child_node, BAD_CAST (GLADE_AUTO_ADD_RESOURCE),
	            BAD_CAST (value));
	g_free (value);

	value = g_strdup_printf ("%d", plugin->priv->separated_designer_layout);
	xmlSetProp (child_node, BAD_CAST (GLADE_SEPARATED_DESIGNER_LAYOUT),
	            BAD_CAST (value));
	g_free (value);

	xmlSetProp (child_node, BAD_CAST (GLADE_DEFAULT_RESOURCE_TARGET),
	            BAD_CAST (plugin->priv->default_resource_target));
}

static void
on_associations_changed (DesignerAssociations *self, DesignerAssociationsItem *item,
                         DesignerAssociationsAction action, GladePlugin *plugin);
static void
on_default_resource_target_changed (const gchar *selected, GladePlugin *plugin);

static gboolean
glade_plugin_do_save_associations (GladePlugin *plugin, GError **error)
{
	xmlDocPtr xml_doc;
	xmlNodePtr node;
	GFile *associations_file;
	gchar *associations_filename;

	if (!plugin->priv->associations)
	{
		g_set_error (error, PLUGIN_GLADE_ERROR,
		             PLUGIN_GLADE_ERROR_GENERIC,
		             _("No associoations initialized, nothing to save"));
		return FALSE;
	}
	if (!plugin->priv->project_root)
	{
		g_set_error (error, PLUGIN_GLADE_ERROR,
		             PLUGIN_GLADE_ERROR_GENERIC,
		             _("Couldn't save associations because project root isn't set"));
		return FALSE;
	}

	xml_doc = xmlNewDoc (BAD_CAST ("1.0"));
	node = xmlNewDocNode (xml_doc, NULL, BAD_CAST (DA_XML_TAG_ROOT), NULL);
	xmlDocSetRootElement (xml_doc, node);
	glade_plugin_save_preferences (plugin, xml_doc, node);
	designer_associations_save_to_xml (plugin->priv->associations,
	                                   xml_doc, node,
	                                   plugin->priv->project_root);
	xmlKeepBlanksDefault (0);

	associations_file =
		g_file_resolve_relative_path (plugin->priv->project_root,
		                             ".anjuta/associations");
	associations_filename = g_file_get_path (associations_file);
	DEBUG_PRINT ("Saving associations to file %s", associations_filename);
	if (xmlSaveFormatFile (associations_filename, xml_doc, 1) == -1)
	{
		g_set_error (error, PLUGIN_GLADE_ERROR,
		             PLUGIN_GLADE_ERROR_GENERIC,
		             _("Failed to save associations"));
		return FALSE;
	}
	g_object_unref (associations_file);
	g_free (associations_filename);
	xmlFreeDoc (xml_doc);

	return TRUE;
}

/* Save and destroy associations */
static void
glade_plugin_save_associations (GladePlugin *plugin)
{
	GError *error = NULL;

	if (plugin->priv->dialog)
	{
		gtk_widget_destroy (GTK_WIDGET (plugin->priv->dialog));
		plugin->priv->dialog = NULL;
	}

	glade_plugin_do_save_associations (plugin, &error);
	if (error)
	{
		DEBUG_PRINT ("Error while saving associations: %s", error->message);
		g_error_free (error);
	}

	if (plugin->priv->associations)
	{
		g_object_unref (plugin->priv->associations);
		plugin->priv->associations = NULL;
	}
}

static void
update_actions (GladePlugin *plugin)
{
	on_default_resource_target_changed (plugin->priv->default_resource_target, plugin);
}

static void
update_prefs_page (GladePlugin *plugin);

static gchar *
claim_xml_string (xmlChar *str)
{
	gchar *new_str = NULL;


	if (xmlStrcmp (str, BAD_CAST ("")) != 0)
		new_str = g_strdup ((gchar *) str);
	xmlFree (str);
	return new_str;
}

static void
glade_plugin_load_preferences (GladePlugin *plugin, xmlDocPtr xml_doc, xmlNodePtr node)
{
	xmlNodePtr child_node;
	gchar *value;

	child_node = search_child (node,
	                           GLADE_PREFERENCES_TAG);
	if (!child_node)
		return;

	value = (gchar*)xmlGetProp (child_node, BAD_CAST (GLADE_DEFAULT_HANDLER_TEMPLATE_INDEX_TAG));
	if (value)
	{
		plugin->priv->default_handler_template = g_ascii_strtoll (value, NULL, 0);
		xmlFree (value);
	}

	value = (gchar*)xmlGetProp (child_node, BAD_CAST (GLADE_INSERT_HANDLER_ON_EDIT_TAG));
	if (value)
	{
		plugin->priv->insert_handler_on_edit = g_ascii_strtoll (value, NULL, 0);
		xmlFree (value);
	}

	value = (gchar*)xmlGetProp (child_node, BAD_CAST (GLADE_AUTO_ADD_RESOURCE));
	if (value)
	{
		plugin->priv->auto_add_resource = g_ascii_strtoll (value, NULL, 0);
		xmlFree (value);
	}

	value = (gchar*)xmlGetProp (child_node, BAD_CAST (GLADE_SEPARATED_DESIGNER_LAYOUT));
	if (value)
	{
		update_separated_designer_layout (g_ascii_strtoll (value, NULL, 0), plugin);
		xmlFree (value);
	}

	plugin->priv->default_resource_target =
		claim_xml_string (xmlGetProp (child_node,
		                              BAD_CAST (GLADE_DEFAULT_RESOURCE_TARGET)));

	update_actions (plugin);
	update_prefs_page (plugin);
}

static void
glade_plugin_load_associations (GladePlugin *plugin)
{
	GError *error = NULL;
	xmlDocPtr xml_doc;
	xmlNodePtr node;
	GFile *associations_file;
	gchar *associations_filename;

	if (plugin->priv->associations)
	{
		DEBUG_PRINT ("Associations is already loaded");
		return;
	}
	plugin->priv->associations = designer_associations_new();
	g_signal_connect (plugin->priv->associations, "item-notify",
			  G_CALLBACK(on_associations_changed), plugin);
	//designer_associations_clear (plugin->priv->associations);

	if (plugin->priv->project_root)
	{
		associations_file =
			g_file_resolve_relative_path (plugin->priv->project_root,
			                             ".anjuta/associations");
		associations_filename = g_file_get_path (associations_file);
		xml_doc = xmlParseFile (associations_filename);
		DEBUG_PRINT ("Loading associations from %s", associations_filename);
		g_object_unref (associations_file);
		g_free (associations_filename);
		if (xml_doc)
		{
			node = xmlDocGetRootElement (xml_doc);
			if (node)
			{
				glade_plugin_load_preferences (plugin, xml_doc, node);
				designer_associations_load_from_xml (plugin->priv->associations, xml_doc, node,
				                                     plugin->priv->project_root, &error);
				if (error)
				{
					g_warning ("%s", error->message);
					g_error_free (error);
				}
			}
			else
				DEBUG_PRINT ("Couldn't load associations root node");
			xmlFreeDoc (xml_doc);
		}
		else
			DEBUG_PRINT ("Couldn't load associations");
	}
	else
		DEBUG_PRINT ("Couldn't load associations because project_root is not set");
}

static void
glade_plugin_save_doc_list (AnjutaShell *shell, AnjutaSession *session,
                            GladePlugin *plugin)
{
	GList *files, *docwids, *node;
	IAnjutaDocumentManager *docman;

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		files = anjuta_session_get_string_list (session, "File Loader", "Files");
		if (files)
			files = g_list_reverse (files);
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				GFile* file;
				file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (file != NULL)
				{
					DEBUG_PRINT ("Saving %s to session", g_file_get_uri (file));
					files = g_list_prepend (files, g_file_get_uri (file));
					/* uri is not freed here */
				}
				g_object_unref (file);
			}
		}
		g_list_free (docwids);
		if (files)
		{
			files = g_list_reverse (files);
			anjuta_session_set_string_list (session, "File Loader", "Files", files);
			g_list_foreach (files, (GFunc)g_free, NULL);
			g_list_free (files);
		}
	}
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
                 AnjutaSession *session, GladePlugin *plugin)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	DEBUG_PRINT ("Saving session");
	glade_plugin_save_doc_list (shell, session, plugin);
	glade_plugin_save_associations (plugin);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
                 AnjutaSession *session, GladePlugin *plugin)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	DEBUG_PRINT ("Loading session");
	glade_plugin_load_associations (plugin);
}

#if (GLADEUI_VERSION >= 330)
static void
inspector_item_activated_cb (GladeInspector     *inspector,
							 AnjutaPlugin       *plugin)
{
	GList *item = glade_inspector_get_selected_items (inspector);
	g_assert (GLADE_IS_WIDGET (item->data) && (item->next == NULL));

	/* switch to this widget in the workspace */
	glade_widget_show (GLADE_WIDGET (item->data));

	g_list_free (item);
}

static void
on_glade_resource_removed (GladeProject *project, const gchar *resource,
                           GladePlugin *plugin)
{
}

static void
on_glade_resource_added (GladeProject *project, const gchar *resource,
                         GladePlugin *plugin)
{
	gchar *str, *glade_basename, *resource_uri;
	GError *error = NULL;
	IAnjutaProjectManager *projman;

	if (!plugin->priv->auto_add_resource)
		return;
	if (!plugin->priv->default_resource_target)
	{
		DEBUG_PRINT ("No default data target");
		return;
	}
	projman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
	                                      IAnjutaProjectManager, NULL);
	glade_basename = g_path_get_dirname (glade_project_get_path (project));
	resource_uri = g_build_filename (glade_basename, resource, NULL);
	DEBUG_PRINT ("Adding resource \"%s\" to the target \"%s\"",
	              resource_uri, plugin->priv->default_resource_target);
	str = ianjuta_project_manager_add_source_quiet (projman,
	                                                plugin->priv->default_resource_target,
	                                                resource_uri,
	                                                &error);
	if (error)
	{
		g_warning ("Error while adding resource: %s", error->message);
		g_error_free (error);
	}

	g_free (str);
	g_free (resource_uri);
	g_free (glade_basename);
}

#endif




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * *   Signal handlers management  * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static GladeWidget *
find_root_gwidget (GladeWidget *widget)
{
	g_return_val_if_fail (widget, NULL);

	while (widget->parent)
		widget = widget->parent;

	return widget;
}

static gchar *
get_uri_from_ianjuta_file (IAnjutaFile *ifile)
{
	GFile *file = ianjuta_file_get_file (ifile, NULL);
	gchar *path = g_file_get_uri (file);
	g_object_unref (file);
	return path;
}

enum
{
	iptCurrent,
	iptBeforeEnd,
	iptAfterBegin,
	iptEOF,
	count_ipt
};

const gchar *ipt_names [count_ipt + 1] = {
	/* iptCurrent: */       "Current",
	/* iptBeforeEnd: */     "Before end",
	/* iptAfterBegin: */    "After begin",
	/* ipEOF: */            "End of file",
	NULL
};

/* position should be set to the begginning of special mark */
static void
insert_code_block_into_editor (IAnjutaEditor *editor, IAnjutaIterable *position,
                               GPtrArray *code_block, guint lines_count,
                               gchar *start_str, gchar *end_str)
{
	gint i;
	gchar *str;
	gchar **str_array;

	str_array = g_new0 (gchar *, lines_count + 2 + 1);
	str_array[0] = start_str;
	for (i = 1; i < lines_count+1; i++)
	{
		str_array[i] = g_ptr_array_index (code_block, i-1);
	}
	str_array[lines_count+1] = end_str;

	str = g_strjoinv (NULL, str_array);

	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (editor), NULL);
	ianjuta_editor_insert (editor, position, str, -1, NULL);
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (editor), NULL);

	g_free (str);
	g_free (str_array);
}

#define append_line(s) \
	{g_ptr_array_add (code_block, (gchar*)s); lines_count++;}

#define append_line_dup(s) \
	append_line (g_strdup (s))

/* returns chunk count */
static guint
format_handler_stub (GPtrArray *code_block, GType object_type,
                     const gchar *object_name, const gchar *signal_name,
                     const gchar *handler_name, gint *line_offset)
{
	GSignalQuery query;
	guint signal_id;
	guint lines_count = 0;
	gchar *buffer;

	signal_id = g_signal_lookup (signal_name, object_type);
	g_signal_query (signal_id, &query);

	if (query.signal_id)
	{
		gint i;

		if (query.return_type == 0)
		{
			append_line_dup ("void");
		}
		else
		{
			append_line_dup (g_type_name (query.return_type));
		}
		append_line_dup ("\n");

		buffer = g_strdup_printf ("%s (%s *self", handler_name,
		                           g_type_name (object_type));

		for (i=0; i<query.n_params; i++)
		{
			if (G_TYPE_IS_OBJECT (query.param_types[i]) ||
			    G_TYPE_FUNDAMENTAL (query.param_types[i]) == G_TYPE_BOXED)
			{
				buffer = g_strconcat (buffer,
				                      g_strdup_printf (", %s *arg%d",
				                                        g_type_name (query.param_types[i]), i),
				                                        NULL);
			}
			else
			{
				buffer = g_strconcat (buffer,
				                      g_strdup_printf (", %s arg%d",
				                                        g_type_name (query.param_types[i]), i),
				                                        NULL);
			}
		}

		buffer = g_strconcat (buffer, ", gpointer user_data)\n", NULL);
		append_line (buffer);

		append_line_dup (g_strdup("{\n\n}"));

		if (line_offset)
			(*line_offset) += 3;
	}

	return lines_count;
}

static gboolean
validate_position (IAnjutaIterable *position, gint position_type)
{
	return (position || position_type == iptEOF);
}

static void
do_insert_handler_stub_C (IAnjutaDocumentManager *docman, IAnjutaEditor *editor,
                          IAnjutaIterable *position, gint position_type,
                          GType object_type, const gchar *object_name,
                          const gchar *signal_name, const gchar *handler_name,
                          gboolean raise_editor, GError **error)
{
	GPtrArray *code_block;
	gchar *start_str = "";
	gchar *end_str = "";
	gint lineno;
	gint line_offset = 0;
	gint lines_count;

	if (!validate_position (position, position_type))
		return;

	switch (position_type)
	{
	case iptBeforeEnd:
		end_str = "\n\n";
		break;
	case iptAfterBegin:
		ianjuta_editor_get_line_from_position (editor, position, NULL);
		position = ianjuta_editor_get_line_end_position (editor, lineno, NULL);
		/* there's no need in break, honestly */
	case iptCurrent:
		start_str = "\n\n";
		line_offset += 2;
		break;
	case iptEOF:
		position = ianjuta_editor_get_end_position (editor, NULL);
		start_str = "\n";
		end_str = "\n";
		line_offset += 1;
		break;
	}

	g_return_if_fail (position);
	lineno = ianjuta_editor_get_line_from_position (editor,
							position,
							NULL);
	code_block = g_ptr_array_new ();
	lines_count = format_handler_stub (code_block, object_type, object_name,
	                                   signal_name, handler_name, &line_offset);
	insert_code_block_into_editor (editor, position, code_block, lines_count,
	                               start_str, end_str);
	if (lines_count > 0 && raise_editor)
	{
		ianjuta_document_manager_set_current_document (docman,
		                                               IANJUTA_DOCUMENT (editor),
		                                               NULL);
		ianjuta_document_grab_focus (IANJUTA_DOCUMENT (editor), NULL);
		ianjuta_editor_goto_position (editor,
		                              ianjuta_editor_get_line_end_position (editor,
		                                                                    lineno + line_offset,
		                                                                    NULL),
		                              NULL);
	}
	if (lines_count <= 0)
		g_set_error (error,
	                 PLUGIN_GLADE_ERROR,
	                 PLUGIN_GLADE_ERROR_GENERIC,
	                 _("couldn't introspect the signal"));

	{
		gint i;
		for (i = 0; i < lines_count; i++)
		{
			g_free (g_ptr_array_index (code_block, i));
		}
		g_ptr_array_free (code_block, TRUE);
	}
}

static void
do_insert_handler_stub_Python (IAnjutaDocumentManager *docman, IAnjutaEditor *editor,
                               IAnjutaIterable *position, gint position_type,
                               GType object_type, const gchar *object_name,
                               const gchar *signal_name, const gchar *handler_name,
                               gboolean raise_editor, GError **error)
{
	g_set_error (error,
	             PLUGIN_GLADE_ERROR,
	             PLUGIN_GLADE_ERROR_GENERIC,
	             _("Python language isn't supported yet"));
}

static void
do_insert_handler_stub_Vala (IAnjutaDocumentManager *docman, IAnjutaEditor *editor,
                             IAnjutaIterable *position, gint position_type,
                             GType object_type, const gchar *object_name,
                             const gchar *signal_name, const gchar *handler_name,
                             gboolean raise_editor, GError **error)
{
	g_set_error (error,
	             PLUGIN_GLADE_ERROR,
	             PLUGIN_GLADE_ERROR_GENERIC,
	             _("Vala language isn't supported yet"));
}

typedef struct _LanguageSyntax
{
	gchar *begin_regexp;
	gchar *end_regexp;
} LanguageSyntax;

const LanguageSyntax _default_syntax_C = {
	"^\\s*\\/\\*+\\s*(\\S*)\\s*callbacks",
	"^\\s*\\/\\*+\\s*end\\s*of\\s*(\\S*)\\s*callbacks"
};

const LanguageSyntax _default_syntax_Python = {
	"^\\s*#\\s*(\\S*)\\s*callbacks",
	"^\\s*#\\s*end\\s*of\\s*(\\S*)\\s*callbacks"
};

typedef enum
{
	LANGUAGE_NONE,
	LANGUAGE_C,
	LANGUAGE_PYTHON,
	LANGUAGE_VALA
} LanguageId;

LanguageSyntax const *default_syntax_C = &_default_syntax_C;
LanguageSyntax const *default_syntax_Python = &_default_syntax_Python;
LanguageSyntax const *default_syntax_Vala = &_default_syntax_C;

static IAnjutaIterable *
find_auto_position (GladePlugin *plugin, IAnjutaEditor *editor,
                    const gchar *regexp_str)
{
	IAnjutaIterable *position = NULL;
	IAnjutaIterable *fallback_pos = NULL;
	GRegex *regex;
	GError *error = NULL;
	GMatchInfo *match_info;
	gchar *all_text;

	g_return_val_if_fail (editor, NULL);
	g_return_val_if_fail (regexp_str, NULL);

	regex = g_regex_new (regexp_str,
	                     G_REGEX_CASELESS | G_REGEX_MULTILINE,
	                     0, &error);
	if (error)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}
	if (!regex)
		return NULL;

	all_text = ianjuta_editor_get_text_all (editor, NULL);
	g_regex_match (regex, all_text, 0, &match_info);
	while (g_match_info_matches (match_info))
	{
		gchar *word = g_match_info_fetch (match_info, 1);

		if (plugin->priv->last_toplevel_name && word &&
		    g_str_equal (plugin->priv->last_toplevel_name, word))
		{
			gint start_pos, end_pos;
			g_match_info_fetch_pos (match_info, 0, &start_pos, &end_pos);

			/* use "+1" because we are at the end of previous line */
			position = ianjuta_editor_get_position_from_offset (editor, start_pos + 1, NULL);
			g_free (word);
			break;
		}

		if (word && g_str_equal (word, ""))
		{
			gint start_pos, end_pos;
			g_match_info_fetch_pos (match_info, 0, &start_pos, &end_pos);
			fallback_pos = ianjuta_editor_get_position_from_offset (editor, start_pos + 1, NULL);
		}
		g_free (word);
		g_match_info_next (match_info, NULL);
	}

	g_match_info_free (match_info);
	g_regex_unref (regex);
	g_free (all_text);

	return position ? position : fallback_pos;
}

static IAnjutaEditor *
find_editor_by_file (IAnjutaDocumentManager* docman, GFile *editor)
{
	GList *docwids, *node;
	IAnjutaEditor *retval = NULL;

	g_return_val_if_fail (G_IS_FILE (editor), NULL);

	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (IANJUTA_IS_EDITOR (node->data))
			{
				GFile *file;

				file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (g_file_equal (editor, file))
					retval = IANJUTA_EDITOR (node->data);
				g_object_unref (file);
				if (retval)
					break;
			}
		}
		g_list_free (docwids);
	}

	return retval;
}

static AnjutaDesignDocument *
find_designer_by_file (IAnjutaDocumentManager* docman, GFile *designer)
{
	GList *docwids, *node;
	AnjutaDesignDocument *retval = NULL;

	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				GFile *file;

				file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (g_file_equal (designer, file))
					retval = ANJUTA_DESIGN_DOCUMENT (node->data);
				g_object_unref (file);
				if (retval)
					break;
			}
		}
		g_list_free (docwids);
	}

	return retval;
}

static GList *
designer_associations_match_editor (DesignerAssociations *self,
                                    GFile *editor,
                                    GFile *project_root)
{
	GList *node;
	GList *result = NULL;
	DesignerAssociationsItem *item;

	node = self->associations;
	while (node)
	{
		item = node->data;

		if (g_file_equal (item->editor, editor))
			result = g_list_prepend (result, item);

		node = node->next;
	}

	result = g_list_reverse (result);
	return result;
}

static GList *
designer_associations_match_designer (DesignerAssociations *self,
                                      GFile *designer,
                                      const gchar *widget_name,
                                      GFile *project_root)
{
	GList *node;
	GList *result = NULL;
	GList *fallback_result = NULL;
	DesignerAssociationsItem *item;

	node = self->associations;
	while (node)
	{
		item = node->data;

		if (g_file_equal (item->designer, designer))
		{
			if (widget_name)
			{
				if (item->widget_name)
				{
					if (widget_name && item->widget_name &&
					    g_str_equal (widget_name, item->widget_name))
					{
						result = g_list_prepend (result, item);
					}
				}
				else
					fallback_result = g_list_prepend (fallback_result, item);
			}
			else
			{ /* if (!widget_name) */
				if (item->widget_name)
					fallback_result = g_list_prepend (fallback_result, item);
				else
					result = g_list_prepend (result, item);
			}
		}
		node = node->next;
	}

	result = g_list_reverse (result);
	fallback_result = g_list_reverse (fallback_result);
	result = g_list_concat (result, fallback_result);
	return result;
}

#ifdef DEBUG
static void
dump_items_list (GList *items)
{
	GList *node;
	DesignerAssociationsItem *item;

	DEBUG_PRINT ("======================================================");
	DEBUG_PRINT ("Dumping items list");
	for (node = items; node; node = node->next)
	{
		item = node->data;
		DEBUG_PRINT ("Designer: \"%s\", %d refs;\nWidget: %s\nEditor: \"%s\", %d refs"
		             "---------------------------------------------------------------------",
		              g_file_get_path (item->designer), G_OBJECT(item->designer)->ref_count,
		              item->widget_name,
		              g_file_get_path (item->editor), G_OBJECT(item->editor)->ref_count);
	}
	DEBUG_PRINT ("======================================================");
}
#endif

static GList *
get_associated_items_for_designer (IAnjutaDocument *doc,
                                   const gchar *widget_name,
                                   GladePlugin* plugin)
{
	GList *list = NULL;
	IAnjutaFile *file;
	GFile *gfile;

	g_return_val_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (doc), NULL);

	file = IANJUTA_FILE (doc);
	g_return_val_if_fail (file, NULL);
	gfile = ianjuta_file_get_file (file, NULL);
	g_return_val_if_fail (gfile, NULL);

	list = designer_associations_match_designer (plugin->priv->associations,
	                                             gfile,
	                                             widget_name,
	                                             plugin->priv->project_root);
	g_object_unref (gfile);
#ifdef DEBUG
	dump_items_list (list);
#endif
	return list;
}

static GList *
get_associated_items_for_editor (IAnjutaDocument *doc,
                                 GladePlugin* plugin)
{
	GList *list = NULL;
	IAnjutaFile *file;
	GFile *gfile;

	g_return_val_if_fail (IANJUTA_IS_EDITOR (doc), NULL);
	file = IANJUTA_FILE (doc);
	g_return_val_if_fail (file, NULL);
	gfile = ianjuta_file_get_file (file, NULL);
	g_return_val_if_fail (gfile, NULL);

	list = designer_associations_match_editor (plugin->priv->associations,
	                                           gfile,
	                                           plugin->priv->project_root);
	g_object_unref (gfile);
#ifdef DEBUG
	dump_items_list (list);
#endif
	return list;
}

static IAnjutaEditor *
find_valid_editor (GList *items, GladePlugin *plugin,
                   DesignerAssociationsItem **matching_item)
{
	GList *node;
	IAnjutaEditor *retval = NULL;
	DesignerAssociationsItem *item;
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);
	IAnjutaDocumentManager *docman =
		IANJUTA_DOCUMENT_MANAGER (anjuta_shell_get_object (anjuta_plugin->shell,
		                         "IAnjutaDocumentManager", NULL));
	g_return_val_if_fail (docman, NULL);

	for (node = items; node; node = node->next)
	{
		item = node->data;
		retval = find_editor_by_file (docman, item->editor);
		if (retval)
		{
			if (matching_item)
				*matching_item = item;
			break;
		}
	}

	return retval;
}

static AnjutaDesignDocument *
find_valid_designer (GList *items, GladePlugin *plugin,
                     DesignerAssociationsItem **matching_item)
{
	GList *node;
	AnjutaDesignDocument *retval = NULL;
	DesignerAssociationsItem *item;
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);
	IAnjutaDocumentManager *docman =
		IANJUTA_DOCUMENT_MANAGER (anjuta_shell_get_object (anjuta_plugin->shell,
		                         "IAnjutaDocumentManager", NULL));
	g_return_val_if_fail (docman, NULL);

	for (node = items; node; node = node->next)
	{
		item = node->data;
		retval = find_designer_by_file (docman, item->designer);
		if (retval)
		{
			if (matching_item)
				*matching_item = item;
			break;
		}
	}

	return retval;
}

static IAnjutaEditor *
get_associated_editor_for_doc (IAnjutaDocument *doc, const gchar *widget_name,
                               GladePlugin* plugin, IAnjutaDocumentManager *docman,
                               DesignerAssociationsItem **matching_item)
{
	IAnjutaEditor *editor = NULL;

	if (IANJUTA_IS_EDITOR(doc))
	{
		GList *list = get_associated_items_for_editor (doc, plugin);
		if (find_valid_designer (list, plugin, matching_item))
			editor = IANJUTA_EDITOR (doc);
		g_list_free (list);
	}
	else if (ANJUTA_IS_DESIGN_DOCUMENT(doc))
	{
		GList *list = get_associated_items_for_designer (doc, widget_name, plugin);
		editor = find_valid_editor (list, plugin, matching_item);
		g_list_free (list);
	}

	return editor;
}

static gchar *
gse_get_signal_name (GtkTreeModel *model, GtkTreeIter *iter)
{
	gchar *signal_name;
	gtk_tree_model_get (model, iter,
	                    GSE_COLUMN_SIGNAL, &signal_name,
	                    -1);

	if (signal_name == NULL)
	{
		GtkTreeIter iter_signal;

		if (!gtk_tree_model_iter_parent (model, &iter_signal, iter))
			g_assert (FALSE);

		gtk_tree_model_get (model, &iter_signal, GSE_COLUMN_SIGNAL, &signal_name, -1);
		g_assert (signal_name != NULL);
	}

	return signal_name;
}

static IAnjutaIterable *
get_auto_position (DesignerAssociationsItem *matching_item,
                   IAnjutaEditor *editor, GladePlugin *plugin,
                   gint *result_position_type, const LanguageSyntax *syntax)
{
	IAnjutaIterable *position = NULL;
	gint position_type = 0;
	gchar *str = NULL;

	str = designer_associations_item_get_option (matching_item, "special_regexp");
	if (!str)
	{
		position_type =
			designer_associations_item_get_option_as_int (matching_item,
			                                             "position_type",
			                                              ipt_names);
		switch (position_type)
		{
		case iptCurrent:
			position = ianjuta_editor_get_position (editor, NULL);
		case iptAfterBegin:
			position = find_auto_position (plugin, editor, syntax->begin_regexp);
			break;
		case iptBeforeEnd:
			position = find_auto_position (plugin, editor, syntax->end_regexp);
			break;
		/* iptEOF: nothing for now */
		}
	}
	else
	{
		position = find_auto_position (plugin, editor, str);
		g_free (str);
	}

	if (result_position_type)
		*result_position_type = position_type;
	return position;
}

static void
forget_last_signal (GladePlugin *plugin)
{
	plugin->priv->last_object_type = 0;
	g_free (plugin->priv->last_object_name);
	        plugin->priv->last_object_name = NULL;
	g_free (plugin->priv->last_signal_name);
	        plugin->priv->last_signal_name = NULL;
	g_free (plugin->priv->last_handler_name);
	        plugin->priv->last_handler_name = NULL;
	g_free (plugin->priv->last_toplevel_name);
	        plugin->priv->last_toplevel_name = NULL;
}

static gboolean
validate_last_signal (GladePlugin *plugin)
{
	gboolean validated = (plugin->priv->last_object_type != 0 &&
	                      plugin->priv->last_object_name != NULL &&
	                      plugin->priv->last_signal_name != NULL &&
	                      plugin->priv->last_handler_name != NULL);
	if (!validated)
		forget_last_signal (plugin);

	return validated;
}

static gboolean
glade_plugin_fetch_last_signal (GladePlugin *plugin)
{
	GtkTreeView *tree_view;
	GtkTreeIter iter;
	GtkTreeModel  *model;
	GtkTreeSelection *selection;
	gchar *signal_handler = NULL;
	gchar *signal_name;
	gboolean slot;
	GladeWidget *gwidget = GLADE_WIDGET (plugin->priv->last_gse->widget);

	if (gwidget)
	{
		tree_view = GTK_TREE_VIEW (plugin->priv->last_gse->signals_list);
		selection = gtk_tree_view_get_selection (tree_view);
		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			gtk_tree_model_get (model, &iter, GSE_COLUMN_HANDLER, &signal_handler,
			                                  GSE_COLUMN_SLOT, &slot, -1);
			/*if (plugin->priv->last_signal_editor->is_void_handler (signal_handler))*/
			if (slot)
			{
				g_free (signal_handler);
				signal_handler = NULL;
				return FALSE;
			}

			signal_name = gse_get_signal_name (model, &iter);

			plugin->priv->last_object_name = g_strdup (gwidget->name);
			plugin->priv->last_signal_name = signal_name;
			plugin->priv->last_object_type = GTK_OBJECT_TYPE (gwidget->object);
			plugin->priv->last_handler_name = signal_handler;
			plugin->priv->last_toplevel_name = g_strdup (find_root_gwidget (gwidget)->name);

			return TRUE;
		}
	}

	return validate_last_signal (plugin);
}

static LanguageId
language_name_to_id (const gchar *lang_name)
{
	LanguageId lang_id = LANGUAGE_NONE;

	if (g_str_equal (lang_name, "C") || g_str_equal (lang_name, "C++"))
		lang_id = LANGUAGE_C;
	else if (g_str_equal (lang_name, "Python"))
		lang_id = LANGUAGE_PYTHON;
	else if (g_str_equal (lang_name, "Vala"))
		lang_id = LANGUAGE_VALA;

	return lang_id;
}

static void
insert_handler_stub_auto (IAnjutaDocument *doc, GladePlugin *plugin,
                          gboolean raise_editor, GError **error)
{
	IAnjutaEditor *editor = NULL;
	const gchar *lang_name;
	DesignerAssociationsItem *matching_item = NULL;
	LanguageId lang_id;
	const LanguageSyntax *syntax;
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);
	IAnjutaDocumentManager* docman;
	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (anjuta_plugin)->shell,
		                            IAnjutaLanguage, NULL);

	docman = anjuta_shell_get_interface (anjuta_plugin->shell,
	                                     IAnjutaDocumentManager, NULL);
	g_return_if_fail (lang_manager);
	g_return_if_fail (validate_last_signal (plugin));
	if (doc == NULL)
	{
		if (plugin->priv->separated_designer_layout)
			doc = IANJUTA_DOCUMENT (get_design_document_from_project (glade_app_get_project()));
		else
			doc = ianjuta_document_manager_get_current_document (docman, NULL);
	}

	DEBUG_PRINT ("Inserting handler using autoposition");
	editor = get_associated_editor_for_doc (doc, plugin->priv->last_toplevel_name,
	                                        plugin, docman, &matching_item);

	if (!editor)
	{
		g_set_error (error,
		             PLUGIN_GLADE_ERROR,
		             PLUGIN_GLADE_ERROR_GENERIC,
		             _("there's no associated editor for the designer"));
		return;
	}
	lang_name = ianjuta_language_get_name_from_editor (lang_manager,
	                                                   IANJUTA_EDITOR_LANGUAGE (editor),
	                                                   NULL);
	DEBUG_PRINT ("Language of %s is %s", g_file_get_path (ianjuta_file_get_file (IANJUTA_FILE(editor), NULL)),
	                                     lang_name);

	lang_id = language_name_to_id (lang_name);
	switch (lang_id)
	{
	case LANGUAGE_NONE:
		{
			gchar *uri = get_uri_from_ianjuta_file (IANJUTA_FILE (editor));
			g_set_error (error,
			             PLUGIN_GLADE_ERROR,
			             PLUGIN_GLADE_ERROR_GENERIC,
			             _("unknown language of the editor \"%s\""), uri);
			g_free (uri);
			return;
		}
	case LANGUAGE_C:
		syntax = default_syntax_C;
		break;
	case LANGUAGE_PYTHON:
		syntax = default_syntax_Python;
		break;
	case LANGUAGE_VALA:
		syntax = default_syntax_Vala;
		break;
	}
	IAnjutaIterable *position = NULL;
	gint position_type = 0;

	position = get_auto_position (matching_item, editor, plugin,
	                              &position_type, default_syntax_C);
	if (!position && position_type != iptEOF)
		position_type = iptEOF;

	switch (lang_id)
	{
	case LANGUAGE_NONE:
		g_assert_not_reached();
		return;
	case LANGUAGE_C:
		do_insert_handler_stub_C (docman, editor, position, position_type,
		                          plugin->priv->last_object_type,
		                          plugin->priv->last_object_name,
		                          plugin->priv->last_signal_name,
		                          plugin->priv->last_handler_name,
		                          raise_editor, error);
		break;
	case LANGUAGE_PYTHON:
		do_insert_handler_stub_Python (docman, editor, position, position_type,
		                               plugin->priv->last_object_type,
		                               plugin->priv->last_object_name,
		                               plugin->priv->last_signal_name,
		                               plugin->priv->last_handler_name,
		                               raise_editor, error);
		break;
	case LANGUAGE_VALA:
		do_insert_handler_stub_Vala (docman, editor, position, position_type,
		                             plugin->priv->last_object_type,
		                             plugin->priv->last_object_name,
		                             plugin->priv->last_signal_name,
		                             plugin->priv->last_handler_name,
		                             raise_editor, error);
		break;
	}

	//forget_last_signal (plugin);
}

static void
on_insert_handler_stub_auto (GtkAction* action, GladePlugin* plugin)
{
	GError *error = NULL;

	if (glade_plugin_fetch_last_signal (plugin))
	{
		insert_handler_stub_auto (NULL, plugin, TRUE, &error);
		if (error)
		{
			anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
										_("Error while adding a new handler stub: %s"),
										error->message);
			g_error_free (error);
		}
	}
	else
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                            _("Couldn't find a signal information"));
}

static void
insert_handler_stub_manual (GladePlugin* plugin, gboolean raise_editor)
{
	IAnjutaEditor *editor;
	const gchar *lang_name;
	IAnjutaDocument *doc;
	GError *error = NULL;
	IAnjutaIterable *position;
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);
	IAnjutaDocumentManager* docman =
		anjuta_shell_get_interface (anjuta_plugin->shell,
		                            IAnjutaDocumentManager, NULL);
	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (anjuta_plugin)->shell,
		                            IAnjutaLanguage, NULL);
	g_return_if_fail (lang_manager);
	g_return_if_fail (validate_last_signal (plugin));

	DEBUG_PRINT ("Inserting handler manually");
	doc = ianjuta_document_manager_get_current_document (docman, NULL);

	if (!IANJUTA_IS_EDITOR (doc))
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                            _("Error while adding a new handler stub: %"), _("no current editor"));
		return;
	}
	editor = IANJUTA_EDITOR (doc);
	lang_name = ianjuta_language_get_name_from_editor (lang_manager,
	                                                   IANJUTA_EDITOR_LANGUAGE (editor),
	                                                   NULL);
	DEBUG_PRINT ("Language is %s", lang_name);
	position = ianjuta_editor_get_position (editor, NULL);
	/* Scintilla plugin treats C files as C++ */

	switch (language_name_to_id (lang_name))
	{
	case LANGUAGE_NONE:
	case LANGUAGE_C:
		do_insert_handler_stub_C (docman, editor, position, iptCurrent,
		                          plugin->priv->last_object_type,
		                          plugin->priv->last_object_name,
		                          plugin->priv->last_signal_name,
		                          plugin->priv->last_handler_name,
		                          TRUE, &error);
		break;
	case LANGUAGE_PYTHON:
		do_insert_handler_stub_Python (docman, editor, position, iptCurrent,
		                               plugin->priv->last_object_type,
		                               plugin->priv->last_object_name,
		                               plugin->priv->last_signal_name,
		                               plugin->priv->last_handler_name,
		                               TRUE, &error);
		break;
	case LANGUAGE_VALA:
		do_insert_handler_stub_Vala (docman, editor, position, iptCurrent,
		                             plugin->priv->last_object_type,
		                             plugin->priv->last_object_name,
		                             plugin->priv->last_signal_name,
		                             plugin->priv->last_handler_name,
		                             TRUE, &error);
		break;
	}

	if (error)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
									_("Error while adding a new handler stub: %s"), error->message);
		g_error_free (error);
	}

	/*forget_last_signal (plugin);*/
}

static void
on_insert_handler_stub_manual (GtkAction* action, GladePlugin* plugin)
{
	if (glade_plugin_fetch_last_signal (plugin))
		insert_handler_stub_manual (plugin, TRUE);
	else
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                            _("Couldn't find a signal information"));
}

/* return true only if symbol definitely exists */
static gboolean
goto_symbol (const gchar *handler_name, GladePlugin* plugin,
             gboolean do_goto)
{
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);
	IAnjutaSymbolManager *manager;
	IAnjutaIterable *symbol_iter;
	GError *error = NULL;
	gboolean retval = FALSE;
	IAnjutaDocumentManager *docman;

/* FIXME: symbol database should have been updated here or recent handlers will not be found */
	manager = anjuta_shell_get_interface (anjuta_plugin->shell,
	                                      IAnjutaSymbolManager, NULL);
	docman = anjuta_shell_get_interface (anjuta_plugin->shell,
	                                     IAnjutaDocumentManager, NULL);
	DEBUG_PRINT ("Looking for symbol %s", handler_name);
	if (manager)
	{
		symbol_iter =
			ianjuta_symbol_manager_search (manager,
			                               IANJUTA_SYMBOL_TYPE_FUNCTION, TRUE,
			                               do_goto ? IANJUTA_SYMBOL_FIELD_FILE_PATH :
			                               IANJUTA_SYMBOL_FIELD_SIMPLE,
			                               handler_name, FALSE, FALSE, FALSE,
			                               1, -1, &error);
		if (error)
		{
			g_warning ("%s", error->message);
			g_error_free (error);
		}
		if (symbol_iter)
		{
			if (ianjuta_iterable_get_length (symbol_iter, NULL) > 0)
			{
				GFile *file;
				guint line;
				IAnjutaEditor *editor;
				IAnjutaSymbol *symbol;

				symbol = IANJUTA_SYMBOL (symbol_iter);
				if (symbol)
				{
					retval = TRUE;
					DEBUG_PRINT ("Symbol found");
				}
				if (do_goto)
				{
					file = ianjuta_symbol_get_file (symbol, NULL);
					line = ianjuta_symbol_get_line (symbol, NULL);
					if (file)
					{
						DEBUG_PRINT ("Going to symbol at %s#%d", g_file_get_uri (file), line);
						editor = ianjuta_document_manager_goto_file_line (docman, file, line, NULL);
						g_object_unref (file);

						if (editor)
						{
							ianjuta_document_manager_set_current_document (docman,
							                                               IANJUTA_DOCUMENT (editor),
							                                               NULL);
							ianjuta_document_grab_focus (IANJUTA_DOCUMENT (editor), NULL);
						}
					}
				}
			}
			g_object_unref (G_OBJECT (symbol_iter));
		}
	}

	return retval;
}

#ifdef GLADE_SIGNAL_EDITOR_EXT
static gboolean
on_handler_editing_done (GladeSignalEditor *self, gchar *signal_name,
                         gchar *old_handler, gchar *new_handler, GtkTreeIter *iter,
                         GladePlugin *plugin)
{
	GError *error = NULL;
	g_return_val_if_fail (plugin, FALSE);
	GladeWidget *gwidget = self->widget;

	DEBUG_PRINT ("Handler for signal %s changed from %s to %s in the widget %s",
	              signal_name, old_handler, new_handler, gwidget->name);
	if (old_handler == NULL && goto_symbol (new_handler, plugin, FALSE) == FALSE)
	{
		forget_last_signal (plugin);

		plugin->priv->last_object_name = g_strdup (gwidget->name);
		plugin->priv->last_signal_name = g_strdup (signal_name);
		plugin->priv->last_object_type = GTK_OBJECT_TYPE (gwidget->object);
		plugin->priv->last_handler_name = g_strdup (new_handler);
		plugin->priv->last_toplevel_name = g_strdup (find_root_gwidget (gwidget)->name);

		if (plugin->priv->insert_handler_on_edit)
		{
			IAnjutaDocument *doc;

			g_return_val_if_fail (gwidget->project, FALSE);

			doc = IANJUTA_DOCUMENT (get_design_document_from_project (gwidget->project));
			g_return_val_if_fail (doc, FALSE);
			insert_handler_stub_auto (doc, plugin, TRUE, &error);
			if (error)
			{
				gchar *error_message =
					g_strdup_printf(_("Error while adding a new handler stub: %s"),
					                error->message);
				gchar *hint_message =
					g_strdup_printf(_("To avoid this messages turn off \"%s\" flag in the %s->%s"),
									_("Insert handler on edit"),
									_("Preferences"),
									_("Glade GUI Designer"));

				anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
										   "%s. %s", error_message, hint_message);
				g_error_free (error);
				g_free (error_message);
				g_free (hint_message);
			}
		}
	}

	return FALSE;
}
#endif

static void
switch_designer_and_editor (GladePlugin* plugin)
{
	IAnjutaDocumentManager* docman = NULL;
	GList* docwids = NULL;
	GList *node;
	GFile *assoc_file = NULL;
	IAnjutaLanguage *ilanguage;
	IAnjutaDocument* doc;
	IAnjutaFile *file = NULL;
	DesignerAssociationsItem *matching_item = NULL;
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);

	ilanguage = anjuta_shell_get_interface (anjuta_plugin->shell,
	                                        IAnjutaLanguage, NULL);
	if (!ilanguage)
		return;

	docman = anjuta_shell_get_interface (anjuta_plugin->shell,
	                                     IAnjutaDocumentManager, NULL);
	if (docman == NULL)
		return;
	doc = ianjuta_document_manager_get_current_document (docman, NULL);

	if (IANJUTA_IS_EDITOR(doc))
	{
		GList *list = get_associated_items_for_editor (doc, plugin);
		file = IANJUTA_FILE(find_valid_designer (list, plugin, &matching_item));
		g_list_free (list);
		if (file)
			assoc_file = ianjuta_file_get_file (file, NULL);
	}
	else if (ANJUTA_IS_DESIGN_DOCUMENT(doc))
	{
		GList *list;
		const gchar *widget_name = NULL;
		/* FIXME: use last_toplevel widget name? */
#ifdef GLADE_SIGNAL_EDITOR_EXT
		GladeWidget *widget = GLADE_WIDGET (plugin->priv->last_gse->widget);
		if (widget)
			widget = find_root_gwidget (widget);
		if (widget)
			widget_name = glade_widget_get_name (widget);
#endif

		list = get_associated_items_for_designer (doc, widget_name, plugin);
		file = IANJUTA_FILE(find_valid_editor (list, plugin, &matching_item));
		g_list_free (list);
		if (file)
			assoc_file = ianjuta_file_get_file (file, NULL);
	}

	if (!assoc_file)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                           "Couldn't find an associated document");
		return;
	}

	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			GFile *tmp_file;

			tmp_file = ianjuta_file_get_file (IANJUTA_FILE(node->data), NULL);
			if (g_file_equal (tmp_file, assoc_file))
			{
				if (plugin->priv->separated_designer_layout &&
				    ANJUTA_IS_DESIGN_DOCUMENT (node->data))
				{
					AnjutaDesignDocument *doc = ANJUTA_DESIGN_DOCUMENT (node->data);
					GladeDesignView *design_view = anjuta_design_document_get_design_view (doc);
					glade_app_set_project (glade_design_view_get_project (design_view));
				}
				else
				{
					ianjuta_document_manager_set_current_document (docman,
					                                               IANJUTA_DOCUMENT (node->data), NULL);
					ianjuta_document_grab_focus (IANJUTA_DOCUMENT(node->data), NULL);
					g_object_unref (tmp_file);
					break;
				}
			}
			g_object_unref (tmp_file);
		}
		g_list_free (docwids);
	}

	g_object_unref (assoc_file);
}

static void
on_switch_designer_and_editor (GtkAction* action, GladePlugin* plugin)
{
	switch_designer_and_editor (plugin);
}

static void
associate_designer_and_editor (DesignerAssociations *associations,
                               GFile *designer, gchar *widget_name,
                               GFile *editor,
                               GFile *project_root,
                               GtkWindow *window,
                               GladePlugin* plugin)
{
	DesignerAssociationsItem *item;

	item = designer_associations_search_item (associations,
	                                          editor,
	                                          designer);
	if (item)
	{
		gchar *designer_path, *editor_path;

		designer_path = g_file_get_path (designer);
		editor_path = g_file_get_path (editor);
		g_warning ("Association \"%s\" <=> \"%s\" already exists",
		            designer_path, editor_path);
		anjuta_util_dialog_warning (window,
		                           _("Those documents are already associated"));
		g_free (designer_path);
		g_free (editor_path);
		return;
	}

	item = designer_associations_item_from_data (editor,
	                                             NULL, designer,
	                                             NULL, project_root);
	designer_associations_item_set_option (item, "position_type", ipt_names[3]);
	designer_associations_add_item (associations, item);
}

static void
on_associate_designer_and_editor (GtkAction* action, GladePlugin* plugin)
{
	if (plugin->priv->last_designer && plugin->priv->last_editor)
	{
		associate_designer_and_editor (plugin->priv->associations,
		                               plugin->priv->last_designer, NULL,
		                               plugin->priv->last_editor,
		                               plugin->priv->project_root,
		                               GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                               plugin);
	}
}

static GtkBuilder *
glade_plugin_get_glade_xml (GladePlugin* plugin)
{
	if (!plugin->priv->xml)
	{
		GError *error = NULL;

		plugin->priv->xml = gtk_builder_new ();
		if (!gtk_builder_add_from_file (plugin->priv->xml, GLADE_PLUGIN_GLADE_UI_FILE, &error))
		{
			anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
			                          _("Unable to read file: %s."),
			                          GLADE_PLUGIN_GLADE_UI_FILE);
			g_object_unref (plugin->priv->xml);
			plugin->priv->xml = NULL;
			if (error)
			{
				g_warning ("%s", error->message);
				g_error_free (error);
			}
			return NULL;
		}
		if (error)
		{
			g_warning ("%s", error->message);
			g_error_free (error);
		}

		gtk_builder_connect_signals (plugin->priv->xml, plugin);
	}

	return plugin->priv->xml;
}



/* ------- Associations dialog ------- */

#define MODEL_INITED "model_inited"
#define FIELD_CHANGED "field_changed"
#define ASSOCIATIONS_ITEM "associations_item"
#define ASSOCIATIONS_DIALOG_NAME "associations_dialog"
#define ASSOCIATIONS_TREEVIEW_NAME "treeview_associations"
#define ASSOCIATIONS_SPEC_REGEXP_NAME "spec_regexp"
#define ASSOCIATIONS_WIDGET_NAME_NAME "widget_name"
#define ASSOCIATIONS_SPEC_REGEXP_CHECKBOX_NAME "checkbutton_spec_regexp"
#define ASSOCIATIONS_WIDGET_NAME_CHECKBOX_NAME "checkbutton_widget_name"
#define ASSOCIATIONS_SPEC_REGEXP_REVERT_NAME "button_revert_spec_regexp"
#define ASSOCIATIONS_WIDGET_NAME_REVERT_NAME "button_revert_widget_name"
#define ASSOCIATIONS_POSITION_TYPE_NAME "position_type"
#define ASSOCIATIONS_DESIGNERS_LIST_NAME "designers_list"
#define ASSOCIATIONS_EDITORS_LIST_NAME "editors_list"
#define ASSOCIATIONS_ASSOCIATE_BUTTON_NAME "associate_button"
#define ASSOCIATIONS_TABLE_OPTIONS_NAME "table_options"
#define ASSOCIATIONS_OPTIONS_SAVE_NAME "options_save"
#define ASSOCIATIONS_OPTIONS_REVERT_NAME "options_revert"
#define ASSOCIATIONS_DIALOG_XML "associations_xml"

enum {
	ID_COLUMN,
	ITEM_COLUMN,
	DESIGNER_COLUMN,
	WIDGET_COLUMN,
	EDITOR_COLUMN,
	OPTIONS_COLUMN,
	LAST_COLUMN
};

void
associations_dialog_position_type_changed_cb (GtkComboBox *widget, GladePlugin* plugin);
void
associations_dialog_button_delete_cb (GtkButton *button, GladePlugin* plugin);
void
associations_dialog_button_close_cb (GtkButton *button, GladePlugin* plugin);
void
associations_dialog_button_save_cb (GtkButton *button, GladePlugin* plugin);
void
associations_dialog_button_associate_cb (GtkButton *self, GladePlugin *plugin);
void
associations_dialog_options_save_cb (GtkButton *self, GladePlugin *plugin);
void
associations_dialog_options_revert_cb (GtkButton *self, GladePlugin *plugin);
void
associations_dialog_spec_regexp_toggled (GtkToggleButton *togglebutton, GladePlugin *plugin);
void
associations_dialog_widget_name_toggled (GtkToggleButton *togglebutton, GladePlugin *plugin);
void
associations_dialog_revert_spec_regexp (GtkButton *button, GladePlugin *plugin);
void
associations_dialog_revert_widget_name (GtkButton *button, GladePlugin *plugin);
void
associations_dialog_spec_regexp_changed (GtkEditable *editable, GladePlugin *plugin);
void
associations_dialog_widget_name_changed (GtkEditable *editable, GladePlugin *plugin);
void
associations_dialog_spec_regexp_activate (GtkEntry *entry, GladePlugin *plugin);
void
associations_dialog_widget_name_activate (GtkEntry *entry, GladePlugin *plugin);

static gboolean
gtk_tree_selection_get_one_selected (GtkTreeSelection *selection, GtkTreeModel **model,
                                     GtkTreeIter *iter)
{
	GList *selected;
	GtkTreeModel *tree_model;

	if (gtk_tree_selection_count_selected_rows (selection) != 1)
		return FALSE;
	selected = gtk_tree_selection_get_selected_rows (selection, &tree_model);
	gtk_tree_model_get_iter (tree_model, iter, selected->data);
	gtk_tree_path_free (selected->data);
	g_list_free (selected);
	if (model)
		*model = tree_model;
	return TRUE;
}

static void
associations_dialog_mark_all_fields_as_unchanged (GladePlugin *plugin)
{
	bzero (plugin->priv->dialog_data->fields_changed,
	       sizeof(plugin->priv->dialog_data->fields_changed));

	if (plugin->priv->dialog_data->current_item)
	{
		g_object_unref (plugin->priv->dialog_data->current_item);
		plugin->priv->dialog_data->current_item = NULL;
	}

	gtk_widget_set_sensitive (plugin->priv->dialog_data->options_button_save, FALSE);
	gtk_widget_set_sensitive (plugin->priv->dialog_data->options_button_revert, FALSE);
}

static void
associations_dialog_load_all_field (GladePlugin *plugin)
{
	GtkTreeIter iter;
	DesignerAssociationsItem *item;
	GtkTreeModel *model;
	gchar *spec_regexp;
	GtkEntry *entry;
	GtkCheckButton *checkbutton;
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkComboBox *combobox;
	gint i;

	g_return_if_fail (plugin->priv->dialog);
	g_return_if_fail (plugin->priv->xml);

	treeview = plugin->priv->dialog_data->treeview;
	selection = gtk_tree_view_get_selection (treeview);
	g_return_if_fail (gtk_tree_selection_get_one_selected (selection, &model, &iter));

	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
	                    ITEM_COLUMN, &item, -1);

	/* Special regexp */
	spec_regexp = designer_associations_item_get_option (item, "spec_regexp");
	entry = GTK_ENTRY (plugin->priv->dialog_data->options_entries[DO_SPEC_REGEXP]);
	checkbutton = GTK_CHECK_BUTTON (plugin->priv->dialog_data->options_checkboxes[DO_SPEC_REGEXP]);
	gtk_entry_set_text (entry, spec_regexp ? spec_regexp : "");
	gtk_widget_set_sensitive (GTK_WIDGET (entry), spec_regexp != NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), spec_regexp != NULL);
	g_free (spec_regexp);

	/* Widget name */
	entry = GTK_ENTRY (plugin->priv->dialog_data->options_entries[DO_WIDGET_NAME]);
	checkbutton = GTK_CHECK_BUTTON (plugin->priv->dialog_data->options_checkboxes[DO_WIDGET_NAME]);
	gtk_entry_set_text (entry, item->widget_name ? item->widget_name : "");
	gtk_widget_set_sensitive (GTK_WIDGET (entry), item->widget_name != NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), item->widget_name != NULL);

	/* Position type */
	combobox = GTK_COMBO_BOX (plugin->priv->dialog_data->options_entries[DO_POSITION_TYPE]);
	i = designer_associations_item_get_option_as_int (item, "position_type", ipt_names);
	gtk_combo_box_set_active (combobox, i);

	associations_dialog_mark_all_fields_as_unchanged (plugin);

	g_object_unref (G_OBJECT (item));
}

static void
associations_dialog_clear_all_field (GladePlugin *plugin)
{
	GtkEntry *entry;
	GtkCheckButton *checkbutton;
	GtkComboBox *combobox;

	g_return_if_fail (plugin->priv->dialog);
	g_return_if_fail (plugin->priv->xml);

	/* Special regexp */
	entry = GTK_ENTRY (plugin->priv->dialog_data->options_entries[DO_SPEC_REGEXP]);
	checkbutton = GTK_CHECK_BUTTON (plugin->priv->dialog_data->options_checkboxes[DO_SPEC_REGEXP]);
	gtk_entry_set_text (entry, "");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), FALSE);

	/* Widget name */
	entry = GTK_ENTRY (plugin->priv->dialog_data->options_entries[DO_WIDGET_NAME]);
	checkbutton = GTK_CHECK_BUTTON (plugin->priv->dialog_data->options_checkboxes[DO_WIDGET_NAME]);
	gtk_entry_set_text (entry, "");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), FALSE);

	/* Position type */
	combobox = GTK_COMBO_BOX (plugin->priv->dialog_data->options_entries[DO_POSITION_TYPE]);
	gtk_combo_box_set_active (combobox, -1);

	associations_dialog_mark_all_fields_as_unchanged (plugin);
}

static void
associations_dialog_update_options_editor (GladePlugin *plugin)
{
	GtkTreeIter iter;
	GtkBuilder *xml;
	GtkTreeModel *model;
	GtkWidget *table;
	GtkTreeSelection *selection;

	g_return_if_fail (plugin->priv->dialog);
	g_return_if_fail (plugin->priv->xml);
	g_return_if_fail (!plugin->priv->dialog_data->updating);
	plugin->priv->dialog_data->updating = TRUE;

	xml = plugin->priv->xml;
	selection = gtk_tree_view_get_selection (plugin->priv->dialog_data->treeview);
	table = GTK_WIDGET (plugin->priv->dialog_data->options_table);

	if (gtk_tree_selection_get_one_selected (selection, &model, &iter))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (table), TRUE);
		associations_dialog_load_all_field (plugin);
	}
	else
	{
		gtk_widget_set_sensitive (GTK_WIDGET (table), FALSE);
		associations_dialog_clear_all_field (plugin);
	}

	plugin->priv->dialog_data->updating = FALSE;
}

static void
fill_position_type_combobox_model (GtkListStore *model)
{
	GtkTreeIter iter;
	int i;

	for (i=0; i<count_ipt; i++)
	{
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
		                    0, i,
		                    1, ipt_names[i],
		                    -1);
	}
}

static void
associations_dialog_update_model (GtkListStore *model, GladePlugin *plugin)
{
	DesignerAssociationsItem *item;
	GList *node;

	g_return_if_fail (plugin->priv->dialog);
	g_return_if_fail (!plugin->priv->dialog_data->updating);
	plugin->priv->dialog_data->updating = TRUE;

	gtk_list_store_clear (model);

	for (node = plugin->priv->associations->associations; node;
	     node = node->next)
	{
		gchar *designer, *editor, *options;
		GtkTreeIter iter;

		item = node->data;
		designer = g_file_get_basename (item->designer);
		editor = g_file_get_basename (item->editor);
		options = designer_associations_options_to_string (item->options, "=", ";");

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
		                    ID_COLUMN, item->id,
		                    ITEM_COLUMN, item,
		                    DESIGNER_COLUMN, designer,
		                    WIDGET_COLUMN, item->widget_name,
		                    EDITOR_COLUMN, editor,
		                    OPTIONS_COLUMN, options,
		                    -1);

		g_free (designer);
		g_free (editor);
		g_free (options);
	}

	plugin->priv->dialog_data->updating = FALSE;
}

static void
associations_dialog_update_row (DesignerAssociationsItem *item, GtkTreeModel *model,
                                GtkTreeIter *iter, GladePlugin *plugin)
{
	gchar *designer, *editor, *options;

	g_return_if_fail (plugin->priv->dialog);
	g_return_if_fail (!plugin->priv->dialog_data->updating);
	plugin->priv->dialog_data->updating = TRUE;

	designer = g_file_get_basename (item->designer);
	editor = g_file_get_basename (item->editor);
	options = designer_associations_options_to_string (item->options, "=", ";");
	gtk_list_store_set (GTK_LIST_STORE (model), iter,
	                    DESIGNER_COLUMN, designer,
	                    WIDGET_COLUMN, item->widget_name,
	                    EDITOR_COLUMN, editor,
	                    OPTIONS_COLUMN, options,
	                    -1);

	g_free (designer);
	g_free (editor);
	g_free (options);

	plugin->priv->dialog_data->updating = FALSE;
}

static const gchar *
get_string_from_entry (GtkEntry *entry, GtkCheckButton *checkbutton)
{
	const gchar *value;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)) == FALSE)
		value = NULL;
	else
	{
		value = gtk_entry_get_text (entry);
		if (value && !strcmp (value, ""))
			value = NULL;
	}

	return value;
}

static gboolean
associations_dialog_commit_all_fields (GladePlugin *plugin)
{
	DesignerAssociationsItem *item;
	GtkComboBox *combobox;

	/* plugin->priv->dialog may be null while closing the dialog */
	g_return_val_if_fail (plugin->priv->xml, FALSE);
	g_return_val_if_fail (!plugin->priv->dialog_data->updating, FALSE);

	item = plugin->priv->dialog_data->current_item;
	if (!item)
		return FALSE;
	g_object_ref (item);

	if (plugin->priv->dialog_data->fields_changed[DO_SPEC_REGEXP])
	{
		designer_associations_item_set_option (item, "spec_regexp",
		                                       get_string_from_entry (GTK_ENTRY (plugin->priv->dialog_data->options_entries[DO_SPEC_REGEXP]),
		                                                              GTK_CHECK_BUTTON (plugin->priv->dialog_data->options_checkboxes[DO_SPEC_REGEXP])));
	}

	if (plugin->priv->dialog_data->fields_changed[DO_WIDGET_NAME])
	{
		designer_associations_item_set_widget_name (item,
		                                            get_string_from_entry (GTK_ENTRY (plugin->priv->dialog_data->options_entries[DO_WIDGET_NAME]),
	                                                                       GTK_CHECK_BUTTON (plugin->priv->dialog_data->options_checkboxes[DO_WIDGET_NAME])));
	}

	if (plugin->priv->dialog_data->fields_changed[DO_POSITION_TYPE])
	{
		gint i;
		combobox = GTK_COMBO_BOX (plugin->priv->dialog_data->options_entries[DO_POSITION_TYPE]);
		i = gtk_combo_box_get_active (combobox);
		if (i >= 0 && i < count_ipt)
			designer_associations_item_set_option (item, "position_type", ipt_names[i]);
		else
			g_warning ("Invalid item index of position type");
	}

	designer_associations_notify_changed (plugin->priv->associations, item);

	g_object_unref (item);
	plugin->priv->dialog_data->current_item = NULL;

	return TRUE;
}

void
associations_dialog_options_save_cb (GtkButton *self, GladePlugin *plugin)
{
	associations_dialog_commit_all_fields (plugin);
}

void
associations_dialog_options_revert_cb (GtkButton *self, GladePlugin *plugin)
{
	associations_dialog_update_options_editor (plugin);
}

static gboolean
associations_dialog_mark_field_as_changed (GladePlugin *plugin, gint index)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	DesignerAssociationsItem *item = NULL;
	GtkTreeModel *model;

	g_return_val_if_fail (plugin->priv->dialog, FALSE);
	g_return_val_if_fail (plugin->priv->xml, FALSE);
	if (plugin->priv->dialog_data->updating)
		return FALSE;

	treeview = GTK_TREE_VIEW(plugin->priv->dialog_data->treeview);
	selection = gtk_tree_view_get_selection (treeview);
	g_return_val_if_fail (gtk_tree_selection_get_one_selected (selection, &model, &iter), FALSE);

	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
	                    ITEM_COLUMN, &item, -1);
	g_return_val_if_fail (item, FALSE);
	if  (plugin->priv->dialog_data->current_item &&
	     plugin->priv->dialog_data->current_item != item)
	{
		g_warning ("plugin->priv->dialog_data->current_item != item");
	}
	g_object_unref (item);

	plugin->priv->dialog_data->fields_changed[index] = TRUE;
	plugin->priv->dialog_data->current_item = item;

	gtk_widget_set_sensitive (plugin->priv->dialog_data->options_button_save, TRUE);
	gtk_widget_set_sensitive (plugin->priv->dialog_data->options_button_revert, TRUE);

	return TRUE;
}

void
associations_dialog_spec_regexp_changed (GtkEditable *editable, GladePlugin *plugin)
{
	associations_dialog_mark_field_as_changed (plugin, DO_SPEC_REGEXP);
}

void
associations_dialog_widget_name_changed (GtkEditable *editable, GladePlugin *plugin)
{
	associations_dialog_mark_field_as_changed (plugin, DO_WIDGET_NAME);
}

void
associations_dialog_spec_regexp_toggled (GtkToggleButton *togglebutton, GladePlugin *plugin)
{
	if (associations_dialog_mark_field_as_changed (plugin, DO_SPEC_REGEXP))
		gtk_widget_set_sensitive (plugin->priv->dialog_data->options_entries[DO_SPEC_REGEXP],
					  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin->priv->dialog_data->options_checkboxes[DO_SPEC_REGEXP])));
}

void
associations_dialog_widget_name_toggled (GtkToggleButton *togglebutton, GladePlugin *plugin)
{
	if (associations_dialog_mark_field_as_changed (plugin, DO_WIDGET_NAME))
		gtk_widget_set_sensitive (plugin->priv->dialog_data->options_entries[DO_WIDGET_NAME],
					  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin->priv->dialog_data->options_checkboxes[DO_WIDGET_NAME])));
}

void
associations_dialog_position_type_changed_cb (GtkComboBox *widget, GladePlugin* plugin)
{
	associations_dialog_mark_field_as_changed (plugin, DO_POSITION_TYPE);
}

void
associations_dialog_button_delete_cb (GtkButton *button, GladePlugin* plugin)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	gint selected_count;
	GtkTreeModel *model;

	g_return_if_fail (plugin->priv->dialog);

	treeview = GTK_TREE_VIEW(gtk_builder_get_object (plugin->priv->xml,
	                                                 ASSOCIATIONS_TREEVIEW_NAME));
	g_return_if_fail (treeview);

	selection = gtk_tree_view_get_selection (treeview);
	selected_count = gtk_tree_selection_count_selected_rows (selection);
	if (selected_count > 0)
	{
		GList *list, *node;
		gint id;
		GtkTreeIter iter;

		if (selected_count > 1)
			designer_associations_lock_notification (plugin->priv->associations);

		node = list = gtk_tree_selection_get_selected_rows (selection, &model);
		while (node)
		{
			/* TODO: remember cursor position */
			gtk_tree_model_get_iter (model, &iter, node->data);
			gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
			designer_associations_remove_item_by_id (plugin->priv->associations, id);

			node = node->next;
		}
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		if (selected_count > 1)
			designer_associations_unlock_notification (plugin->priv->associations);
	}
}

static void
glade_plugin_hide_associations_dialog (GladePlugin* plugin)
{
	GtkWindow *dialog;

	g_return_if_fail (plugin->priv->dialog);
	g_return_if_fail (plugin->priv->xml);

	dialog = plugin->priv->dialog;
	/* Stops listening on association events */
	plugin->priv->dialog = NULL;

	gtk_widget_hide (GTK_WIDGET(dialog));

	if (plugin->priv->dialog_data->current_item)
		associations_dialog_commit_all_fields (plugin);
}

void
associations_dialog_button_close_cb (GtkButton *button, GladePlugin* plugin)
{
	glade_plugin_hide_associations_dialog (plugin);
}

void
associations_dialog_button_save_cb (GtkButton *button, GladePlugin* plugin)
{
	GError *error = NULL;
	glade_plugin_do_save_associations (plugin, &error);
	if (error)
	{
		anjuta_util_dialog_error (plugin->priv->dialog,
		                          "Error: %s", error->message);
		g_error_free (error);
	}
}

static gboolean
associations_dialog_delete_event_cb (GtkWindow *dialog, GdkEvent *event, GladePlugin* plugin)
{
	DEBUG_PRINT ("dialog hidden");

	if (plugin->priv->dialog)
	{
		g_return_val_if_fail (plugin->priv->xml, TRUE);

		glade_plugin_hide_associations_dialog (plugin);
	}

	return TRUE;
}

static void
associations_dialog_selection_changed (GtkTreeSelection *selection, GladePlugin* plugin)
{
	g_return_if_fail (plugin->priv->dialog);
	g_return_if_fail (plugin->priv->xml);
	if (plugin->priv->dialog_data->updating)
		return;

	/* Changing associations leads to options editor update */
	associations_dialog_commit_all_fields (plugin);
	associations_dialog_update_options_editor (plugin);
}

static void
associations_dialog_insert_text_column (GtkTreeView *treeview, gchar *title,
                                        gint column_id, gint column_width)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (title, renderer,
	                                                  "text", column_id,
	                                                   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_fixed_width (column, column_width);
	gtk_tree_view_insert_column (treeview, column, -1);
}

static void
show_associations_dialog (GladePlugin* plugin)
{
	GtkBuilder *xml;
	GtkTreeView *treeview;
	GtkCellRenderer *renderer;
	GtkWindow *dialog;
	GtkComboBox *combobox;
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);

	if (plugin->priv->dialog)
	{
		gtk_window_present (plugin->priv->dialog);
		return;
	}

	g_return_if_fail (plugin->priv->xml);
	xml = plugin->priv->xml;

	plugin->priv->dialog = dialog =
		GTK_WINDOW (gtk_builder_get_object (xml, ASSOCIATIONS_DIALOG_NAME));
	if (!dialog || !GTK_IS_WINDOW (dialog))
	{
		g_warning (_("Widget not found: %s"), ASSOCIATIONS_DIALOG_NAME);
		g_object_unref (xml);
		return;
	}
	g_signal_handlers_disconnect_by_func (dialog,
	                                      G_CALLBACK (associations_dialog_delete_event_cb),
	                                      plugin);
	g_signal_connect (dialog, "delete-event",
	                  G_CALLBACK (associations_dialog_delete_event_cb), plugin);
	{
		treeview = plugin->priv->dialog_data->treeview =
			GTK_TREE_VIEW (gtk_builder_get_object (xml, ASSOCIATIONS_TREEVIEW_NAME));
		plugin->priv->dialog_data->options_button_save =
			GTK_WIDGET(gtk_builder_get_object (xml, ASSOCIATIONS_OPTIONS_SAVE_NAME));
		plugin->priv->dialog_data->options_button_revert =
			GTK_WIDGET(gtk_builder_get_object (xml, ASSOCIATIONS_OPTIONS_REVERT_NAME));
		plugin->priv->dialog_data->options_table =
			GTK_TABLE(gtk_builder_get_object (xml, ASSOCIATIONS_TABLE_OPTIONS_NAME));

		plugin->priv->dialog_data->options_entries[0] =
			GTK_WIDGET(gtk_builder_get_object (xml, ASSOCIATIONS_SPEC_REGEXP_NAME));
		plugin->priv->dialog_data->options_entries[1] =
			GTK_WIDGET(gtk_builder_get_object (xml, ASSOCIATIONS_WIDGET_NAME_NAME));
		plugin->priv->dialog_data->options_entries[2] =
			GTK_WIDGET(gtk_builder_get_object (xml, ASSOCIATIONS_POSITION_TYPE_NAME));

		plugin->priv->dialog_data->options_checkboxes[0] =
			GTK_WIDGET(gtk_builder_get_object (xml, ASSOCIATIONS_SPEC_REGEXP_CHECKBOX_NAME));
		plugin->priv->dialog_data->options_checkboxes[1] =
			GTK_WIDGET(gtk_builder_get_object (xml, ASSOCIATIONS_WIDGET_NAME_CHECKBOX_NAME));

	}

	if (!treeview)
	{
		g_warning (_("Widget not found: %s"), ASSOCIATIONS_TREEVIEW_NAME);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		g_object_unref (xml);
		return;
	}
	if (GPOINTER_TO_INT(g_object_get_data (G_OBJECT(treeview), MODEL_INITED)) == FALSE)
	{
		GtkListStore *model;
		g_object_set_data (G_OBJECT(treeview), MODEL_INITED, GINT_TO_POINTER (TRUE));
		model = gtk_list_store_new (LAST_COLUMN,
		                            G_TYPE_INT,
		                            DESIGNER_TYPE_ASSOCIATIONS_ITEM,
		                            G_TYPE_STRING,
		                            G_TYPE_STRING,
		                            G_TYPE_STRING,
		                            G_TYPE_STRING);
		gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (model));

		associations_dialog_insert_text_column (treeview, "designer", DESIGNER_COLUMN, 200);
		associations_dialog_insert_text_column (treeview, "toplevel widget", WIDGET_COLUMN, 150);
		associations_dialog_insert_text_column (treeview, "editor", EDITOR_COLUMN, 200);
		associations_dialog_insert_text_column (treeview, "options", OPTIONS_COLUMN, 200);

		g_object_set_data (G_OBJECT (dialog), ASSOCIATIONS_TREEVIEW_NAME, treeview);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview),
		                             GTK_SELECTION_MULTIPLE);
		g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (treeview)),
				 "changed", G_CALLBACK (associations_dialog_selection_changed),
				  plugin);
	}
	associations_dialog_update_model (GTK_LIST_STORE(gtk_tree_view_get_model (treeview)),
	                                  plugin);

	combobox = GTK_COMBO_BOX (plugin->priv->dialog_data->options_entries[2]);
	if (GPOINTER_TO_INT(g_object_get_data (G_OBJECT(combobox), MODEL_INITED)) == FALSE)
	{
		GtkListStore *model;
		g_object_set_data (G_OBJECT(combobox), MODEL_INITED, GINT_TO_POINTER (TRUE));
		model = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
		gtk_combo_box_set_model (combobox, GTK_TREE_MODEL (model));
		fill_position_type_combobox_model (model);
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
		                               "text", 1, NULL);
	}

	combobox = GTK_COMBO_BOX (gtk_builder_get_object (xml, ASSOCIATIONS_DESIGNERS_LIST_NAME));
	if (GPOINTER_TO_INT(g_object_get_data (G_OBJECT(combobox), MODEL_INITED)) == FALSE)
	{
		GtkListStore *model;
		g_object_set_data (G_OBJECT(combobox), MODEL_INITED, GINT_TO_POINTER (TRUE));
		model = gtk_list_store_new (2, G_TYPE_FILE, G_TYPE_STRING);
		gtk_combo_box_set_model (combobox, GTK_TREE_MODEL (model));
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
		                               "text", 1, NULL);
	}

	combobox = GTK_COMBO_BOX (gtk_builder_get_object (xml, ASSOCIATIONS_EDITORS_LIST_NAME));
	if (GPOINTER_TO_INT(g_object_get_data (G_OBJECT(combobox), MODEL_INITED)) == FALSE)
	{
		GtkListStore *model;
		g_object_set_data (G_OBJECT(combobox), MODEL_INITED, GINT_TO_POINTER (TRUE));
		model = gtk_list_store_new (2, G_TYPE_FILE, G_TYPE_STRING);
		gtk_combo_box_set_model (combobox, GTK_TREE_MODEL (model));
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
		                               "text", 1, NULL);
	}

	doc_list_changed (anjuta_plugin);

	associations_dialog_update_options_editor (plugin);

	gtk_window_set_transient_for (dialog, GTK_WINDOW (anjuta_plugin->shell));
	gtk_window_set_modal (dialog, FALSE);

	gtk_widget_show_all (GTK_WIDGET (dialog));
}

static void
on_show_associations_dialog (GtkAction* action, GladePlugin* plugin)
{
	if (glade_plugin_get_glade_xml (plugin))
		show_associations_dialog (plugin);
}

static void
on_associations_changed (DesignerAssociations *self, DesignerAssociationsItem *item,
                         DesignerAssociationsAction action, GladePlugin *plugin)
{
	if (plugin->priv->dialog)
	{
		GtkTreeView *treeview;
		GtkTreeModel *model;
		GtkTreeSelection *selection;

		treeview = GTK_TREE_VIEW (gtk_builder_get_object (plugin->priv->xml,
		                                                  ASSOCIATIONS_TREEVIEW_NAME));
		selection = gtk_tree_view_get_selection (treeview);
		model = gtk_tree_view_get_model (treeview);
		if (action == DESIGNER_ASSOCIATIONS_CHANGED)
		{
			GtkTreeIter iter;
			gboolean found = FALSE;
			if (!gtk_tree_model_get_iter_first (model, &iter))
				return;
			do
			{
				guint id;

				gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);
				if (id == item->id)
					found = TRUE;
			} while (!found && gtk_tree_model_iter_next (model, &iter));
			if (found)
			{
				associations_dialog_update_row (item, model, &iter, plugin);
				if (plugin->priv->dialog_data->current_item)
				{
					GtkTreeIter selected_iter;
					if (gtk_tree_selection_get_one_selected (selection, NULL, &selected_iter))
					{
						DesignerAssociationsItem *selected_item;
						gtk_tree_model_get (model, &selected_iter,
						                    ITEM_COLUMN, &selected_item, -1);
						if (selected_item &&
						    plugin->priv->dialog_data->current_item != selected_item)
						{
							/* Do not update editor */
							return;
						}
					}
				}
			}
		}
		else
		{
			GList *list, *node;
			GList *refs_list = NULL;
			/* Save selection */
			list = gtk_tree_selection_get_selected_rows (selection, NULL);
			for (node = list; node; node = node->next)
			{
				refs_list = g_list_prepend (refs_list,
				                            gtk_tree_row_reference_new (model, node->data));
			}
			g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
			g_list_free (list);

			associations_dialog_update_model (GTK_LIST_STORE (gtk_tree_view_get_model (treeview)),
							  plugin);
			/* Restore selection */
			for (node = refs_list; node; node = node->next)
			{
				if (node->data)
				{
					GtkTreePath *path = gtk_tree_row_reference_get_path (node->data);
					if (path)
						gtk_tree_selection_select_path (selection, path);
					gtk_tree_row_reference_free (node->data);
					gtk_tree_path_free (path);
				}
			}
		}

		associations_dialog_update_options_editor (plugin);
	}
}

static void
doc_list_changed (AnjutaPlugin *anjuta_plugin)
{
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE(anjuta_plugin);
	IAnjutaDocumentManager *docman;
	GList *docwids, *node;
	GtkListStore *designers, *editors;
	GtkComboBox *designer_combobox, *editor_combobox;
	gboolean is_designer;
	GtkTreeIter iter;

	if (!plugin->priv->dialog)
		return;
	g_return_if_fail (plugin->priv->xml);

	designer_combobox = GTK_COMBO_BOX(gtk_builder_get_object (plugin->priv->xml,
	                                                          ASSOCIATIONS_DESIGNERS_LIST_NAME));
	g_return_if_fail (designer_combobox);
	designers = GTK_LIST_STORE(gtk_combo_box_get_model (designer_combobox));
	g_return_if_fail (designers);
	gtk_list_store_clear (designers);

	editor_combobox = GTK_COMBO_BOX(gtk_builder_get_object (plugin->priv->xml,
	                                                        ASSOCIATIONS_EDITORS_LIST_NAME));
	g_return_if_fail (editor_combobox);
	editors = GTK_LIST_STORE(gtk_combo_box_get_model (editor_combobox));
	g_return_if_fail (editors);
	gtk_list_store_clear (editors);

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			is_designer = FALSE;
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
				is_designer = TRUE;
			if (is_designer || IANJUTA_IS_EDITOR (node->data))
			{
				GFile* file;
				file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (file != NULL)
				{
					gchar *basename;

					basename = g_file_get_basename (file);
					if (is_designer)
					{
						gtk_list_store_append (designers, &iter);
						gtk_list_store_set (designers, &iter,
						                    0, file,
						                    1, basename, -1);
						if (plugin->priv->last_designer &&
						    g_file_equal (file, plugin->priv->last_designer))
						{
							gtk_combo_box_set_active_iter (designer_combobox, &iter);
						}
					}
					else
					{
						gtk_list_store_append (editors, &iter);
						gtk_list_store_set (editors, &iter,
						                    0, file,
						                    1, basename, -1);
						if (plugin->priv->last_editor &&
						    g_file_equal (file, plugin->priv->last_editor))
						{
							gtk_combo_box_set_active_iter (editor_combobox, &iter);
						}
					}
					g_free (basename);
				}
				g_object_unref (file);
			}
		}
		g_list_free (docwids);
	}
}

void
associations_dialog_button_associate_cb (GtkButton *self, GladePlugin *plugin)
{
	GtkComboBox *designer_combobox, *editor_combobox;
	GtkTreeModel *designers, *editors;
	GtkTreeIter iter;
	GFile *designer_file, *editor_file;

	g_return_if_fail (plugin->priv->xml);

	designer_combobox = GTK_COMBO_BOX(gtk_builder_get_object (plugin->priv->xml,
	                                                          ASSOCIATIONS_DESIGNERS_LIST_NAME));
	g_return_if_fail (designer_combobox);
	g_return_if_fail (gtk_combo_box_get_active_iter (designer_combobox, &iter));
	designers = GTK_TREE_MODEL(gtk_combo_box_get_model (designer_combobox));
	g_return_if_fail (designers);
	gtk_tree_model_get (designers, &iter, 0, &designer_file, -1);

	editor_combobox = GTK_COMBO_BOX(gtk_builder_get_object (plugin->priv->xml,
	                                                        ASSOCIATIONS_EDITORS_LIST_NAME));
	g_return_if_fail (editor_combobox);
	g_return_if_fail (gtk_combo_box_get_active_iter (editor_combobox, &iter));
	editors = GTK_TREE_MODEL(gtk_combo_box_get_model (editor_combobox));
	g_return_if_fail (editors);
	gtk_tree_model_get (editors, &iter, 0, &editor_file, -1);

	associate_designer_and_editor (plugin->priv->associations,
	                               designer_file, NULL,
	                               editor_file,
	                               plugin->priv->project_root,
	                               GTK_WINDOW (plugin->priv->dialog),
	                               plugin);

	g_object_unref (designer_file);
	g_object_unref (editor_file);
}

/* ------- End of associations dialog ------- */


#ifdef GLADE_SIGNAL_EDITOR_EXT
static void
gse_editing_started (GtkEntry *entry, IsVoidFunc callback)
{
	if (callback (gtk_entry_get_text (entry)))
		gtk_entry_set_text (entry, "");
}

static void
handler_store_update (GladeSignalEditor *editor,
                      const gchar *signal_name,
                      GtkListStore *store)
{
	const gchar *handlers[] = {"gtk_widget_show",
	                           "gtk_widget_hide",
	                           "gtk_widget_grab_focus",
	                           "gtk_widget_destroy",
	                           "gtk_true",
	                           "gtk_false",
	                           "gtk_main_quit",
	                            NULL};

	GtkTreeIter tmp_iter;
	gint i;
	gchar *handler, *signal, *name;

	name = (gchar *) glade_widget_get_name (editor->widget);

	signal = g_strdup (signal_name);
	glade_util_replace (signal, '-', '_');

	gtk_list_store_clear (store);

	gtk_list_store_append (store, &tmp_iter);
	handler = g_strdup_printf ("on_%s_%s", name, signal);
	gtk_list_store_set (store, &tmp_iter, 0, handler, -1);
	g_free (handler);

	gtk_list_store_append (store, &tmp_iter);
	handler = g_strdup_printf ("%s_%s_cb", name, signal);
	gtk_list_store_set (store, &tmp_iter, 0, handler, -1);
	g_free (handler);

	g_free (signal);
	for (i = 0; handlers[i]; i++)
	{
		gtk_list_store_append (store, &tmp_iter);
		gtk_list_store_set (store, &tmp_iter, 0, handlers[i], -1);
	}
}

#define MAX_COMPLETION_STORE_SYMBOLS 500

static void
append_symbols_to_store (GtkListStore *store, GladePlugin *plugin)
{
	GtkTreeIter iter;
	IAnjutaSymbolManager *symbol_manager;
	IAnjutaIterable *symbol_iter;
	IAnjutaSymbol *symbol;
	GError *error = NULL;
	int i;
	AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin);

	symbol_manager = anjuta_shell_get_interface (anjuta_plugin->shell,
	                                             IAnjutaSymbolManager, NULL);
	if (!symbol_manager)
		return;

	symbol_iter =
		ianjuta_symbol_manager_search (symbol_manager,
		                               IANJUTA_SYMBOL_TYPE_FUNCTION, TRUE,
		                               IANJUTA_SYMBOL_FIELD_SIMPLE,
		                               "", TRUE, FALSE, FALSE,
		                               MAX_COMPLETION_STORE_SYMBOLS, -1,
		                               &error);
	if (error)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}
	if (symbol_iter)
	{
		/* symbol browser plugin ignores results_limit parameter */
		i = 0;
		while (ianjuta_iterable_get_position (symbol_iter, NULL) >= 0)
		{
			const gchar *name;

			symbol = IANJUTA_SYMBOL (symbol_iter);
			name = ianjuta_symbol_get_name (symbol, NULL);
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, name, -1);

			i ++;
			if (i > MAX_COMPLETION_STORE_SYMBOLS)
				break;
			if (ianjuta_iterable_next (symbol_iter, NULL) == FALSE)
				break;
		}
	}
	else
		DEBUG_PRINT ("No symbols");
}

static gboolean
on_handler_editing_started (GladeSignalEditor *editor,
                            gchar *signal_name,
                            GtkTreeIter *iter,
                            GtkCellEditable *editable,
                            gpointer user_data)
{

	GtkEntry *entry;
	GtkEntryCompletion *completion;
	GtkListStore *completion_store = (GtkListStore *) editor->handler_store;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (user_data);

	g_return_val_if_fail (GTK_IS_BIN (editable), FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (completion_store), FALSE);

	entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (editable)));

	gse_editing_started (entry, editor->is_void_handler);

	handler_store_update (editor, signal_name,
	                      completion_store);
	append_symbols_to_store (completion_store, plugin);

	completion = gtk_entry_completion_new ();
	gtk_entry_completion_set_text_column (completion, 0);
	gtk_entry_completion_set_inline_completion (completion, TRUE);
	gtk_entry_completion_set_popup_completion (completion, FALSE);
	gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (completion_store));
	gtk_entry_set_completion (entry, completion);

	return FALSE;
}
#endif /* GLADE_SIGNAL_EDITOR_EXT */

#ifdef GLADE_SIGNAL_EDITOR_EXT
static gchar *
make_default_handler_name (const gchar *widget_name, const gchar *signal_name,
                           GladePlugin *plugin)
{
	gchar *retval = NULL;

	switch (plugin->priv->default_handler_template)
	{
	case 0:
		return g_strdup_printf ("on_%s_%s", widget_name, signal_name);
		break;
	case 1:
		return g_strdup_printf ("%s_%s_cb", widget_name, signal_name);
		break;
	}

	return retval;
}
#endif

static void
on_signal_row_activated (GtkTreeView       *tree_view,
                         GtkTreePath       *path,
                         GtkTreeViewColumn *column,
                         GladePlugin       *plugin)
{
	GtkTreeIter iter, tmp_iter;
	gchar *signal_handler;
	GtkTreeModel *model;
	gboolean slot;

	model = gtk_tree_view_get_model (tree_view);
	gtk_tree_model_get_iter (model, &iter, path);
	/* if invalid row */
	if (!gtk_tree_model_iter_parent (model, &tmp_iter, &iter))
		return;
	gtk_tree_model_get (model, &iter, GSE_COLUMN_HANDLER, &signal_handler,
	                                  GSE_COLUMN_SLOT, &slot, -1);
	/*if (plugin->priv->last_signal_editor->is_void_handler (signal_handler))*/
	if (slot)
	{
		g_free (signal_handler);
		signal_handler = NULL;
	}

	if (signal_handler)
	{
		goto_symbol (signal_handler, plugin, TRUE);
		g_free (signal_handler);
	}
#ifdef GLADE_SIGNAL_EDITOR_EXT
	else
	{
		GladeWidget *gwidget;
		gchar *signal_handler, *signal_name, *norm_signal_name;
		gboolean handled;

		gwidget = plugin->priv->last_gse->widget;
		if (gwidget)
		{
			if (GTK_IS_TREE_STORE (model))
			{
				signal_name = gse_get_signal_name (model, &iter);
				norm_signal_name = g_strdup (signal_name);
				glade_util_replace (norm_signal_name, '-', '_');

				signal_handler = make_default_handler_name (gwidget->name, norm_signal_name, plugin);

				gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
				                    GSE_COLUMN_HANDLER, signal_handler, -1);

				g_signal_emit_by_name (G_OBJECT (plugin->priv->last_gse), "handler-editing-done",
				                       signal_name, NULL, signal_handler, &iter, &handled);

				g_free (norm_signal_name);
				g_free (signal_name);
				g_free (signal_handler);
			}
		}
	}
#endif
}


#ifdef GLADE_LAYOUT_WIDGET_EVENTS

#define DEFAULT_SIGNAL_NAME_COUNT 16
gchar* default_signal_name[DEFAULT_SIGNAL_NAME_COUNT][2] = {
	{"GtkButton", "clicked"},
	{"GtkToggleButton", "toggled"},
	{"GtkFontButton", "font-set"},
	{"GtkColorButton", "color-set"},
	{"GtkScaleButton", "value-changed"},
	{"GtkRange", "value-changed"},
	{"GtkDialog", "response"},
	{"GtkComboBox", "changed"},
	{"GtkFileChooserButton", "selection-changed"},
	{"GtkFileChooserWidget", "file-activated"},
	{"GtkEntry", "activate"},
	{"GtkSpinButton", "value-changed"},
	{"GtkToolButton", "clicked"},
	{"GtkToggleToolButton", "toggled"},
	{"GtkMenuItem", "activate"},
	{"GtkCheckMenuItem", "toggled"},
};

static gchar *
search_for_default_signal (const gchar *class_name)
{
	gint i;

	for (i=0; i<DEFAULT_SIGNAL_NAME_COUNT; i++)
		if (g_str_equal (class_name, default_signal_name[i][0]))
			return g_strdup (default_signal_name[i][1]);
	return NULL;
}

static gchar *
glade_widget_adaptor_get_default_signal (GladeWidgetAdaptor *adaptor)
{
	gchar *signal_name;

	while (adaptor)
	{
		signal_name = search_for_default_signal (adaptor->name);
		if (signal_name)
			return signal_name;
		adaptor = glade_widget_adaptor_get_parent_adaptor (adaptor);
	}

	return NULL;
}

static void
on_glade_widget_2button_press (GladeProject *project, GladeWidget *gwidget,
                               GdkEvent *event, GladePlugin *plugin)
{
	gboolean found = FALSE;
	GtkTreeIter iter, child_iter;
	GtkTreeModel *model;
	gchar *signal_name, *default_signal_name;
	GtkTreePath *path;
	GtkTreeView *treeview;

	default_signal_name = glade_widget_adaptor_get_default_signal (gwidget->adaptor);
	treeview = GTK_TREE_VIEW (plugin->priv->last_gse->signals_list);
	model = gtk_tree_view_get_model (treeview);
	DEBUG_PRINT ("Searching for default signal %s", default_signal_name);
	if (default_signal_name && gtk_tree_model_get_iter_first (model, &iter))
		do
		{
			gtk_tree_model_iter_nth_child (model, &child_iter, &iter, 0);
			do
			{
				gtk_tree_model_get (model, &child_iter, GSE_COLUMN_SIGNAL, &signal_name, -1);
				if (signal_name && g_str_equal (signal_name, default_signal_name))
					found = TRUE;
				g_free (signal_name);
			}
			while (!found && gtk_tree_model_iter_next (model, &child_iter));
		}
		while (!found && gtk_tree_model_iter_next (model, &iter));

	g_free (default_signal_name);
	if (found)
	{
		DEBUG_PRINT ("Default signal row found");
		gtk_tree_selection_select_iter (gtk_tree_view_get_selection (treeview), &child_iter);
		path = gtk_tree_model_get_path (model, &child_iter);
		on_signal_row_activated (treeview, path, NULL, plugin);
		gtk_tree_path_free (path);
	}
	else
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                           _("Couldn't find a default signal name"));
	}
}

static gint
on_glade_designer_widget_event (GladeProject *project, GladeWidget *gwidget,
                                GdkEvent *event, GladePlugin *glade_plugin)
{
	GladeDesignerMode mode;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN_GLADE (glade_plugin), 0);
	if (event->type != GDK_BUTTON_PRESS && event->type != GDK_2BUTTON_PRESS)
		return 0;

	if ((((GdkEventButton*)event)->state & GDK_MOD4_MASK) == GDK_MOD4_MASK)
		mode = GLADE_DESIGNER_PREVIEW;
	else
		mode = GLADE_DESIGNER_DESIGN;

	switch (mode)
	{
	case GLADE_DESIGNER_DESIGN:
		/* will be handled later */
		return 0;

	case GLADE_DESIGNER_PREVIEW:
		/* just deliver the signal to widget */
		return GLADE_WIDGET_EVENT_STOP_EMISSION;

	case GLADE_DESIGNER_DEFAULT:
		/* cannot handle double-click because of remaining
		 * problem with composited widgets
		 */
		return 0;

	default:
		return 0;
	}
}

static gint
on_glade_designer_widget_event_after (GladeProject *project, GladeWidget *gwidget,
                                      GdkEvent *event, GladePlugin *glade_plugin)
{
	GladeDesignerMode mode;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN_GLADE (glade_plugin), 0);
	if (event->type != GDK_BUTTON_PRESS && event->type != GDK_2BUTTON_PRESS)
		return 0;

	if ((((GdkEventButton*)event)->state & GDK_MOD4_MASK) == GDK_MOD4_MASK)
		mode = GLADE_DESIGNER_PREVIEW;
	else
		mode = GLADE_DESIGNER_DESIGN;

	switch (mode)
	{
	case GLADE_DESIGNER_DESIGN:
		/* default handler for ordinary click has been already called */
		if (event->type == GDK_2BUTTON_PRESS)
			on_glade_widget_2button_press (project, gwidget,
			                               event, glade_plugin);
		/* doesn't allow clicks to reach widgets */
		return GLADE_WIDGET_EVENT_RETURN_TRUE;

	case GLADE_DESIGNER_PREVIEW:
		g_assert_not_reached ();

	case GLADE_DESIGNER_DEFAULT:
		return 0;

	default:
		return 0;
	}
}
#endif /* GLADE_LAYOUT_WIDGET_EVENTS */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * *   End of handlers management  * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
on_designer_doc_update_save_ui (AnjutaDesignDocument *doc,
                            GladePlugin *plugin)
{
	g_return_if_fail (ANJUTA_IS_DESIGN_DOCUMENT (doc));
	if (!plugin->priv->separated_designer_layout)
		return;
	GladeProject *project;
	GtkWidget *child;
	GladeDesignView *view =
		anjuta_design_document_get_design_view (doc);
	project = glade_design_view_get_project (view);
	g_return_if_fail (project);
	child = gtk_widget_get_parent (GTK_WIDGET (view));
	g_return_if_fail (child);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (plugin->priv->new_container),
	                            child,
	                            gtk_label_new (glade_project_get_name (project)));
}

static void
glade_plugin_add_project (GladePlugin *glade_plugin, GladeProject *project,
                          const gchar *project_name)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkWidget *view;
	GtkWidget *doc;
	GladePluginPriv *priv;
	GladeDesignLayout *layout;
	IAnjutaDocumentManager* docman =
		anjuta_shell_get_interface(ANJUTA_PLUGIN(glade_plugin)->shell,
								   IAnjutaDocumentManager, NULL);

 	g_return_if_fail (GLADE_IS_PROJECT (project));
	priv = glade_plugin->priv;

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->projects_combo)));
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
	                    NAME_COL, project_name,
	                    PROJECT_COL, project, -1);

 	view = glade_design_view_new(project);
	g_signal_connect (G_OBJECT(view), "destroy",
	                  G_CALLBACK (on_document_destroy), glade_plugin);

	if (priv->separated_designer_layout)
	{
		GtkContainer *eb = GTK_CONTAINER (gtk_event_box_new());
		doc = anjuta_design_document_new (glade_plugin,
		                                  GLADE_DESIGN_VIEW(view),
		                                  eb);
		designer_layout_add_doc (ANJUTA_DESIGN_DOCUMENT (doc), eb, glade_plugin, TRUE);
	}
	else
	{
		doc = anjuta_design_document_new (glade_plugin,
		                                  GLADE_DESIGN_VIEW(view),
		                                  NULL);
		gtk_widget_show_all (doc);
	}

	g_object_set_data (G_OBJECT (project), "design_document", doc);

	ianjuta_document_manager_add_document(docman, IANJUTA_DOCUMENT(doc), NULL);

	g_signal_connect (G_OBJECT (doc), "update-save-ui",
	                  G_CALLBACK (on_designer_doc_update_save_ui), glade_plugin);

#ifdef GLADE_LAYOUT_WIDGET_EVENTS
	layout = glade_design_view_get_layout (GLADE_DESIGN_VIEW (view));
	g_signal_connect (G_OBJECT (layout), "widget-event",
	                  G_CALLBACK (on_glade_designer_widget_event), glade_plugin);
	g_signal_connect_after (G_OBJECT (layout), "widget-event",
	                        G_CALLBACK (on_glade_designer_widget_event_after), glade_plugin);
#endif

#if (GLADEUI_VERSION >= 330)
	g_signal_connect (G_OBJECT (project), "resource-added",
	                  G_CALLBACK (on_glade_resource_added), glade_plugin);
	g_signal_connect (G_OBJECT (project), "resource-removed",
	                  G_CALLBACK (on_glade_resource_removed), glade_plugin);
#endif
}

static void
set_default_resource_target (const gchar *value, GladePlugin* plugin)
{
	g_free (plugin->priv->default_resource_target);
	if (!value || strlen (value) == 0)
		plugin->priv->default_resource_target = NULL;
	else
		plugin->priv->default_resource_target = g_strdup (value);
	on_default_resource_target_changed (value, plugin);
}

static void
on_set_default_resource_target (GtkAction* action, GladePlugin* plugin)
{
	gchar *selected;
	IAnjutaProjectManager *projman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
		                            IAnjutaProjectManager, NULL);

	selected = ianjuta_project_manager_get_selected_id (projman, IANJUTA_PROJECT_MANAGER_TARGET, NULL);
	DEBUG_PRINT ("Selected element is %s", selected);
	set_default_resource_target (selected, plugin);
	g_free (selected);
}

#if 0
static void
on_glade_verify_project (GtkRadioAction* action, GladePlugin* plugin)
{
	GladeProject *project;
	AnjutaDesignDocument *doc;
	IAnjutaDocumentManager *docman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
		                            IAnjutaDocumentManager, NULL);

	doc = find_designer_by_file (docman, plugin->priv->last_designer);
	project = glade_design_view_get_project (GLADE_DESIGN_VIEW(doc));
	if (glade_project_verify (project, FALSE))
	{
		gchar *name = glade_project_get_name (project);
		anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                         _("Project %s has no deprecated widgets "
		                           "or version mismatches."),
		                         name);
		g_free (name);
	}
}
#endif

static void
on_glade_show_version_dialog (GtkAction* action, GladePlugin* plugin)
{
	GladeProject *project = glade_app_get_project ();
	if (project)
		glade_project_preferences (project);
	else
	{
		anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
		                         _("There's no Glade projects"));
	}
}

static void
glade_plugin_sync_current_doc_with_project (GladePlugin* plugin)
{
	IAnjutaDocument *doc;
	IAnjutaDocumentManager *docman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
		                            IAnjutaDocumentManager, NULL);
	doc = IANJUTA_DOCUMENT (get_design_document_from_project
	                        (glade_app_get_project ()));
	ianjuta_document_manager_set_current_document (docman, doc, NULL);
}

static void
on_glade_layout_close (GtkAction* action, GladePlugin* plugin)
{
	GtkUIManager *ui;
	GtkAction *doc_action;

	glade_plugin_sync_current_doc_with_project (plugin);
	ui = GTK_UI_MANAGER (anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL));
	doc_action = gtk_ui_manager_get_action (ui, "/MenuMain/MenuFile/PlaceholderFileMenus/Close");
	gtk_action_activate (doc_action);
}

static void
on_glade_layout_save (GtkAction* action, GladePlugin* plugin)
{
	GtkUIManager *ui;
	GtkAction *doc_action;

	glade_plugin_sync_current_doc_with_project (plugin);
	ui = GTK_UI_MANAGER (anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL));
	doc_action = gtk_ui_manager_get_action (ui, "/MenuMain/MenuFile/PlaceholderFileMenus/Save");
	gtk_action_activate (doc_action);
}

static void
on_glade_layout_undo (GtkAction* action, GladePlugin* plugin)
{
	g_return_if_fail (glade_app_get_project());
	glade_app_command_undo ();
}

static void
on_glade_layout_redo (GtkAction* action, GladePlugin* plugin)
{
	g_return_if_fail (glade_app_get_project());
	glade_app_command_redo ();
}

static void
on_glade_layout_cut (GtkAction* action, GladePlugin* plugin)
{
	glade_app_command_cut ();
}

static void
on_glade_layout_copy (GtkAction* action, GladePlugin* plugin)
{
	glade_app_command_copy ();
}

static void
on_glade_layout_paste (GtkAction* action, GladePlugin* plugin)
{
	glade_app_command_paste (NULL);
}

static void
on_glade_layout_delete (GtkAction* action, GladePlugin* plugin)
{
	glade_app_command_delete ();
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_glade[] =
{
	{
		"ActionMenuGlade",   /* Action name */
		NULL,                /* Stock icon, if any */
		N_("_Glade"),        /* Display label */
		NULL,                /* short-cut */
		NULL,                /* Tooltip */
		NULL                 /* action callback */
	},
	{
		"ActionGladeSwitchDesigner",
		NULL,
		N_("Switch designer/code"),
		"F12",
		N_("Switch designer/code"),
		G_CALLBACK (on_switch_designer_and_editor)
	},
	{
		"ActionInsertHandlerStub",
		NULL,
		N_("Insert handler stub"),
		NULL,
		N_("Insert handler stub"),
		G_CALLBACK (on_insert_handler_stub_manual)
	},
	{
		"ActionInsertAutoHandlerStub",
		NULL,
		N_("Insert handler stub, autoposition"),
		NULL,
		N_("Insert handler stub, autoposition"),
		G_CALLBACK (on_insert_handler_stub_auto)
	},
	{
		"ActionAssociateDesignerAndEditor",
		NULL,
		N_("Associate last designer and last editor"),
		NULL,
		N_("Associate last designer and editor"),
		G_CALLBACK (on_associate_designer_and_editor)
	},
	{
		"ActionGladeAssociationsDialog",
		NULL,
		N_("Associations dialog..."),
		NULL,
		N_("Associations dialog..."),
		G_CALLBACK (on_show_associations_dialog)
	},
	{
		"ActionVersionDialog",
		GTK_STOCK_PROPERTIES,
		N_("Versioning..."),
		NULL,
		N_("Switch between library versions and check deprecations"),
		G_CALLBACK (on_glade_show_version_dialog)
	},
	{
		"ActionSetDefaultTarget",
		NULL,
		N_("Set as default resource target"),
		NULL,
		N_("Set as default resource target"),
		G_CALLBACK (on_set_default_resource_target)
	},
	{
		"ActionDefaultTarget",
		NULL,
		"",
		NULL,
		N_("Current default target"),
		NULL
	},
	{
		"ActionGladeClose",
		GTK_STOCK_CLOSE,
		N_("Close"),
		NULL,
		N_("Close the current file"),
		G_CALLBACK (on_glade_layout_close)
	},
	{
		"ActionGladeSave",
		GTK_STOCK_SAVE,
		N_("Save"),
		NULL,
		N_("Save the current file"),
		G_CALLBACK (on_glade_layout_save)
	},
	{
		"ActionGladeUndo",
		GTK_STOCK_UNDO,
		N_("Undo"),
		NULL,
		N_("Undo the last action"),
		G_CALLBACK (on_glade_layout_undo)
	},
	{
		"ActionGladeRedo",
		GTK_STOCK_REDO,
		N_("Redo"),
		NULL,
		N_("Redo the last action"),
		G_CALLBACK (on_glade_layout_redo)
	},
	{
		"ActionGladeCut",
		GTK_STOCK_CUT,
		N_("Cut"),
		NULL,
		N_("Cut the selection"),
		G_CALLBACK (on_glade_layout_cut)
	},
	{
		"ActionGladeCopy",
		GTK_STOCK_COPY,
		N_("Copy"),
		NULL,
		N_("Copy the selection"),
		G_CALLBACK (on_glade_layout_copy)
	},
	{
		"ActionGladePaste",
		GTK_STOCK_PASTE,
		N_("Paste"),
		NULL,
		N_("Paste the clipboard"),
		G_CALLBACK (on_glade_layout_paste)
	},
	{
		"ActionGladeDelete",
		GTK_STOCK_DELETE,
		N_("Delete"),
		NULL,
		N_("Delete the selection"),
		G_CALLBACK (on_glade_layout_delete)
	}
};

static GtkWidget *
create_toolbar (GtkUIManager *ui, GladePlugin *plugin)
{
	return gtk_ui_manager_get_widget (ui, "/GladeDesignLayoutToolBar");
}

#ifdef GLADE_SIGNAL_EDITOR_EXT

static void
on_gse_week_ref (gpointer data, GObject *where_the_object_was)
{
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (data);

	if (plugin->priv->last_gse == GLADE_SIGNAL_EDITOR (where_the_object_was))
	{
		plugin->priv->last_gse = glade_app_get_editor()->signal_editor;
	}
}

static void
on_gse_selection_changed (GtkTreeSelection *treeselection,
                          gpointer          user_data)
{
	gpointer *signal_data = user_data;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (signal_data[0]);
	plugin->priv->last_gse = GLADE_SIGNAL_EDITOR (signal_data[1]);
}

static gboolean
on_gse_focus_in (GtkWidget     *widget,
                 GdkEventFocus *event,
                 gpointer       user_data)
{
	gpointer *signal_data = user_data;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (signal_data[0]);
	plugin->priv->last_gse = GLADE_SIGNAL_EDITOR (signal_data[1]);
	return FALSE;
}

static void
on_gse_signal_data_free (gpointer data, GClosure *closure)
{
	g_slice_free1 (sizeof(gpointer)*2, data);
}

static void
on_gse_created (GladeApp *app, GladeSignalEditor *gse, gpointer data)
{
	GtkCellRenderer *renderer;
	GtkTreeModel *completion;

	completion = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	renderer = gtk_cell_renderer_combo_new ();
	g_object_set (G_OBJECT (renderer),
	             "model", completion,
	             "text-column", 0,
	              NULL);

	g_object_set (G_OBJECT (gse), "handler-completion", completion,
	                              "handler-renderer", renderer,
	                               NULL);
}

static void
on_gse_created_after (GladeApp *app, GladeSignalEditor *gse, gpointer data)
{
	GtkTreeView *tree_view;
	GtkTreeSelection *selection;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (data);
	GladePluginPriv *priv = plugin->priv;
	gpointer *signal_data;

	if (plugin->priv->last_gse == NULL)
		plugin->priv->last_gse = gse;

	g_signal_connect (G_OBJECT (gse),
	                 "handler-editing-done",
	                  G_CALLBACK (on_handler_editing_done),
	                  plugin);
	g_signal_connect (G_OBJECT (gse),
	                 "handler-editing-started",
	                  G_CALLBACK (on_handler_editing_started),
	                  plugin);
	g_signal_connect (G_OBJECT (gse),
	                 "userdata-editing-started",
	                  G_CALLBACK (glade_signal_editor_userdata_editing_started_default_impl),
	                  plugin);

	tree_view = GTK_TREE_VIEW (gse->signals_list);
	selection = gtk_tree_view_get_selection (tree_view);
	g_signal_connect (G_OBJECT (tree_view),
	                 "row-activated",
	                  G_CALLBACK (on_signal_row_activated),
	                  plugin);
	signal_data = g_slice_alloc (sizeof(gpointer)*2);
	signal_data[0] = plugin;
	signal_data[1] = gse;
	g_signal_connect_data (G_OBJECT (selection),
	                      "changed",
	                       G_CALLBACK (on_gse_selection_changed),
	                       signal_data,
	                       on_gse_signal_data_free,
	                       0);
	/* Don't free the "signal_data" twice */
	g_signal_connect_data (G_OBJECT (tree_view),
	                      "focus-in-event",
	                       G_CALLBACK (on_gse_focus_in),
	                       signal_data,
	                       NULL,
	                       0);

	g_object_weak_ref (G_OBJECT (gse), on_gse_week_ref, plugin);
	priv->gse_list = g_list_prepend (priv->gse_list, gse);
}

static void
disconnect_glade_signal_editors (GladePlugin *plugin)
{
	GladeSignalEditor *gse;
	GtkTreeView *tree_view;
	GtkTreeSelection *selection;

	while (plugin->priv->gse_list)
	{
		gse = GLADE_SIGNAL_EDITOR (plugin->priv->gse_list->data);
		g_object_weak_unref (G_OBJECT (gse), on_gse_week_ref, plugin);
		plugin->priv->gse_list =
			g_list_delete_link (plugin->priv->gse_list,
			                    plugin->priv->gse_list);

		on_gse_week_ref (plugin, G_OBJECT (gse));

		g_signal_handlers_disconnect_by_func (G_OBJECT (gse),
		                                      G_CALLBACK (on_handler_editing_started), plugin);
		g_signal_handlers_disconnect_by_func (G_OBJECT (gse),
		                                      G_CALLBACK (on_handler_editing_done), plugin);

		tree_view = GTK_TREE_VIEW (gse->signals_list);
		selection = gtk_tree_view_get_selection (tree_view);

		g_signal_handlers_disconnect_by_func (G_OBJECT (tree_view),
		                                      G_CALLBACK (on_signal_row_activated), plugin);
		g_signal_handlers_disconnect_by_func (G_OBJECT (selection),
		                                      G_CALLBACK (on_gse_selection_changed), plugin);
	}
}
#endif

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GladePlugin *glade_plugin;
	GladePluginPriv *priv;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkAction *action;

	DEBUG_PRINT ("%s", "GladePlugin: Activating Glade plugin...");

	glade_plugin = ANJUTA_PLUGIN_GLADE (plugin);

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	priv = glade_plugin->priv;
	priv->deactivating = FALSE;

	priv->separated_designer_layout = FALSE;

	priv->dialog_data = g_new0 (AssociationsDialogData, 1);

	/* Add actions */
	priv->action_group =
		anjuta_ui_add_action_group_entries (ui,
			"ActionGroupGlade", _("Glade designer operations"),
			 actions_glade, G_N_ELEMENTS (actions_glade),
			 GETTEXT_PACKAGE, TRUE, plugin);
	priv->uiid = anjuta_ui_merge (ui, UI_FILE);

	register_stock_icons (plugin);

	if (!priv->gpw)
	{
		priv->gpw = g_object_new(GLADE_TYPE_APP, NULL);

		glade_app_set_window (GTK_WIDGET (ANJUTA_PLUGIN(plugin)->shell));
		glade_app_set_transient_parent (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell));

		/* Create a view for us */
		priv->view_box = gtk_vbox_new (FALSE, 0);
		store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING,
									G_TYPE_POINTER, NULL);

		priv->projects_combo = gtk_combo_box_new ();
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->projects_combo),
									renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->projects_combo),
										renderer, "text", NAME_COL, NULL);
		gtk_combo_box_set_model (GTK_COMBO_BOX (priv->projects_combo),
								 GTK_TREE_MODEL (store));
		g_object_unref (G_OBJECT (store));
		gtk_box_pack_start (GTK_BOX (priv->view_box), priv->projects_combo,
							FALSE, FALSE, 0);

#if (GLADEUI_VERSION >= 330)
        priv->inspector = glade_inspector_new ();

        g_signal_connect (priv->inspector, "item-activated",
        				  G_CALLBACK (inspector_item_activated_cb),
        				  plugin);
#else
		priv->inspector = glade_project_view_new ();
#endif

#if (GLADEUI_VERSION < 330)
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->inspector),
										GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		glade_app_add_project_view (GLADE_PROJECT_VIEW (priv->inspector));
#endif
		gtk_box_pack_start (GTK_BOX (priv->view_box), GTK_WIDGET (priv->inspector),
							TRUE, TRUE, 0);

		gtk_widget_show_all (priv->view_box);
		gtk_notebook_set_scrollable (GTK_NOTEBOOK (glade_app_get_editor ()->notebook),
									 TRUE);
		gtk_notebook_popup_enable (GTK_NOTEBOOK (glade_app_get_editor ()->notebook));
	}

#ifdef GLADE_SIGNAL_EDITOR_EXT
	g_signal_connect       (priv->gpw, "signal-editor-created",
	                        G_CALLBACK (on_gse_created), glade_plugin);
	g_signal_connect_after (priv->gpw, "signal-editor-created",
	                        G_CALLBACK (on_gse_created_after), glade_plugin);
	glade_editor_set_signal_editor (glade_app_get_editor(),
	                                glade_signal_editor_new ((gpointer) glade_app_get_editor()));
#endif

	g_signal_connect(G_OBJECT(plugin->shell), "destroy",
					 G_CALLBACK(on_shell_destroy), plugin);

	g_signal_connect (G_OBJECT (priv->projects_combo), "changed",
					  G_CALLBACK (on_glade_project_changed), plugin);
	g_signal_connect (G_OBJECT (priv->gpw), "update-ui",
					  G_CALLBACK (glade_update_ui), plugin);

	g_signal_connect(G_OBJECT(glade_app_get_editor()), "gtk-doc-search",
					 G_CALLBACK(on_api_help), plugin);

	/* FIXME: Glade doesn't want to die these widget, so
	 * hold a permenent refs on them
	 */
	g_object_ref (glade_app_get_palette ());
	g_object_ref (glade_app_get_editor ());
	g_object_ref (priv->view_box);

	gtk_widget_show (GTK_WIDGET (glade_app_get_palette ()));
	gtk_widget_show (GTK_WIDGET (glade_app_get_editor ()));

	/* Add widgets */
	priv->designer_layout_box = gtk_event_box_new();
	gtk_widget_set_events (priv->designer_layout_box,
	                       gtk_widget_get_events (priv->designer_layout_box) | GDK_ENTER_NOTIFY);

	priv->designer_layout_box_child = gtk_vbox_new (FALSE, 2);
	priv->new_container = gtk_notebook_new();
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (priv->new_container),
	                             TRUE);

	priv->designer_toolbar = create_toolbar (GTK_UI_MANAGER (ui), glade_plugin);
	gtk_box_pack_start (GTK_BOX (priv->designer_layout_box_child),
	                    priv->designer_toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->designer_layout_box_child),
	                    priv->new_container, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (priv->designer_layout_box),
	                   priv->designer_layout_box_child);
	gtk_widget_show_all (priv->designer_layout_box);

	priv->button_undo = gtk_menu_tool_button_new_from_stock (GTK_STOCK_UNDO);
	priv->button_redo = gtk_menu_tool_button_new_from_stock (GTK_STOCK_REDO);
	gtk_widget_show (GTK_WIDGET (priv->button_undo));
	gtk_widget_show (GTK_WIDGET (priv->button_redo));
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (priv->button_undo),
	                                             _("Go back in undo history"));
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (priv->button_redo),
	                                             _("Go forward in undo history"));
	/* FIXME: hardcoded buttons position */
	gtk_toolbar_insert (GTK_TOOLBAR (priv->designer_toolbar), GTK_TOOL_ITEM (priv->button_undo), 6);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->designer_toolbar), GTK_TOOL_ITEM (priv->button_redo), 7);

	action = gtk_action_group_get_action (priv->action_group, "ActionGladeUndo");
	gtk_action_connect_proxy (action, GTK_WIDGET (priv->button_undo));

	action = gtk_action_group_get_action (priv->action_group, "ActionGladeRedo");
	gtk_action_connect_proxy (action, GTK_WIDGET (priv->button_redo));

	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 priv->designer_layout_box,
							 "AnjutaGladeDesignerLayout", _("Designer"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	if (!GTK_IS_WINDOW (glade_app_get_clipboard_view()))
		anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
								 GTK_WIDGET (glade_app_get_clipboard_view()),
								 "AnjutaGladeClipboard", _("Glade Clipboard"),
								 "glade-plugin-icon",
								 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (priv->view_box),
							 "AnjutaGladeTree", _("Widgets"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (glade_app_get_palette ()),
							 "AnjutaGladePalette", _("Palette"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (glade_app_get_editor ()),
							 "AnjutaGladeEditor", _("Properties"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

	priv->project_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
		                         value_added_project_root_uri,
		                         value_removed_project_root_uri, NULL);
	/* nonzero default value */
	glade_plugin->priv->insert_handler_on_edit = TRUE;
	glade_plugin->priv->auto_add_resource = TRUE;

#ifndef GLADE_SIGNAL_EDITOR_EXT
	priv->last_gse = glade_app_get_editor()->signal_editor;
	g_signal_connect (G_OBJECT (GTK_TREE_VIEW (priv->last_gse->signals_list)),
	                 "row-activated",
	                  G_CALLBACK (on_signal_row_activated),
	                  plugin);
#endif

	/* Load associations and preferences */
	glade_plugin_load_associations (glade_plugin);

	/* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session",
					  G_CALLBACK (on_session_save), plugin);
	g_signal_connect (G_OBJECT (plugin->shell), "load_session",
					  G_CALLBACK (on_session_load), plugin);

	priv->pm_current_uri_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_PROJECT_MANAGER_CURRENT_URI,
		                         value_added_pm_current_uri,
		                         value_removed_pm_current_uri, NULL);
	value_removed_pm_current_uri (plugin, NULL, NULL);
	/* Watch documents */
	glade_plugin->priv->editor_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor, NULL);

	return TRUE;
}

static void
glade_close_all (AnjutaPlugin *plugin)
{
	GList *docwids, *node;
	IAnjutaDocumentManager *docman;

	docman = anjuta_shell_get_interface (plugin->shell,
	                                     IAnjutaDocumentManager, NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		DEBUG_PRINT ("Closing all designer documents");
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				ianjuta_document_manager_remove_document (docman,
				                                          IANJUTA_DOCUMENT(node->data),
				                                          FALSE, NULL);
			}
		}
		g_list_free (docwids);
	}
	else
		DEBUG_PRINT ("No designer documents opened");
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	GladePluginPriv *priv;
	AnjutaUI *ui;
	GladePlugin *glade_plugin;

	glade_plugin =  ANJUTA_PLUGIN_GLADE (plugin);
	priv = glade_plugin->priv;
	if (priv->deactivating)
		return TRUE;
	priv->deactivating = TRUE;

	DEBUG_PRINT ("%s", "GladePlugin: Dectivating Glade plugin...");

	anjuta_plugin_remove_watch (plugin, priv->editor_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, priv->pm_current_uri_watch_id, FALSE);
	priv->editor_watch_id = 0;
	priv->pm_current_uri_watch_id = 0;

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_shell_destroy),
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_save), plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
	                                      G_CALLBACK (on_session_load), plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->projects_combo),
										  G_CALLBACK (on_glade_project_changed),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->gpw),
										  G_CALLBACK (glade_update_ui),
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT(glade_app_get_editor()),
										  G_CALLBACK(on_api_help), plugin);
#ifdef GLADE_SIGNAL_EDITOR_EXT
	disconnect_glade_signal_editors (glade_plugin);
#else
	g_signal_handlers_disconnect_by_func (G_OBJECT(GTK_TREE_VIEW(priv->last_gse->signals_list)),
	                                      G_CALLBACK (on_signal_row_activated), plugin);
#endif

	/* Remove widgets */
	anjuta_shell_remove_widget (plugin->shell,
								priv->designer_layout_box,
								NULL);
	if (!GTK_IS_WINDOW (glade_app_get_clipboard_view()))
		anjuta_shell_remove_widget (plugin->shell,
		                            GTK_WIDGET (glade_app_get_clipboard_view()),
		                            NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_palette ()),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_editor ()),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (priv->view_box),
								NULL);
	/* FIXME: Don't destroy glade, since it doesn't want to */
	/* g_object_unref (G_OBJECT (priv->gpw)); */
	/* priv->gpw = NULL */

	glade_close_all (plugin);

	/* Save associations */
	glade_plugin_save_associations (glade_plugin);

	anjuta_plugin_remove_watch (plugin, priv->project_watch_id, TRUE);
	priv->project_watch_id = 0;

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, priv->uiid);
	anjuta_ui_remove_action_group (ui, priv->action_group);
	priv->uiid = 0;
	priv->action_group = NULL;

	forget_last_signal (glade_plugin);
	if (priv->last_designer)
	{
		g_object_unref (priv->last_designer);
		priv->last_designer = NULL;
	}
	if (priv->last_editor)
	{
		g_object_unref (priv->last_editor);
		priv->last_editor = NULL;
	}
	g_free (priv->default_resource_target);
	priv->default_resource_target = NULL;

	if (priv->xml)
	{
		g_object_unref (priv->xml);
		priv->xml = NULL;
	}

	g_free (priv->dialog_data);
	priv->dialog_data = NULL;

	return TRUE;
}

static void
glade_plugin_dispose (GObject *obj)
{
	/* GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj); */

	/* FIXME: Glade widgets should be destroyed */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
glade_plugin_finalize (GObject *obj)
{
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);
	g_free (plugin->priv);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
glade_plugin_instance_init (GObject *obj)
{
	GladePluginPriv *priv;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);

	plugin->priv = (GladePluginPriv *) g_new0 (GladePluginPriv, 1);
	priv = plugin->priv;
	priv->destroying = FALSE;

	DEBUG_PRINT ("%s", "Intializing Glade plugin");
}

static void
glade_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = glade_plugin_dispose;
	klass->finalize = glade_plugin_finalize;
}

gchar* glade_get_filename(GladePlugin *plugin)
{
#if (GLADEUI_VERSION >= 330)
	return glade_project_get_name(glade_app_get_project());
#else
	GladeProject* project = glade_app_get_project();
	if (project)
		return project->name;
	else
		return NULL;
#endif
}

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **err)
{
	GladePluginPriv *priv;
	GladeProject *project;
	gchar *filename;
	IAnjutaDocumentManager* docman;
	GList *glade_obj_node;
	const gchar *project_name;
	AnjutaDesignDocument *doc;

	g_return_if_fail (file != NULL);

	priv = ANJUTA_PLUGIN_GLADE (ifile)->priv;

	filename = g_file_get_path (file);
	if (!filename)
	{
		gchar* uri = g_file_get_parse_name(file);
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								    _("Not local file: %s"), uri);
		g_free (uri);
		return;
	}

	docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(ifile)->shell, IAnjutaDocumentManager,
										NULL);
	doc = find_designer_by_file (docman, file);
	if (doc)
	{
		project = get_project_from_design_document (doc);
		glade_app_set_project (project);
		return;
	}

#if (GLADEUI_VERSION >= 330)
	project = glade_project_load (filename);
#else
	project = glade_project_open (filename);
#endif
	g_free (filename);
	if (!project)
	{
		gchar* name = g_file_get_parse_name (file);
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								    _("Could not open %s"), name);
		g_free (name);
		return;
	}
#if (GLADEUI_VERSION >= 330)
	project_name = glade_project_get_name(project);
#else
	project_name = project->name;
#endif
	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (ifile), project, project_name);

	/* Select the first window in the project */
	for (glade_obj_node = (GList *) glade_project_get_objects (project);
		 glade_obj_node != NULL;
		 glade_obj_node = g_list_next (glade_obj_node))
	{
		GObject *glade_obj = G_OBJECT (glade_obj_node->data);
		if (GTK_IS_WINDOW (glade_obj))
		{
			glade_widget_show (glade_widget_get_from_gobject (glade_obj));
		}
		break;
	}
	anjuta_shell_present_widget (ANJUTA_PLUGIN (ifile)->shell, priv->view_box, NULL);
}

static GFile*
ifile_get_file (IAnjutaFile* ifile, GError** e)
{
#if (GLADEUI_VERSION >= 330)
	const gchar* path = glade_project_get_path(glade_app_get_project());
	GFile* file = g_file_new_for_path (path);
	return file;
#else
	GladeProject* project = glade_app_get_project();
	if (project && project->path)
		return g_file_new_for_path(project->path);
	else
		return NULL;
#endif
}

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

static void
iwizard_activate (IAnjutaWizard *iwizard, GError **err)
{
	GladePluginPriv *priv;
	GladeProject *project;
	GtkListStore *store;
	GtkTreeIter iter;
	const gchar *project_name;

	priv = ANJUTA_PLUGIN_GLADE (iwizard)->priv;

#if (GLADEUI_VERSION >= 330)
	project = glade_project_new ();
#else
	project = glade_project_new (TRUE);
#endif
	if (!project)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (iwizard)->shell),
								    _("Could not create a new glade project."));
		return;
	}
	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->projects_combo)));
	gtk_list_store_append (store, &iter);
#if (GLADEUI_VERSION >= 330)
	project_name = glade_project_get_name(project);
#else
	project_name = project->name;
#endif
	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (iwizard), project, project_name);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (iwizard)->shell,
				     GTK_WIDGET (glade_app_get_palette ()), NULL);
}

static void
iwizard_iface_init(IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

#define PREFERENCES_PAGE_NAME "preferences_page"
#define HANDLER_TEMPLATE_BUTTON0_NAME "handler_template_button0"
#define HANDLER_TEMPLATE_BUTTON1_NAME "handler_template_button1"
#define INSERT_HANDLER_ON_EDIT_NAME "insert_handler_on_edit"
#define AUTO_ADD_RESOURCE_NAME "auto_add_resource"
#define DEFAULT_RESOURCE_ENTRY_NAME "default_resource_entry"
#define SEPARATED_DESIGNER_LAYOUT_NAME "separated_designer_layout"

gboolean
on_preferences_default_resource_entry_focus_out (GtkWidget *widget,
                                                 GdkEventFocus *event,
                                                 GladePlugin *plugin);
void
on_preferences_default_resource_entry_activate (GtkEntry *entry, GladePlugin *plugin);
void
on_set_default_data_signal_template0 (GtkToggleButton *button,
                                      GladePlugin* plugin);
void
on_set_default_data_signal_template1 (GtkToggleButton *button,
                                      GladePlugin* plugin);
void
on_insert_handler_on_edit_toggled (GtkToggleButton *button,
                                   GladePlugin* plugin);
void
on_auto_add_resource_toggled (GtkToggleButton *button,
                                GladePlugin* plugin);
void
on_separated_designer_layout_toggled (GtkToggleButton *button,
                                      GladePlugin* plugin);

void
on_set_default_data_signal_template0 (GtkToggleButton *button,
                                      GladePlugin* plugin)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON(button));
	if (!plugin->priv->prefs)
		return;

	if (gtk_toggle_button_get_active (button))
		plugin->priv->default_handler_template = 0;
}

void
on_set_default_data_signal_template1 (GtkToggleButton *button,
                                      GladePlugin* plugin)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON(button));
	if (!plugin->priv->prefs)
		return;

	if (gtk_toggle_button_get_active (button))
		plugin->priv->default_handler_template = 1;
}

void
on_insert_handler_on_edit_toggled (GtkToggleButton *button,
                                   GladePlugin* plugin)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON(button));
	if (!plugin->priv->prefs)
		return;

	plugin->priv->insert_handler_on_edit =
		gtk_toggle_button_get_active (button);
}

void
on_auto_add_resource_toggled (GtkToggleButton *button,
                                GladePlugin* plugin)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON(button));
	if (!plugin->priv->prefs)
		return;

	plugin->priv->auto_add_resource =
		gtk_toggle_button_get_active (button);
}

void
on_separated_designer_layout_toggled (GtkToggleButton *button,
                                      GladePlugin* plugin)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON(button));
	if (!plugin->priv->prefs)
		return;

	update_separated_designer_layout (gtk_toggle_button_get_active (button),
	                                  plugin);
}

static void
on_insert_handler_on_edit_changed (GladePlugin* plugin)
{
	GtkToggleButton *button;

	if (!plugin->priv->prefs)
		return;
	g_return_if_fail (plugin->priv->xml);

	button = GTK_TOGGLE_BUTTON(gtk_builder_get_object (plugin->priv->xml,
	                                                   INSERT_HANDLER_ON_EDIT_NAME));
	gtk_toggle_button_set_active (button, plugin->priv->insert_handler_on_edit);
}

static void
on_auto_add_resource_changed (GladePlugin* plugin)
{
	GtkToggleButton *button;

	if (!plugin->priv->prefs)
		return;
	g_return_if_fail (plugin->priv->xml);

	button = GTK_TOGGLE_BUTTON(gtk_builder_get_object (plugin->priv->xml,
	                                                   AUTO_ADD_RESOURCE_NAME));
	gtk_toggle_button_set_active (button, plugin->priv->auto_add_resource);
}

static void
on_default_handler_template_changed (GladePlugin* plugin)
{
	GtkToggleButton *button = NULL;

	if (!plugin->priv->prefs)
		return;
	g_return_if_fail (plugin->priv->xml);

	switch (plugin->priv->default_handler_template)
	{
	case 0:
		button = GTK_TOGGLE_BUTTON(gtk_builder_get_object (plugin->priv->xml,
		                                                   HANDLER_TEMPLATE_BUTTON0_NAME));
		break;
	case 1:
		button = GTK_TOGGLE_BUTTON(gtk_builder_get_object (plugin->priv->xml,
		                                                   HANDLER_TEMPLATE_BUTTON1_NAME));
		break;
	}
	if (button)
		gtk_toggle_button_set_active (button, TRUE);
}

static void
on_separated_designer_layout_changed (GladePlugin *plugin)
{
	GtkToggleButton *button;

	if (!plugin->priv->prefs)
		return;
	g_return_if_fail (plugin->priv->xml);

	button = GTK_TOGGLE_BUTTON(gtk_builder_get_object (plugin->priv->xml,
	                                                   SEPARATED_DESIGNER_LAYOUT_NAME));
	gtk_toggle_button_set_active (button, plugin->priv->separated_designer_layout);
}

static void
update_prefs_page (GladePlugin *plugin)
{
	on_insert_handler_on_edit_changed (plugin);
	on_auto_add_resource_changed (plugin);
	on_default_handler_template_changed (plugin);
	on_separated_designer_layout_changed (plugin);
}

static void
remove_widget_from_parent (GtkWidget *parent, GtkWidget *widget)
{
	g_return_if_fail (parent);

	if (GTK_IS_NOTEBOOK (parent))
	{
		gint page_num;

		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (parent), widget);
		gtk_notebook_remove_page (GTK_NOTEBOOK (parent), page_num);
	}
	else
	{
		gtk_container_remove (GTK_CONTAINER (parent), widget);
	}
}

static void
glade_plugin_preferences_add_page (AnjutaPreferences* pr, GtkWidget *page,
                                   const gchar* page_widget_name,
                                   const gchar* title,
                                   const gchar *icon_filename)
{
	GtkWidget *parent;
	GdkPixbuf *pixbuf;

	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (page_widget_name != NULL);
	g_return_if_fail (icon_filename != NULL);

	g_object_ref (page);
	parent = gtk_widget_get_parent (page);
	if (parent && GTK_IS_CONTAINER (parent))
		remove_widget_from_parent (parent, page);
	pixbuf = gdk_pixbuf_new_from_file (icon_filename, NULL);
	anjuta_preferences_dialog_add_page (ANJUTA_PREFERENCES_DIALOG(anjuta_preferences_get_dialog (pr)),
	                                    page_widget_name, title, pixbuf, page);
	g_object_unref (page);
	g_object_unref (pixbuf);
}

gboolean
on_preferences_default_resource_entry_focus_out (GtkWidget *entry,
                                                 GdkEventFocus *event,
                                                 GladePlugin *plugin)
{
	const gchar *value;

	g_return_val_if_fail (plugin->priv->prefs, FALSE);
	value = gtk_entry_get_text (GTK_ENTRY(entry));
	set_default_resource_target (value, plugin);

	return FALSE;
}

void
on_preferences_default_resource_entry_activate (GtkEntry *entry, GladePlugin *plugin)
{
	const gchar *value;

	g_return_if_fail (plugin->priv->prefs);

	value = gtk_entry_get_text (entry);
	set_default_resource_target (value, plugin);
}

static void
on_default_resource_target_changed (const gchar *value, GladePlugin *plugin)
{
	GtkEntry *entry;

	if (!plugin->priv->prefs)
		return;

	entry = GTK_ENTRY(gtk_builder_get_object (plugin->priv->xml,
	                                          DEFAULT_RESOURCE_ENTRY_NAME));
	gtk_entry_set_text (entry, value ? value : "");
}

static void
ipreferences_merge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GladePlugin* plugin = ANJUTA_PLUGIN_GLADE (ipref);
	GtkWidget *page;

	g_return_if_fail (glade_plugin_get_glade_xml (plugin));
	plugin->priv->prefs = page =
		GTK_WIDGET (gtk_builder_get_object (plugin->priv->xml, PREFERENCES_PAGE_NAME));
	glade_plugin_preferences_add_page (prefs, page, "Glade",
	                                   _("Glade GUI Designer"),
	                                   PACKAGE_PIXMAPS_DIR"/"ICON_FILE);
	gtk_widget_show_all (page);
	g_object_ref (page);

	on_default_handler_template_changed (plugin);
	on_insert_handler_on_edit_changed (plugin);
	on_auto_add_resource_changed (plugin);
	on_default_resource_target_changed (plugin->priv->default_resource_target, plugin);
	on_separated_designer_layout_changed (plugin);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *page;
	GladePlugin* plugin = ANJUTA_PLUGIN_GLADE (ipref);

	g_return_if_fail (plugin->priv->prefs);
	page = plugin->priv->prefs;
	/* Stops watching for preferences */
	plugin->priv->prefs = NULL;

	remove_widget_from_parent (gtk_widget_get_parent (page), page);

	anjuta_preferences_remove_page(prefs, _("Glade GUI Designer"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (GladePlugin, glade_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GladePlugin, glade_plugin);
