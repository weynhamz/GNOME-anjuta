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
#include <gtk/gtk.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include "glade.h"
#include "glade-palette.h"
#include "glade-editor.h"
#include "glade-clipboard.h"
#include "glade-clipboard-view.h"
#include "glade-widget.h"
#include "glade-widget-class.h"
#include "glade-property.h"
#include "glade-property-class.h"
#include "glade-project.h"
#include "glade-project-view.h"
#include "glade-project-window.h"
#include "glade-placeholder.h"
#include "glade-command.h"
#include "glade-debug.h"
#include "glade-utils.h"
#include "glade-app.h"

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-glade.ui"

static gpointer parent_class;

/* This is variable required from libdevhelp */
gchar *geometry = NULL;

struct _GladePluginPriv
{
	gint uiid;
	GtkActionGroup *action_group;
	GladeApp *gpw;
	GladeProjectView *view;
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
	GladeApp *gpw;
	GList *prev_redo_item;
	GList *undo_item;
	GList *redo_item;
	const gchar *undo_description = NULL;
	const gchar *redo_description = NULL;
	GladeProject *project;
	GtkAction *action;
	gchar buffer[512];
	AnjutaUI *ui;
	GtkTreeModel *model;
	GtkTreeIter iter;

	gpw = plugin->priv->gpw;

	project = glade_app_get_active_project (gpw);
	if (!project)
	{
		undo_item = NULL;
		redo_item = NULL;
	}
	else
	{
		undo_item = prev_redo_item = project->prev_redo_item;
		redo_item = (prev_redo_item == NULL) ? project->undo_stack : prev_redo_item->next;

		if (undo_item && undo_item->data)
			undo_description = GLADE_COMMAND (undo_item->data)->description;
		if (redo_item && redo_item->data)
			redo_description = GLADE_COMMAND (redo_item->data)->description;
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
			if (project == glade_app_get_active_project (plugin->priv->gpw))
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
	glade_app_command_copy (GLADE_APP (plugin->priv->gpw));
}

static void
on_cut_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_cut (GLADE_APP (plugin->priv->gpw));
}

static void
on_paste_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_paste (GLADE_APP (plugin->priv->gpw));
}

static void
on_delete_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_delete (GLADE_APP (plugin->priv->gpw));
}

static void
on_undo_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_undo (GLADE_APP (plugin->priv->gpw));
}

static void
on_redo_activated (GtkAction *action, GladePlugin *plugin)
{
	glade_app_command_redo (GLADE_APP (plugin->priv->gpw));
}

static void
on_show_clipbard_activated (GtkAction *action, GladePlugin *plugin)
{
	GtkWidget *clipboard_view;
	
	clipboard_view = glade_app_get_clipboard_view (GLADE_APP (plugin->priv->gpw));
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
	anjuta_status_set (status, _("Glade project '%s' saved"), project->name);
}

static void
on_save_activated (GtkAction *action, GladePlugin *plugin)
{
	GladeApp *gpw;
	GladeProject *project;
	GtkWidget *filechooser;
	gchar *path = NULL;

	gpw = plugin->priv->gpw;
	project = glade_app_get_active_project (GLADE_APP (gpw));

	if (project->path != NULL) {
		AnjutaStatus *status;
		
		status = anjuta_shell_get_status (ANJUTA_PLUGIN(plugin)->shell, NULL);
		if (glade_project_save (project, project->path, NULL)) {
			anjuta_status_set (status, _("Glade project '%s' saved"), project->name);
		} else {
			anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
										_("Invalid glade file name"));
		}
		return;
	}

	/* If instead we dont have a path yet, fire up a file selector */
	filechooser = glade_util_file_dialog_new (_("Save glade file ..."),
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
	GladeApp *gpw;
	GladeProject *project;
	GtkWidget *filechooser;
	gchar *path = NULL;

	gpw = plugin->priv->gpw;
	project = glade_app_get_active_project (GLADE_APP (gpw));

	filechooser = glade_util_file_dialog_new (_("Save glade file as ..."),
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
	char *msg;
	gint ret;

	g_return_val_if_fail (GLADE_IS_PROJECT (project), FALSE);

	gpw = plugin->priv->gpw;
	msg = g_strdup_printf (_("<span weight=\"bold\" size=\"larger\">Save changes to glade project \"%s\" before closing?</span>\n\n"
				 "Your changes will be lost if you don't save them.\n"), project->name);

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
		if (project->path != NULL)
		{
			close = glade_project_save (project, project->path, NULL);
		}
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
glade_do_close (GladePlugin *plugin, GladeProject *project)
{
	// FIXME: Remove from combo.	

	glade_app_remove_project (plugin->priv->gpw, project);
}

static void
on_close_activated (GtkAction *action, GladePlugin *plugin)
{
	GladeApp *gpw;
	GladeProject *project;
	gboolean close;
	
	gpw = plugin->priv->gpw;
	project = glade_app_get_active_project (GLADE_APP (gpw));

	if (!project)
		return;

	if (project->changed)
	{
		close = glade_confirm_close_project (plugin, project);
			if (!close)
				return;
	}
	glade_do_close (plugin, project);
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
		N_("Save _As ..."),
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

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GladePlugin *glade_plugin;
	GladePluginPriv *priv;
	// GtkAction *action;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	
	DEBUG_PRINT ("GladePlugin: Activating Glade plugin ...");
	
	glade_plugin = (GladePlugin*) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	priv = glade_plugin->priv;
	
	priv->gpw = glade_app_new();
	glade_app_set_window (priv->gpw, GTK_WIDGET (ANJUTA_PLUGIN(plugin)->shell));
	glade_default_app_set (priv->gpw);
	
	/* Create a view for us */
	priv->view_box = gtk_vbox_new (FALSE, 0);
	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, NULL);
	
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
	
	priv->view = glade_project_view_new (GLADE_PROJECT_VIEW_TREE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->view),
					GTK_POLICY_NEVER, GTK_POLICY_NEVER);

	glade_app_add_project_view (priv->gpw, priv->view);
	gtk_box_pack_start (GTK_BOX (priv->view_box), GTK_WIDGET (priv->view),
						TRUE, TRUE, 0);
	
	gtk_widget_show_all (priv->view_box);
	
	g_signal_connect (G_OBJECT (priv->gpw), "update-ui",
					  G_CALLBACK (glade_update_ui), plugin);
	
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (glade_app_get_editor (priv->gpw)),
								 TRUE);
	gtk_notebook_popup_enable (GTK_NOTEBOOK (glade_app_get_editor (priv->gpw)));

	/* Add action group */
	priv->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupGlade",
										_("Glade operations"),
										actions,
										G_N_ELEMENTS (actions),
										glade_plugin);
	/* Add UI */
	priv->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	/* Add widgets */
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (glade_app_get_palette (priv->gpw)),
							 "AnjutaGladePalette", "Glade Palette", NULL,
							 ANJUTA_SHELL_PLACEMENT_TOP, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (glade_app_get_editor (priv->gpw)),
							 "AnjutaGladeEditor", "Glade Editor", NULL,
							 ANJUTA_SHELL_PLACEMENT_RIGHT, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (priv->view_box),
							 "AnjutaGladeTree", "Glade Tree", NULL,
							 ANJUTA_SHELL_PLACEMENT_RIGHT, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	GladePluginPriv *priv;
	
	priv = ((GladePlugin*)plugin)->priv;
	
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	DEBUG_PRINT ("GladePlugin: Dectivating Glade plugin ...");
	
	/* Remove widgets */
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_palette (priv->gpw)),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_editor (priv->gpw)),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (priv->view_box),
								NULL);
	
	/* Remove UI */
	anjuta_ui_unmerge (ui, priv->uiid);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, priv->action_group);
	
	/* Destroy glade */
	g_object_unref (G_OBJECT (priv->gpw));
	
	priv->uiid = 0;
	priv->action_group = NULL;
	priv->gpw = NULL;
	
	return TRUE;
}

static void
glade_plugin_dispose (GObject *obj)
{
	// GladePlugin *plugin = (GladePlugin*)obj;
	
	/* FIXME: Glade widgets should be destroyed */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
glade_plugin_finalize (GObject *obj)
{
	GladePlugin *plugin = (GladePlugin*)obj;
	g_free (plugin->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
glade_plugin_instance_init (GObject *obj)
{
	GladePluginPriv *priv;
	GladePlugin *plugin = (GladePlugin*)obj;
	
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
	
	g_return_if_fail (uri != NULL);
	
	priv = ((GladePlugin*)G_OBJECT (ifile))->priv;
	project = glade_project_open (uri);
	if (!project)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								    _("Could not open: %s"), uri);
		return;
	}
	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->projects_combo)));
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, NAME_COL, project->name,
						PROJECT_COL, project, -1);
	glade_app_add_project (priv->gpw, project);
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
	
	priv = ((GladePlugin*)G_OBJECT (iwizard))->priv;

	project = glade_project_new (TRUE);
	if (!project)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (iwizard)->shell),
								    _("Could not create a new glade project."));
		return;
	}
	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->projects_combo)));
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, NAME_COL, project->name,
						PROJECT_COL, project, -1);
	glade_app_add_project (priv->gpw, project);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (iwizard)->shell, priv->view_box, NULL);
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
