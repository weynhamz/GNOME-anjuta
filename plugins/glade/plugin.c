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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>

#if (GLADEUI_VERSION <= 302)
# include <glade.h>
#else
# if (GLADEUI_VERSION <= 314)
#   include <glade.h>
#   include <glade-design-view.h>
# else /* Since 3.1.5 */
#   include <gladeui/glade.h>
#   include <gladeui/glade-design-view.h>
# endif
#endif

#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-help.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-glade.ui"

static gpointer parent_class;

/* This is variable required from libdevhelp */
gchar *geometry = NULL;

struct _GladePluginPriv
{
	gint uiid;
	GtkActionGroup *action_group;
	GladeApp  *gpw;
	GtkWidget *inspector;
#if (GLADEUI_VERSION > 302)
	GtkWidget *design_notebook;
#endif
	GtkWidget *view_box;
	GtkWidget *projects_combo;
};

enum {
	NAME_COL,
	PROJECT_COL,
	N_COLUMNS
};

static void
glade_update_ui (GladeApp *app, GladePlugin *plugin)
{
#if (GLADEUI_VERSION >= 330)
	GladeCommand *undo_item;
	GladeCommand *redo_item;
#else
	GList *prev_redo_item;
	GList *undo_item;
	GList *redo_item;
#endif
	const gchar *undo_description = NULL;
	const gchar *redo_description = NULL;
	GladeProject *project;
	GtkAction *action;
	gchar buffer[512];
	AnjutaUI *ui;
	GtkTreeModel *model;
	GtkTreeIter iter;;

	project = glade_app_get_project ();
	if (!project)
	{
		undo_item = NULL;
		redo_item = NULL;
	}
	else
	{
#if (GLADEUI_VERSION >= 330)
		undo_item = glade_project_next_undo_item(project);
		redo_item = glade_project_next_redo_item(project);
		if (undo_item)
			undo_description = undo_item->description;
		if (redo_item)
			redo_description = redo_item->description;
#else
		undo_item = prev_redo_item = project->prev_redo_item;
		redo_item = (prev_redo_item == NULL) ? project->undo_stack : prev_redo_item->next;
		if (undo_item && undo_item->data)
			undo_description = GLADE_COMMAND (undo_item->data)->description;
		if (redo_item && redo_item->data)
			redo_description = GLADE_COMMAND (redo_item->data)->description;
#endif
	}
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupGlade", "ActionGladeUndo");
	if (undo_description)
	{
		snprintf (buffer, 512, _("_Undo: %s"), undo_description);
		g_object_set (G_OBJECT (action), "sensitive", TRUE, "label", buffer, NULL);
	}
	else
	{
		g_object_set (G_OBJECT (action), "sensitive", FALSE, "label", _("_Undo"), NULL);
	}
	action = anjuta_ui_get_action (ui, "ActionGroupGlade", "ActionGladeRedo");
	if (redo_description)
	{
		snprintf (buffer, 512, _("_Redo: %s"), redo_description);
		g_object_set (G_OBJECT (action), "sensitive", TRUE, "label", buffer, NULL);
	}
	else
	{
		g_object_set (G_OBJECT (action), "sensitive", FALSE, "label", _("_Redo"), NULL);
	}
	
	/* Update current project */
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			GladeProject *project;
			gtk_tree_model_get (model, &iter, PROJECT_COL, &project, -1);
			if (project == glade_app_get_project ())
			{
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (plugin->priv->projects_combo), &iter);
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}
}

/* Action callbacks */
static void
on_copy_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_copy ();
}

static void
on_cut_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_cut ();
}

static void
on_paste_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_paste (NULL);
}

static void
on_delete_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_delete ();
}

static void
on_undo_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_undo ();
}

static void
on_redo_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_redo ();
}

static void
on_show_clipbard_activated (GtkAction *action, GladePlugin *plugin)
{
	GtkWidget *clipboard_view;
	
	clipboard_view = glade_app_get_clipboard_view ();
	gtk_widget_show_all (clipboard_view);
	gtk_window_present (GTK_WINDOW (clipboard_view));
}

static void
glade_save (GladePlugin *plugin, GladeProject *project, const gchar *path)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN(plugin)->shell, NULL);
	if (!glade_project_save (project, path, NULL))
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
									_("Invalid glade file name"));
		return;
	}
	// FIXME: gpw_refresh_project_entry (gpw, project);
#if (GLADEUI_VERSION >= 330)
	anjuta_status_set (status, _("Glade project '%s' saved"),
					   glade_project_get_name(project));
#else
	anjuta_status_set (status, _("Glade project '%s' saved"), project->name);
#endif
}

static void
on_save_activated (GtkAction *action, GladePlugin *plugin)
{
	GladeProject *project;
	GtkWidget *filechooser;
	gchar *path = NULL;

	project = glade_app_get_project ();

#if (GLADEUI_VERSION >= 330)
	if (glade_project_get_path(project) != NULL) {
#else
	if (project->path != NULL) {
#endif
		AnjutaStatus *status;
		
		status = anjuta_shell_get_status (ANJUTA_PLUGIN(plugin)->shell, NULL);
		
#if (GLADEUI_VERSION >= 330)
		if (glade_project_save (project, glade_project_get_path(project),
								NULL)) {
			anjuta_status_set (status, _("Glade project '%s' saved"),
							   glade_project_get_name(project));
#else	
		if (glade_project_save (project, project->path, NULL)) {
			anjuta_status_set (status, _("Glade project '%s' saved"),
							   project->name);
#endif
		} else {
			anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
										_("Invalid glade file name"));
		}
		return;
	}

	/* If instead we dont have a path yet, fire up a file selector */
	filechooser = glade_util_file_dialog_new (_("Save glade file..."),
									GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
											  GLADE_FILE_DIALOG_ACTION_SAVE);
	
	if (gtk_dialog_run (GTK_DIALOG(filechooser)) == GTK_RESPONSE_OK)
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));

	gtk_widget_destroy (filechooser);

	if (!path)
		return;

	glade_save (plugin, project, path);
	g_free (path);
}

static void
on_save_as_activated (GtkAction *action, GladePlugin *plugin)
{
	GladeProject *project;
	GtkWidget *filechooser;
	gchar *path = NULL;

	project = glade_app_get_project ();

	filechooser = glade_util_file_dialog_new (_("Save glade file as..."),
									GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
											  GLADE_FILE_DIALOG_ACTION_SAVE);

	if (gtk_dialog_run (GTK_DIALOG(filechooser)) == GTK_RESPONSE_OK)
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));

	gtk_widget_destroy (filechooser);

	if (!path)
		return;

	glade_save (plugin, project, path);
	g_free (path);
}

static gboolean
glade_confirm_close_project (GladePlugin *plugin, GladeProject *project)
{
	GladeApp *gpw;
	GtkWidget *dialog;
	gboolean close = FALSE;
	gchar *msg, *string; 
	gint ret;

	g_return_val_if_fail (GLADE_IS_PROJECT (project), FALSE);

	gpw = plugin->priv->gpw;
	string = g_strdup_printf(
					"<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s", 				
					_("Save changes to glade project \"%s\" before closing?"), 
					_("Your changes will be lost if you don't save them."));
#if (GLADEUI_VERSION >= 330)
	msg = g_strdup_printf (string, glade_project_get_name(project));
#else
	msg = g_strdup_printf (string, project->name);
#endif
	g_free(string);
	dialog = gtk_message_dialog_new (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_NONE,
					 msg);
	g_free(msg);

	gtk_label_set_use_markup (GTK_LABEL (GTK_MESSAGE_DIALOG (dialog)->label), TRUE);

	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				_("_Close without Saving"), GTK_RESPONSE_NO,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_YES, NULL);

	gtk_dialog_set_default_response	(GTK_DIALOG (dialog), GTK_RESPONSE_YES);

	ret = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (ret) {
	case GTK_RESPONSE_YES:
		/* if YES we save the project: note we cannot use gpw_save_cb
		 * since it saves the current project, while the modified 
                 * project we are saving may be not the current one.
		 */
#if (GLADEUI_VERSION >= 330)
		if (glade_project_get_path(project) != NULL)
 		{
			close = glade_project_save (project,
										glade_project_get_path(project),
										NULL);
 		}
#else
		if (project->path != NULL)
		{
			close = glade_project_save (project, project->path, NULL);
		}
#endif
		else
		{
			GtkWidget *filechooser;
			gchar *path = NULL;

			filechooser = glade_util_file_dialog_new (_("Save glade project..."),
									GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								   GLADE_FILE_DIALOG_ACTION_SAVE);

			if (gtk_dialog_run (GTK_DIALOG(filechooser)) == GTK_RESPONSE_OK)
				path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));

			gtk_widget_destroy (filechooser);

			if (!path)
				break;

			glade_save (plugin, project, path);
			g_free (path);
			close = FALSE;
		}
		break;
	case GTK_RESPONSE_NO:
		close = TRUE;
		break;
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_DELETE_EVENT:
		close = FALSE;
		break;
	default:
		g_assert_not_reached ();
		close = FALSE;
	}

	gtk_widget_destroy (dialog);
	return close;
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
#if (GLADEUI_VERSION > 302)
	GtkWidget *design_view;

	design_view = g_object_get_data (G_OBJECT (project), "design_view");
	gtk_notebook_remove_page (GTK_NOTEBOOK (plugin->priv->design_notebook),
							  gtk_notebook_page_num (GTK_NOTEBOOK
													 (plugin->priv->design_notebook),
													 design_view));
	gtk_widget_destroy (design_view);
#endif
	glade_app_remove_project (project);
}

static void
on_close_activated (GtkAction *action, GladePlugin *plugin)
{
	GladeProject *project;
	gboolean close;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	project = glade_app_get_project ();

	if (!project)
	{
		/* if (gtk_tree_model_iter_n_children (model, NULL) <= 0) */
		anjuta_plugin_deactivate (ANJUTA_PLUGIN (plugin));
		return;
	}
	
#if (GLADEUI_VERSION >= 330)
	if (glade_project_get_modified (project))
#else
	if (project->changed)
#endif
	{
		close = glade_confirm_close_project (plugin, project);
			if (!close)
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
	if (gtk_tree_model_iter_n_children (model, NULL) <= 0)
		anjuta_plugin_deactivate (ANJUTA_PLUGIN (plugin));
}

static void
on_glade_project_changed (GtkComboBox *combo, GladePlugin *plugin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_combo_box_get_active_iter (combo, &iter))
	{
		GladeProject *project;
		
#if (GLADEUI_VERSION > 302)
		GtkWidget *design_view;
		gint design_pagenum;
#endif
		gtk_tree_model_get (model, &iter, PROJECT_COL, &project, -1);
		glade_app_set_project (project);
		
#if (GLADEUI_VERSION > 302)
		design_view = g_object_get_data (G_OBJECT (project), "design_view");
		design_pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (plugin->priv->design_notebook),
												design_view);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (plugin->priv->design_notebook),
									   design_pagenum);
#  if (GLADEUI_VERSION >= 330)
        glade_inspector_set_project (GLADE_INSPECTOR (plugin->priv->inspector), project);
#  endif								   
									   
#endif
	}
}

/* Action definitions */
static GtkActionEntry actions[] = {
	/* Go menu */
	{
		"ActionMenuGlade",
		NULL,
		N_("_Glade"),
		NULL,
		NULL,
		NULL
	},
	{
		"ActionGladeUndo",
		GTK_STOCK_UNDO,
		N_("_Undo"),
		NULL,
		N_("Undo last action"),
		G_CALLBACK (on_undo_activated)
	},
	{
		"ActionGladeRedo",
		GTK_STOCK_REDO,
		N_("_Redo"),
		NULL,
		N_("Redo last undone action"),
		G_CALLBACK (on_redo_activated)
	},
	{
		"ActionGladeCut",
		GTK_STOCK_CUT,
		N_("Cu_t"),
		NULL,
		N_("Cut selection"),
		G_CALLBACK (on_cut_activated)
	},
	{
		"ActionGladeCopy",
		GTK_STOCK_COPY,
		N_("_Copy"),
		"",
		N_("Copy selection"),
		G_CALLBACK (on_copy_activated)
	},
	{
		"ActionGladePaste",
		GTK_STOCK_PASTE,
		N_("_Paste"),
		"",
		N_("Paste selection"),
		G_CALLBACK (on_paste_activated)
	},
	{
		"ActionGladeDelete",
		GTK_STOCK_DELETE,
		N_("_Delete"),
		"",
		N_("Delete selection"),
		G_CALLBACK (on_delete_activated)
	},
	{
		"ActionGladeShowClipboard",
		NULL,
		N_("_Show Clipboard"),
		NULL,
		N_("Show clipboard"),
		G_CALLBACK (on_show_clipbard_activated)
	},
	{
		"ActionGladeSave",
		GTK_STOCK_SAVE,
		N_("_Save"),
		"",
		N_("Save glade project"),
		G_CALLBACK (on_save_activated)
	},
	{
		"ActionGladeSaveAs",
		GTK_STOCK_SAVE_AS,
		N_("Save _As..."),
		"",
		N_("Save as glade project"),
		G_CALLBACK (on_save_as_activated)
	},
	{
		"ActionGladeClose",
		GTK_STOCK_CLOSE,
		N_("Clos_e"),
		"",
		N_("Close current glade project"),
		G_CALLBACK (on_close_activated)
	}
};

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (PACKAGE_PIXMAPS_DIR"/anjuta-glade-plugin.png",
				   "glade-plugin-icon");
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, GladePlugin *plugin)
{
	GList *files;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	files = anjuta_session_get_string_list (session, "File Loader", "Files");
	files = g_list_reverse (files);
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			gchar *uri;
			GladeProject *project;
			gtk_tree_model_get (model, &iter, PROJECT_COL, &project, -1);
#if (GLADEUI_VERSION >= 330)
			if (glade_project_get_path(project))
#else
			if (project->path)
#endif
			{
#if (GLADEUI_VERSION >= 330)
				uri = gnome_vfs_get_uri_from_local_path
							(glade_project_get_path(project));
#else
				uri = gnome_vfs_get_uri_from_local_path (project->path);
#endif
				if (uri)
					files = g_list_prepend (files, uri);
				/* URI is not freed here */
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}
	files = g_list_reverse (files);
	anjuta_session_set_string_list (session, "File Loader", "Files", files);
	g_list_foreach (files, (GFunc)g_free, NULL);
	g_list_free (files);
}

/* FIXME: Glade does not allow destroying its (singleton) widgets.
 * Make sure they are removed when the application is destroyed
 * without the plugin unloaded.
 */
static void
on_shell_destroy (AnjutaShell *shell, GladePlugin *glade_plugin)
{
	GtkWidget *wid;
	GtkWidget *parent;
	
	/* Remove widgets before the container destroyes them */
	wid = GTK_WIDGET (glade_app_get_palette ());
	parent = gtk_widget_get_parent (wid);
	gtk_container_remove (GTK_CONTAINER (parent), wid);
	
	wid = GTK_WIDGET (glade_app_get_editor ());
	parent = gtk_widget_get_parent (wid);
	gtk_container_remove (GTK_CONTAINER (parent), wid);
	
	wid = GTK_WIDGET (glade_plugin->priv->view_box);
	parent = gtk_widget_get_parent (wid);
	gtk_container_remove (GTK_CONTAINER (parent), wid);
}

#if (GLADEUI_VERSION > 302)
static void
glade_plugin_add_project (GladePlugin *glade_plugin, GladeProject *project)
{
	GtkWidget *view;
	GladePluginPriv *priv;
	
 	g_return_if_fail (GLADE_IS_PROJECT (project));
 	
	priv = glade_plugin->priv;
 	view = glade_design_view_new (project);	
	gtk_widget_show (view);
	g_object_set_data (G_OBJECT (project), "design_view", view);
	glade_app_add_project (project);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->design_notebook), GTK_WIDGET (view), NULL);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->design_notebook), -1);	
}
#else
static void
glade_plugin_add_project (GladePlugin *glade_plugin, GladeProject *project)
{
	glade_app_add_project (project);
}
#endif

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
#endif

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GladePlugin *glade_plugin;
	GladePluginPriv *priv;
	// GtkAction *action;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	
	DEBUG_PRINT ("GladePlugin: Activating Glade plugin...");
	
	glade_plugin = ANJUTA_PLUGIN_GLADE (plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	priv = glade_plugin->priv;
	
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
#if (GLADEUI_VERSION > 302)
#  if (GLADEUI_VERSION >= 330)
        priv->inspector = glade_inspector_new ();
        
        g_signal_connect (priv->inspector, "item-activated",
        				  G_CALLBACK (inspector_item_activated_cb),
        				  plugin);
#  else
		priv->inspector = glade_project_view_new ();
#  endif
#else
		priv->inspector = glade_project_view_new (GLADE_PROJECT_VIEW_TREE);
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

		
#if (GLADEUI_VERSION > 302)
		/* Create design_notebook */
		priv->design_notebook = gtk_notebook_new ();
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->design_notebook), FALSE);
		gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->design_notebook), FALSE);
		gtk_widget_show (priv->design_notebook);
#endif
	}
	
	g_signal_connect (G_OBJECT (priv->projects_combo), "changed",
					  G_CALLBACK (on_glade_project_changed), plugin);
	g_signal_connect (G_OBJECT (priv->gpw), "update-ui",
					  G_CALLBACK (glade_update_ui), plugin);
	
	g_signal_connect(G_OBJECT(glade_app_get_editor()), "gtk-doc-search",
					 G_CALLBACK(on_api_help), plugin);
	
	g_signal_connect(G_OBJECT(plugin->shell), "destroy",
					 G_CALLBACK(on_shell_destroy), plugin);
	
	/* Add action group */
	priv->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupGlade",
											_("Glade operations"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE,
											glade_plugin);
	/* Add UI */
	priv->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	/* FIXME: Glade doesn't want to die these widget, so
	 * hold a permenent refs on them
	 */
	g_object_ref (glade_app_get_palette ());
	g_object_ref (glade_app_get_editor ());
	g_object_ref (priv->view_box);
#if (GLADEUI_VERSION > 302)
	g_object_ref (priv->design_notebook);
#endif
	gtk_widget_show (GTK_WIDGET (glade_app_get_palette ()));
	gtk_widget_show (GTK_WIDGET (glade_app_get_editor ()));
	
	/* Add widgets */
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
#if (GLADEUI_VERSION > 302)
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (priv->design_notebook),
							 "AnjutaGladeDesigner", _("Designer"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
#endif
	/* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session",
					  G_CALLBACK (on_session_save), plugin);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	GladePluginPriv *priv;
	
	priv = ANJUTA_PLUGIN_GLADE (plugin)->priv;
	
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	DEBUG_PRINT ("GladePlugin: Dectivating Glade plugin...");
	
	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_save), plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->projects_combo),
										  G_CALLBACK (on_glade_project_changed),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->gpw),
										  G_CALLBACK (glade_update_ui),
										  plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT(glade_app_get_editor()),
										  G_CALLBACK(on_api_help), plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_shell_destroy),
										  plugin);
	/* Remove widgets */
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_palette ()),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_editor ()),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (priv->view_box),
								NULL);
#if (GLADEUI_VERSION > 302)
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (priv->design_notebook),
								NULL);
#endif
	
	/* Remove UI */
	anjuta_ui_unmerge (ui, priv->uiid);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, priv->action_group);
	
	/* FIXME: Don't destroy glade, since it doesn't want to */
	/* g_object_unref (G_OBJECT (priv->gpw)); */
	/* priv->gpw = NULL */
	
	priv->uiid = 0;
	priv->action_group = NULL;
	priv->gpw = NULL;
	
	return TRUE;
}

static void
glade_plugin_dispose (GObject *obj)
{
	/* GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj); */
	
	/* FIXME: Glade widgets should be destroyed */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
glade_plugin_finalize (GObject *obj)
{
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);
	g_free (plugin->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
glade_plugin_instance_init (GObject *obj)
{
	GladePluginPriv *priv;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);
	
	plugin->priv = (GladePluginPriv *) g_new0 (GladePluginPriv, 1);
	priv = plugin->priv;
	
	DEBUG_PRINT ("Intializing Glade plugin");
}

static void
glade_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = glade_plugin_dispose;
	klass->dispose = glade_plugin_finalize;
}

static void
ifile_open (IAnjutaFile *ifile, const gchar *uri, GError **err)
{
	GladePluginPriv *priv;
	GladeProject *project;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *filename;
	
	g_return_if_fail (uri != NULL);
	
	priv = ANJUTA_PLUGIN_GLADE (ifile)->priv;
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	if (!filename)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								    _("Not local file: %s"), uri);
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
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								    _("Could not open: %s"), uri);
		return;
	}
	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->projects_combo)));
	gtk_list_store_append (store, &iter);
#if (GLADEUI_VERSION >= 330)
	gtk_list_store_set (store, &iter, NAME_COL, glade_project_get_name(project),
						PROJECT_COL, project, -1);
#else
	gtk_list_store_set (store, &iter, NAME_COL, project->name,
						PROJECT_COL, project, -1);
#endif
	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (ifile), project);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (ifile)->shell, priv->view_box, NULL);
}

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
}

static void
iwizard_activate (IAnjutaWizard *iwizard, GError **err)
{
	GladePluginPriv *priv;
	GladeProject *project;
	GtkListStore *store;
	GtkTreeIter iter;
	
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
	gtk_list_store_set (store, &iter, NAME_COL, glade_project_get_name(project),
 						PROJECT_COL, project, -1);
#else
	gtk_list_store_set (store, &iter, NAME_COL, project->name,
						PROJECT_COL, project, -1);
#endif
	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (iwizard), project);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (iwizard)->shell,
				     GTK_WIDGET (glade_app_get_palette ()), NULL);
}

static void
iwizard_iface_init(IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (GladePlugin, glade_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GladePlugin, glade_plugin);
