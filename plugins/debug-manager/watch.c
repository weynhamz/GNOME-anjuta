/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
 * watch.c Copyright (C) 2000 Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "watch.h"

#include "debug_tree.h"

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

/* Type
 *---------------------------------------------------------------------------*/

struct _ExprWatch
{
	AnjutaPlugin *plugin;
	
	GtkWidget *scrolledwindow;
	DebugTree *debug_tree;	
	DmaDebuggerQueue *debugger;
	
	/* Menu action */
	GtkActionGroup *action_group;
	GtkActionGroup *toggle_group;
};

struct _InspectDialog
{
	DebugTree *tree;
	GtkWidget *treeview;
	GtkWidget *dialog;
};

typedef struct _InspectDialog InspectDialog;

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define ADD_WATCH_DIALOG "add_watch_dialog"
#define CHANGE_WATCH_DIALOG "change_watch_dialog"
#define INSPECT_EVALUATE_DIALOG "watch_dialog"
#define NAME_ENTRY "name_entry"
#define VALUE_ENTRY "value_entry"
#define VALUE_TREE "value_treeview"
#define AUTO_UPDATE_CHECK "auto_update_check"

/* Private functions
 *---------------------------------------------------------------------------*/

#if 0
static void
on_entry_updated (const gchar *value, gpointer user_data, GError *err)
{
	GtkWidget *entry = GTK_WIDGET (user_data);
	
	gtk_entry_set_text (GTK_ENTRY (entry), value);
	gtk_widget_unref (entry);
}
#endif

static void
debug_tree_inspect_evaluate_dialog (ExprWatch * ew, const gchar* expression)
{
	GladeXML *gxml;
	gint reply;
	gchar *new_expr;
	// const gchar *value;
	InspectDialog dlg;
	IAnjutaDebuggerVariable var = {NULL, NULL, NULL, NULL, FALSE, -1};

	gxml = glade_xml_new (GLADE_FILE, INSPECT_EVALUATE_DIALOG, NULL);
	dlg.dialog = glade_xml_get_widget (gxml, INSPECT_EVALUATE_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (dlg.dialog),
								  NULL);
	dlg.treeview = glade_xml_get_widget (gxml, VALUE_TREE);
	g_object_unref (gxml);

	/* Create debug tree */
	dlg.tree = debug_tree_new_with_view (ANJUTA_PLUGIN (ew->plugin), GTK_TREE_VIEW (dlg.treeview));
	if (ew->debugger)
		debug_tree_connect (dlg.tree, ew->debugger);
	if (expression != NULL)
	{
		var.expression = (gchar *)expression;
		debug_tree_add_watch (dlg.tree, &var, FALSE);
	}
	else
	{
		debug_tree_add_dummy (dlg.tree, NULL);
	}

	for(;;)
	{
		reply = gtk_dialog_run (GTK_DIALOG (dlg.dialog));
		switch (reply)
		{
		case GTK_RESPONSE_OK:
			/* Add in watch window */
			new_expr = debug_tree_get_first (dlg.tree);

			if (new_expr != NULL)
			{
		    	var.expression = new_expr;
				debug_tree_add_watch (ew->debug_tree, &var, FALSE);
				g_free (new_expr);
			}
			break;
		default:
			break;
		}
		break;
	}
	debug_tree_free (dlg.tree);
	gtk_widget_destroy (dlg.dialog);
}

static void
debug_tree_add_watch_dialog (ExprWatch *ew, const gchar* expression)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *name_entry;
	GtkWidget *auto_update_check;
	gint reply;
	IAnjutaDebuggerVariable var = {NULL, NULL, NULL, NULL, FALSE, -1};

	gxml = glade_xml_new (GLADE_FILE, ADD_WATCH_DIALOG, NULL);
	dialog = glade_xml_get_widget (gxml, ADD_WATCH_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  NULL);
	auto_update_check = glade_xml_get_widget (gxml, AUTO_UPDATE_CHECK);
	name_entry = glade_xml_get_widget (gxml, NAME_ENTRY);
	g_object_unref (gxml);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_update_check), TRUE);
	gtk_entry_set_text (GTK_ENTRY (name_entry), expression == NULL ? "" : expression);
	
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	if (reply == GTK_RESPONSE_OK)
	{
		var.expression = (gchar *)gtk_entry_get_text (GTK_ENTRY (name_entry));
		debug_tree_add_watch (ew->debug_tree, &var,
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (auto_update_check)));
	}
	gtk_widget_destroy (dialog);
}

static void
debug_tree_change_watch_dialog (ExprWatch *ew, GtkTreeIter* iter)
{
#if 0
 	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *name_entry;
	GtkWidget *value_entry;
	gint reply;
	TrimmableItem *item = NULL;
	GtkTreeModel* model = NULL;
	
	model = gtk_tree_view_get_model(d_tree->view);
	gtk_tree_model_get (model, iter, ITEM_COLUMN, &item, -1);

	gxml = glade_xml_new (GLADE_FILE, CHANGE_WATCH_DIALOG, NULL);
	dialog = glade_xml_get_widget (gxml, CHANGE_WATCH_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  NULL);
	name_entry = glade_xml_get_widget (gxml, NAME_ENTRY);
	value_entry = glade_xml_get_widget (gxml, VALUE_ENTRY);
	g_object_unref (gxml);

	gtk_widget_grab_focus (value_entry);
	gtk_entry_set_text (GTK_ENTRY (name_entry), &item->name[1]);
	gtk_entry_set_text (GTK_ENTRY (value_entry), item->value);
	
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	if (reply == GTK_RESPONSE_APPLY)
	{
		debug_tree_evaluate (d_tree, iter, gtk_entry_get_text (GTK_ENTRY (value_entry))); 
	}
	gtk_widget_destroy (dialog);
#endif
}

static void
on_program_stopped (ExprWatch *ew)
{
	debug_tree_update_all (ew->debug_tree);
}

static void
on_debugger_stopped (ExprWatch *ew)
{
	debug_tree_disconnect (ew->debug_tree);

	/* Disconnect to other debugger signal */
	g_signal_handlers_disconnect_by_func (ew->plugin, G_CALLBACK (on_debugger_stopped), ew);
	g_signal_handlers_disconnect_by_func (ew->plugin, G_CALLBACK (on_program_stopped), ew);
}

static void
on_program_loaded (ExprWatch *ew)
{
	if (!(dma_debugger_queue_get_feature (ew->debugger) & HAS_VARIABLE)) return;

	debug_tree_connect (ew->debug_tree, ew->debugger);
	
	/* Connect to other debugger signal */
	g_signal_connect_swapped (ew->plugin, "debugger-stopped", G_CALLBACK (on_debugger_stopped), ew);
	g_signal_connect_swapped (ew->plugin, "program-stopped", G_CALLBACK (on_program_stopped), ew);
}

/* Menu call backs
 *---------------------------------------------------------------------------*/

static void
on_debug_tree_inspect (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	GObject *obj;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *te = NULL;
	gchar *expression = NULL;
	
	/* Get current editor and line */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (ew->plugin)->shell,
			"IAnjutaDocumentManager", NULL /* TODO */);
	docman = IANJUTA_DOCUMENT_MANAGER (obj);
	if (docman != NULL)
	{
		te = IANJUTA_EDITOR(ianjuta_document_manager_get_current_document (docman, NULL));
		if (!te)
			return;
		expression = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL);
		if (expression == NULL)
		{
			expression = ianjuta_editor_get_current_word (IANJUTA_EDITOR (te), NULL);
		}
	}
	
	debug_tree_inspect_evaluate_dialog (ew, expression);
	g_free (expression);
}

static void
on_debug_tree_add_watch (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	
	debug_tree_add_watch_dialog (ew, NULL);
}

static void
on_debug_tree_remove_watch (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	GtkTreeIter iter;
	
	if (debug_tree_get_current (ew->debug_tree, &iter))
	{
		debug_tree_remove (ew->debug_tree, &iter);
	}
}

static void
on_debug_tree_update_watch (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	GtkTreeIter iter;

	if (debug_tree_get_current (ew->debug_tree, &iter))
	{
		debug_tree_update (ew->debug_tree, &iter, TRUE);
	}
}

static void
on_debug_tree_auto_update_watch (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	GtkTreeIter iter;

	if (debug_tree_get_current (ew->debug_tree, &iter))
	{
		gboolean state;
		
		state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
		debug_tree_set_auto_update (ew->debug_tree, &iter, state);
	}
}

static void
on_debug_tree_edit_watch (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	GtkTreeIter iter;
	
	if (debug_tree_get_current (ew->debug_tree, &iter))
	{
		debug_tree_change_watch_dialog (ew, &iter);
	}
}

static void
on_debug_tree_update_all_watch (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	
	debug_tree_update_all (ew->debug_tree);
}

static void
on_debug_tree_remove_all_watch (GtkAction *action, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;
	
	debug_tree_remove_all (ew->debug_tree);
}

static gboolean
on_debug_tree_button_press (GtkWidget *widget, GdkEventButton *bevent, gpointer user_data)
{
	ExprWatch * ew = (ExprWatch *)user_data;

	if (bevent->button == 3)
	{
		GtkAction *action;
		AnjutaUI *ui;
		GtkTreeIter iter;
		GtkWidget *middle_click_menu;

		ui = anjuta_shell_get_ui (ew->plugin->shell, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupWatchToggle", "ActionDmaAutoUpdateWatch");
		if (debug_tree_get_current (ew->debug_tree, &iter))
		{
			gtk_action_set_sensitive (GTK_ACTION (action), TRUE);
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), debug_tree_get_auto_update (ew->debug_tree, &iter));
		}
		else
		{
			gtk_action_set_sensitive (GTK_ACTION (action), FALSE);
		}

		action = anjuta_ui_get_action (ui, "ActionGroupWatch", "ActionDmaEditWatch");
		gtk_action_set_sensitive (GTK_ACTION (action), FALSE);   // FIXME: Not implemented
		
		middle_click_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupWatch");
		g_return_val_if_fail (middle_click_menu != NULL, FALSE);
		gtk_menu_popup (GTK_MENU (middle_click_menu), NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
	}
	
	return FALSE;
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_watch[] = {
    {
		"ActionDmaInspect",                      /* Action name */
		GTK_STOCK_DIALOG_INFO,                   /* Stock icon, if any */
		N_("Ins_pect/Evaluate..."),              /* Display label */
		NULL,                                    /* short-cut */
		N_("Inspect or evaluate an expression or variable"), /* Tooltip */
		G_CALLBACK (on_debug_tree_inspect) /* action callback */
    },
	{
		"ActionDmaAddWatch",                      
		NULL,                                     
		N_("Add Watch..."),                      
		NULL,                                    
		NULL,                                     
		G_CALLBACK (on_debug_tree_add_watch)     
	},
	{
		"ActionDmaRemoveWatch",
		NULL,
		N_("Remove Watch"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_remove_watch)
	},
	{
		"ActionDmaUpdateWatch",
		NULL,
		N_("Update Watch"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_update_watch)
	},
	{
		"ActionDmaEditWatch",
		NULL,
		N_("Change Value"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_edit_watch)
	},
	{
		"ActionDmaUpdateAllWatch",
		NULL,
		N_("Update all"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_update_all_watch)
	},
	{
		"ActionDmaRemoveAllWatch",
		NULL,
		N_("Remove all"),
		NULL,
		NULL,
		G_CALLBACK (on_debug_tree_remove_all_watch)
	}
};		

static GtkToggleActionEntry toggle_watch[] = {
	{
		"ActionDmaAutoUpdateWatch",               /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Automatic update"),                   /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		G_CALLBACK (on_debug_tree_auto_update_watch), /* action callback */
		FALSE                                     /* Initial state */
	}
};

static void
create_expr_watch_gui (ExprWatch * ew)
{
	AnjutaUI *ui;
	
	ew->debug_tree = debug_tree_new (ew->plugin);
	ew->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (ew->scrolledwindow);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ew->scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (ew->scrolledwindow),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (ew->scrolledwindow), debug_tree_get_tree_widget (ew->debug_tree));
	
	ui = anjuta_shell_get_ui (ew->plugin->shell, NULL);
	ew->action_group =
	      anjuta_ui_add_action_group_entries (ui, "ActionGroupWatch",
											_("Watch operations"),
											actions_watch,
											G_N_ELEMENTS (actions_watch),
											GETTEXT_PACKAGE, TRUE, ew);
	ew->toggle_group =
		      anjuta_ui_add_toggle_action_group_entries (ui, "ActionGroupWatchToggle",
											_("Watch operations"),
											toggle_watch,
											G_N_ELEMENTS (toggle_watch),
											GETTEXT_PACKAGE, TRUE, ew);
	g_signal_connect (debug_tree_get_tree_widget (ew->debug_tree), "button-press-event", G_CALLBACK (on_debug_tree_button_press), ew);  
	
	gtk_widget_show_all (ew->scrolledwindow);
}

/* Public function
 *---------------------------------------------------------------------------*/

/* Callback for saving session
 *---------------------------------------------------------------------------*/

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ExprWatch *ew)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	list = debug_tree_get_full_watch_list (ew->debug_tree);
	if (list != NULL)
		anjuta_session_set_string_list (session, "Debugger", "Watch", list);
	g_list_foreach (list, (GFunc)g_free, NULL);
    g_list_free (list);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ExprWatch *ew)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	/* debug_tree_remove_all (ew->debug_tree); */
	list = anjuta_session_get_string_list (session, "Debugger", "Watch");
	if (list != NULL)
		debug_tree_add_full_watch_list (ew->debug_tree, list);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

ExprWatch *
expr_watch_new (AnjutaPlugin *plugin)
{
	ExprWatch *ew;

	ew = g_new0 (ExprWatch, 1);
	ew->plugin = plugin;
	create_expr_watch_gui (ew);
	ew->debugger = dma_debug_manager_get_queue (ANJUTA_PLUGIN_DEBUG_MANAGER (plugin));

	/* Connect to Load and Save event */
	g_signal_connect (ew->plugin->shell, "save-session",
					  G_CALLBACK (on_session_save), ew);
    	g_signal_connect (ew->plugin->shell, "load-session",
					  G_CALLBACK (on_session_load), ew);
	
	/* Add watch window */
	anjuta_shell_add_widget (ew->plugin->shell,
							 ew->scrolledwindow,
                             "AnjutaDebuggerWatch", _("Watches"),
                             "gdb-watch-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
                              NULL);
	
	/* Connect to debugger */
	g_signal_connect_swapped (ew->plugin, "program-loaded", G_CALLBACK (on_program_loaded), ew);
	
	return ew;
}

void
expr_watch_destroy (ExprWatch * ew)
{
	AnjutaUI *ui;
	
	g_return_if_fail (ew != NULL);
	
	/* Disconnect from Load and Save event */
	g_signal_handlers_disconnect_matched (ew->plugin->shell, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, ew);
	g_signal_handlers_disconnect_matched (ew->plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, ew);

	ui = anjuta_shell_get_ui (ew->plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, ew->action_group);
	anjuta_ui_remove_action_group (ui, ew->toggle_group);

	debug_tree_free (ew->debug_tree);
	gtk_widget_destroy (ew->scrolledwindow);
	g_free (ew);
}
