/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Massimo Cora' 2005 <maxcvs@gmail.com>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#include <config.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-about.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

#include "vgdefaultview.h"
#include "plugin.h"
#include "symtab.h"


#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-valgrind.ui"
#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-valgrind.glade"
#define ICON_FILE PACKAGE_PIXMAPS_DIR"/anjuta-valgrind.png"

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);


enum {
	MEMCHECK_OPTION,
	ADDRCHECK_OPTION,
//	CACHEGRIND_OPTION,
	HELGRIND_OPTION
};

static gpointer parent_class;

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
	REGISTER_ICON ("anjuta-valgrind-knight.png", "valgrind-knight");
}


static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaValgrindPlugin *val_plugin;
	const gchar *root_uri;

	val_plugin = (AnjutaValgrindPlugin*) plugin;
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		if (root_dir)
			val_plugin->project_root_uri = g_strdup(root_dir);
		else
			val_plugin->project_root_uri = NULL;
		g_free (root_dir);
	}
	else
		val_plugin->project_root_uri = NULL;
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	AnjutaValgrindPlugin *val_plugin;
	val_plugin = (AnjutaValgrindPlugin*) plugin;
	
	if (val_plugin->project_root_uri)
		g_free(val_plugin->project_root_uri);
	val_plugin->project_root_uri = NULL;
}

static SymTab *
load_symtab (const char *progname)
{
	SymTab *symtab;
	char *filename;
	
	if (!(filename = vg_tool_view_scan_path (progname)))
		return NULL;
	
	symtab = symtab_new (filename);
	g_free (filename);
	
	return symtab;
}

/*---------------------------------------------------------------------------
 * Perform some actions on select_and_run_dialog options button clicked.
 * In particular it displays the option window for the selected [via combobox] valgrind 
 * tool.
 */
static void
on_options_button_clicked (GtkButton *button, GladeXML *gxml) 
{
	GtkWidget *tool_combobox, *vgtool;
	GtkDialog *dlg;
	gint active_option;

	vgtool = NULL;
	tool_combobox = glade_xml_get_widget (gxml, "val_tool");
	
	active_option = gtk_combo_box_get_active (GTK_COMBO_BOX (tool_combobox));
	
	dlg = GTK_DIALOG (gtk_dialog_new_with_buttons ( _("Options"), NULL, GTK_DIALOG_MODAL,
										  GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
										  NULL));
	g_signal_connect_swapped (dlg, "response",
							  G_CALLBACK (gtk_widget_destroy), dlg);
	
	switch (active_option) {
		case MEMCHECK_OPTION:
		case ADDRCHECK_OPTION:
			vgtool = valgrind_plugin_prefs_get_memcheck_widget ();
			break;
/*		
		case CACHEGRIND_OPTION:
			vgtool = valgrind_plugin_prefs_get_cachegrind_widget ();
			break;
*/		
		case HELGRIND_OPTION:
			vgtool = valgrind_plugin_prefs_get_helgrind_widget ();
			break;
	}
	
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dlg)->vbox), 3);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vgtool, TRUE, TRUE, 0);
	
	gtk_widget_show_all (GTK_WIDGET (dlg));
	gtk_dialog_run (dlg);
}

static void
on_menu_editrules_activate (GtkAction *action, AnjutaValgrindPlugin *plugin)  
{
	vg_tool_view_show_rules ((VgToolView *) plugin->valgrind_widget);
}

static void
on_menu_kill_activate (GtkAction *action, AnjutaValgrindPlugin *plugin) 
{
	vg_actions_kill (plugin->val_actions);
}

static void
on_menu_run_activate (GtkAction *action, AnjutaValgrindPlugin *plugin) 
{
	IAnjutaProjectManager *pm;
	GList *exec_targets;
			
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
									 IAnjutaProjectManager, NULL);
	g_return_if_fail (pm != NULL);
			
	exec_targets =
		ianjuta_project_manager_get_targets (pm,
						 IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE,
						 NULL);

	if (g_list_length (exec_targets) >= 1) {
		GladeXML *gxml;
		GtkWidget *dlg, *treeview, *tool_combobox;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		GtkButton *options_button;
		GtkListStore *store;
		gint response, tool_selected;
		GList *node;
		GtkTreeIter iter;
		const gchar *sel_target = NULL;

		tool_selected = 0;
		gxml = glade_xml_new (GLADE_FILE, "select_and_run_dialog",
							  NULL);
		dlg = glade_xml_get_widget (gxml, "select_and_run_dialog");
		treeview = glade_xml_get_widget (gxml, "programs_treeview");
		
		tool_combobox = glade_xml_get_widget (gxml, "val_tool");
		gtk_combo_box_set_active (GTK_COMBO_BOX (tool_combobox), 0);
		
		options_button = GTK_BUTTON (glade_xml_get_widget (gxml, "options_button"));
		
		/* connect the signal to grab any click on it */
		g_signal_connect (G_OBJECT (options_button), "clicked",
				G_CALLBACK (on_options_button_clicked), gxml);
				
		gtk_window_set_transient_for (GTK_WINDOW (dlg),
					  GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell));
		store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
		node = exec_targets;	
	
		while (node) {
			const gchar *rel_path;
			rel_path = 	(gchar*)node->data + 
					strlen (plugin->project_root_uri) + 1;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, rel_path, 1,
								node->data, -1);
			g_free (node->data);
			node = g_list_next (node);
		}
		g_list_free (exec_targets);

		gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
								 GTK_TREE_MODEL (store));
		g_object_unref (store);
				
		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_sizing (column,
										 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_title (column,
										_("Select debugging target"));

		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (column, renderer, FALSE);
		gtk_tree_view_column_add_attribute (column, renderer, "text",
											0);
		gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
		gtk_tree_view_set_expander_column (GTK_TREE_VIEW (treeview),
									   column);

		/* Run dialog */
		response = gtk_dialog_run (GTK_DIALOG (dlg));
		if (response == GTK_RESPONSE_OK) {		
			GtkTreeSelection *sel;
			GtkTreeModel *model;
					
			sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
			if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
				gtk_tree_model_get (model, &iter, 1, &sel_target, -1);
			}
			
			/* get the selected tool before destroying the dialog */
			tool_selected = gtk_combo_box_get_active (GTK_COMBO_BOX (tool_combobox));
		}
		
		gtk_widget_destroy (dlg);
		
		if (sel_target) {
			gchar *prgname;
			gchar *program_dir;
			SymTab *symtab;

			prgname = gnome_vfs_format_uri_for_display (sel_target);
			DEBUG_PRINT ("target program selected is %s", prgname);			
			
			/* lets set some infos */
			program_dir = g_path_get_dirname (prgname);
			DEBUG_PRINT ("target a basedir: %s", program_dir);
			
			vg_tool_view_set_argv ((VgToolView *) plugin->valgrind_widget, 
							(const char**)sel_target);
			vg_tool_view_set_srcdir ((VgToolView *) plugin->valgrind_widget, 
							(const char**)program_dir);
			
			symtab = load_symtab (prgname);
			vg_tool_view_set_symtab ((VgToolView *) plugin->valgrind_widget,
							symtab);			

			plugin->val_actions = 
					vg_actions_new (plugin, plugin->val_prefs, 
							(VgDefaultView *)plugin->valgrind_widget);
			
			switch (tool_selected) {
				case MEMCHECK_OPTION:
					/* this is not a blocking call. The process will fork */
					vg_actions_run (plugin->val_actions,
								prgname, "memcheck", NULL);
					break;
		
				case ADDRCHECK_OPTION:
					/* this is not a blocking call. The process will fork */
					vg_actions_run (plugin->val_actions, 
								prgname, "addrcheck", NULL);
					break;
		
				case HELGRIND_OPTION:
					/* this is not a blocking call. The process will fork */
					vg_actions_run (plugin->val_actions, 
								prgname, "helgrind", NULL);
					break;
			}
		}
		else {
			/* TODO */
			/* display a message that says to the user that the project hasn't 
			 * any available target */
		}
		
		g_object_unref (gxml);
	}	
}



static GtkActionEntry actions_file[] = {
	{
		"ActionMenuValgrind",                   /* Action name */
		"valgrind-knight",                      /* Stock icon, if any */
		N_("_Valgrind"),                     	/* Display label */
		NULL,                                   /* short-cut */
		NULL,                      				/* Tooltip */			
		NULL    								/* action callback */
	},
	{
		"ActionValgrindRun",                    /* Action name */
		GTK_STOCK_EXECUTE,                      /* Stock icon, if any */
		N_("_Select tool and run..."),			/* Display label */
		NULL,                                   /* short-cut */
		NULL,                      				/* Tooltip */			
		G_CALLBACK(on_menu_run_activate)		/* action callback */
	},
	{
		"ActionValgrindKill",                   /* Action name */
		GTK_STOCK_CANCEL,                      	/* Stock icon, if any */
		N_("_Kill execution"),					/* Display label */
		NULL,                                   /* short-cut */
		NULL,                      				/* Tooltip */			
		G_CALLBACK(on_menu_kill_activate)		/* action callback */
	},
	{
		"ActionValgrindLoad",                   /* Action name */
		GTK_STOCK_OPEN,                      	/* Stock icon, if any */
		N_("_Load log"),						/* Display label */
		NULL,                                   /* short-cut */
		NULL,                      				/* Tooltip */			
		NULL								    /* action callback */
	},
	{
		"ActionValgrindSave",                   /* Action name */
		GTK_STOCK_SAVE,                      	/* Stock icon, if any */
		N_("S_ave log"),						/* Display label */
		NULL,                                   /* short-cut */
		NULL,                      				/* Tooltip */			
		NULL								    /* action callback */
	},
	{
		"ActionValgrindEditRules",              /* Action name */
		NULL,                      				/* Stock icon, if any */
		N_("Edit Rules"),						/* Display label */
		NULL,                                   /* short-cut */
		NULL,                      				/* Tooltip */			
		G_CALLBACK(on_menu_editrules_activate)  /* action callback */
	}	
	
};


void
valgrind_set_busy_status (AnjutaValgrindPlugin *plugin, gboolean status) {
	plugin->is_busy = status;
	
}

/*-----------------------------------------------------------------------------
 * we adjourn the Debug->Valgrind->* menu status [i.e. sensitive or not].
 */
void
valgrind_update_ui (AnjutaValgrindPlugin *plugin) 
{
	AnjutaUI *ui;
	GtkAction *action;	
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupValgrind",
								   "ActionValgrindRun");
	g_object_set (G_OBJECT (action), "sensitive",
				  !plugin->is_busy, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupValgrind",
								   "ActionValgrindKill");
	g_object_set (G_OBJECT (action), "sensitive",
				  plugin->is_busy, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupValgrind",
								   "ActionValgrindLoad");
	g_object_set (G_OBJECT (action), "sensitive",
				  !plugin->is_busy, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupValgrind",
								   "ActionValgrindSave");
	g_object_set (G_OBJECT (action), "sensitive",
				  !plugin->is_busy, NULL);

}


static gboolean
valgrind_activate (AnjutaPlugin *plugin)
{
	AnjutaPreferences *prefs;
	AnjutaUI *ui;
	GdkPixbuf *pixbuf;
	static gboolean initialized = FALSE;
	AnjutaValgrindPlugin *valgrind;
	ValgrindPluginPrefs *valgrind_prefs;	
	GtkWidget *wid;
	
	DEBUG_PRINT ("AnjutaValgrindPlugin: Activating AnjutaValgrindPlugin plugin ...");
	valgrind = (AnjutaValgrindPlugin*) plugin;

	if (!initialized) {
		register_stock_icons (plugin);
	}

	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupValgrind",
										_("Use Valgrind debug tool"),
										actions_file,
										G_N_ELEMENTS (actions_file),
										GETTEXT_PACKAGE, plugin);
	valgrind->uiid = anjuta_ui_merge (ui, UI_FILE);

	/* Add prefs page */
	valgrind_prefs = valgrind_plugin_prefs_new ();
	valgrind->general_prefs = valgrind_plugin_prefs_get_anj_prefs ();
	
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	if (!initialized) {
		pixbuf = gdk_pixbuf_new_from_file (ICON_FILE, NULL);
	
		anjuta_preferences_dialog_add_page (ANJUTA_PREFERENCES_DIALOG (prefs),
						"Valgrind", pixbuf, valgrind->general_prefs);
		g_object_unref (pixbuf);
	}

	/* Build the ValgrindPluginPrefs */
	valgrind->val_prefs = valgrind_plugin_prefs_new ();
	
	
	/* Add main widget to shell */
	wid = vg_default_view_new (valgrind);
	valgrind->valgrind_widget = wid;
	
	anjuta_shell_add_widget (((AnjutaPlugin*)plugin)->shell, wid,
							 "AnjutaValgrindPluginWidget", _("Valgrind"), "valgrind-knight",
							 ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);	

	/* set up project directory watch */
	valgrind->project_root_uri = NULL;
	valgrind->root_watch_id = anjuta_plugin_add_watch (plugin,
									"project_root_uri",
									project_root_added,
									project_root_removed, NULL);

	/* set busy status to FALSE: while initializing the plugin we're surely not 
	 running valgrind */
	valgrind_set_busy_status (valgrind, FALSE);
	valgrind_update_ui (valgrind);

	initialized = TRUE;
	return TRUE;
}

static gboolean
valgrind_deactivate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;

	DEBUG_PRINT ("AnjutaValgrindPlugin: Dectivating AnjutaValgrindPlugin plugin ...");

	if ( ((AnjutaValgrindPlugin*)plugin)->valgrind_widget != NULL )
		anjuta_shell_remove_widget (plugin->shell, ((AnjutaValgrindPlugin*)plugin)->valgrind_widget,
									NULL);

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, ((AnjutaValgrindPlugin*)plugin)->uiid);


	// FIXME: destroys vgactions/vgtoolview/ValgrindPluginPrefs objects
	
	return TRUE;
}

static void
valgrind_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
valgrind_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
valgrind_instance_init (GObject *obj)
{
	AnjutaValgrindPlugin *plugin = (AnjutaValgrindPlugin*)obj;

	plugin->uiid = 0;

	plugin->valgrind_widget = NULL;
	plugin->general_prefs = NULL;
}

static void
valgrind_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = valgrind_activate;
	plugin_class->deactivate = valgrind_deactivate;
	klass->finalize = valgrind_finalize;
	klass->dispose = valgrind_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (AnjutaValgrindPlugin, valgrind);
ANJUTA_SIMPLE_PLUGIN (AnjutaValgrindPlugin, valgrind);
