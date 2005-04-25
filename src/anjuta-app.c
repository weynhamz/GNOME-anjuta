/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta.c Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#include <gnome.h>
#include <gtk/gtkwidget.h>
#include <glade/glade.h>

#include <gdl/gdl-dock.h>
#include <gdl/gdl-dock-bar.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/plugins.h>
#include <libanjuta/anjuta-debug.h>

#include "anjuta-app.h"
#include "anjuta-callbacks.h"
#include "anjuta-actions.h"
#include "about.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta.ui"

static void anjuta_app_layout_load (AnjutaApp *app,
									const gchar *layout_filename,
									const gchar *name);
static void anjuta_app_layout_save (AnjutaApp *app,
									const gchar *layout_filename,
									const gchar *name);

static gpointer parent_class = NULL;

static void
on_toggle_widget_view (GtkCheckMenuItem *menuitem, GtkWidget *dockitem)
{
	gboolean state;
	state = gtk_check_menu_item_get_active (menuitem);
	if (state)
		gdl_dock_item_show_item (GDL_DOCK_ITEM (dockitem));
	else
		gdl_dock_item_hide_item (GDL_DOCK_ITEM (dockitem));
}

static void
on_update_widget_view_menuitem (gpointer key, gpointer wid, gpointer data)
{
	GtkCheckMenuItem *menuitem;
	GdlDockItem *dockitem;
	
	dockitem = g_object_get_data (G_OBJECT (wid), "dockitem");
	menuitem = g_object_get_data (G_OBJECT (wid), "menuitem");
	
	g_signal_handlers_block_by_func (menuitem,
									 G_CALLBACK (on_toggle_widget_view),
									 dockitem);
	
	if (GDL_DOCK_OBJECT_ATTACHED (dockitem))
		gtk_check_menu_item_set_active (menuitem, TRUE);
	else
		gtk_check_menu_item_set_active (menuitem, FALSE);
	
	g_signal_handlers_unblock_by_func (menuitem,
									   G_CALLBACK (on_toggle_widget_view),
									   dockitem);
}

static void 
on_layout_dirty_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	if (!strcmp (pspec->name, "dirty")) {
		gboolean dirty;
		g_object_get (object, "dirty", &dirty, NULL);
		if (dirty) {
			/* Update UI toggle buttons */
			g_hash_table_foreach (ANJUTA_APP (user_data)->widgets,
								  on_update_widget_view_menuitem,
								  NULL);
		}
	}
}

static void
on_layout_locked_notify (GdlDockMaster *master,
                         GParamSpec    *pspec,
                         AnjutaApp     *app)
{
	AnjutaUI *ui;
	GtkAction *action;
	gint locked;
	
	ui = app->ui;
	action = anjuta_ui_get_action (ui, "ActionGroupToggleView",
								   "ActionViewLockLayout");
	
	g_object_get (master, "locked", &locked, NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
								  (locked == 1));
}

static void
on_toolbar_view_toggled (GtkCheckMenuItem *menuitem, GtkWidget *widget)
{
	AnjutaApp *app;
	BonoboDockItem *dock_item;
	const gchar *name;
	gint band;
	
	name = gtk_widget_get_name (widget);
	app = g_object_get_data (G_OBJECT(widget), "app");
	dock_item = g_object_get_data (G_OBJECT(widget), "dock_item");
	band = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(widget), "band"));
	
	if (gtk_check_menu_item_get_active (menuitem))
	{
		if (!dock_item)
		{
			/* Widget not yet added to the dock. Add it */
			gnome_app_add_docked (GNOME_APP (app), widget,
								  name,
								  BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL |
								  BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING,
								  BONOBO_DOCK_TOP, band, 0, 0);
		
			dock_item = gnome_app_get_dock_item_by_name (GNOME_APP (app),
														 name);
			g_object_set_data (G_OBJECT(widget), "dock_item", dock_item);
		}
		gtk_widget_show (GTK_WIDGET (dock_item));
		gtk_widget_show (GTK_BIN (dock_item)->child);
	}
	else if (dock_item)
	{
		gtk_widget_hide (GTK_WIDGET (dock_item));
		gtk_widget_hide (GTK_BIN (dock_item)->child);
	}
}

static void
on_add_merge_widget (GtkUIManager *merge, GtkWidget *widget,
					 GtkWidget *ui_container)
{
	DEBUG_PRINT ("Adding UI item ...");
	
	if (GTK_IS_MENU_BAR (widget))
	{
		gnome_app_set_menus (GNOME_APP (ui_container), GTK_MENU_BAR (widget));
		gtk_widget_show (widget);
	}
	else
	{
		static gint count = 0;
		const gchar *toolbarname;
		gchar* key;
		AnjutaPreferences *pr;
		GtkWidget *menuitem;
		
		/*
		gtk_toolbar_set_icon_size (GTK_TOOLBAR (widget),
								   GTK_ICON_SIZE_SMALL_TOOLBAR);
		*/
		gtk_toolbar_set_show_arrow (GTK_TOOLBAR (widget), FALSE);
		
		/* FIXME: This is probably not HIG-friendly. But the text/both
		 * toolbar styles in anjuta is horible. We need to fix that first.
		 */
		gtk_toolbar_set_style (GTK_TOOLBAR (widget), GTK_TOOLBAR_BOTH_HORIZ);
		
		toolbarname = gtk_widget_get_name (widget);
		
		DEBUG_PRINT ("Adding toolbar: %s", toolbarname);
		
		gtk_widget_show (widget);
		g_object_set_data (G_OBJECT (widget), "app", ui_container);
		g_object_set_data (G_OBJECT (widget), "band",
						   GINT_TO_POINTER(count+1));
		
		if (!ANJUTA_APP (ui_container)->toolbars_menu)
		{
			ANJUTA_APP (ui_container)->toolbars_menu = gtk_menu_new ();
			gtk_widget_show (GTK_WIDGET (ANJUTA_APP (ui_container)->toolbars_menu));
		}
		
		menuitem = gtk_check_menu_item_new_with_label (toolbarname);
		gtk_menu_append (GTK_MENU (ANJUTA_APP (ui_container)->toolbars_menu),
						 menuitem);
		gtk_widget_show (GTK_WIDGET (menuitem));
		g_signal_connect (G_OBJECT (menuitem), "toggled",
						  G_CALLBACK (on_toolbar_view_toggled), widget);

		/* FIXME: This doesn't work: Show/hide toolbar */
		pr = ANJUTA_PREFERENCES (ANJUTA_APP(ui_container)->preferences);
		key = g_strconcat (toolbarname, ".visible", NULL);
		if (anjuta_preferences_get_int_with_default (pr, key,
													 (count == 0)? 1:0))
		{
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
											TRUE);
		}
		g_free (key);
		count ++;
	}
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, AnjutaApp *app)
{
	gchar *geometry, *layout_file;
	GdkWindowState state;
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	/* Save geometry */
	state = gdk_window_get_state (GTK_WIDGET (app)->window);
	if (state & GDK_WINDOW_STATE_MAXIMIZED) {
		anjuta_session_set_int (session, "Anjuta", "Maximized", 1);
	}
	if (state & GDK_WINDOW_STATE_FULLSCREEN) {
		anjuta_session_set_int (session, "Anjuta", "Fullscreen", 1);
	}
	
	/* Save geometry only if window is not maximized or fullscreened */
	if (!(state & GDK_WINDOW_STATE_MAXIMIZED) ||
		!(state & GDK_WINDOW_STATE_FULLSCREEN))
	{
		geometry = anjuta_app_get_geometry (app);
		if (geometry)
			anjuta_session_set_string (session, "Anjuta", "Geometry",
									   geometry);
		g_free (geometry);
	}

	/* Save layout */
	layout_file = g_build_filename (anjuta_session_get_session_directory (session),
									"dock-layout.xml", NULL);
	anjuta_app_layout_save (app, layout_file, NULL);
	g_free (layout_file);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, AnjutaApp *app)
{
	/* We load layout at last so that all plugins would have loaded by now */
	if (phase == ANJUTA_SESSION_PHASE_LAST)
	{
		gchar *geometry;
		gchar *layout_file;
		
		/* Restore geometry */
		geometry = anjuta_session_get_string (session, "Anjuta", "Geometry");
		anjuta_app_set_geometry (app, geometry);
		
		/* Restore window state */
		if (anjuta_session_get_int (session, "Anjuta", "Fullscreen"))
		{
			gtk_window_fullscreen (GTK_WINDOW (shell));
		}
		else if (anjuta_session_get_int (session, "Anjuta", "Maximized"))
		{
			gtk_window_maximize (GTK_WINDOW (shell));
		}
		
		/* Restore layout */
		layout_file = g_build_filename (anjuta_session_get_session_directory (session),
										"dock-layout.xml", NULL);
		anjuta_app_layout_load (app, layout_file, NULL);
		g_free (layout_file);
	}
}

static void
anjuta_app_dispose (GObject *widget)
{
	AnjutaApp *app;

	g_return_if_fail (ANJUTA_IS_APP (widget));
	
	app = ANJUTA_APP (widget);
	
	if (app->widgets)
	{
		if (g_hash_table_size (app->widgets) > 0)
		{
			/*
			g_warning ("Some widgets are still inside shell (%d widgets), they are:",
					   g_hash_table_size (app->widgets));
			g_hash_table_foreach (app->widgets, (GHFunc)puts, NULL);
			*/
		}
		g_hash_table_destroy (app->widgets);
		app->widgets = NULL;
	}
	
	if (app->values)
	{
		if (g_hash_table_size (app->values) > 0)
		{
			/*
			g_warning ("Some Values are still left in shell (%d Values), they are:",
					   g_hash_table_size (app->values));
			g_hash_table_foreach (app->values, (GHFunc)puts, NULL);
			*/
		}
		g_hash_table_destroy (app->values);
		app->values = NULL;
	}
	
	if (app->layout_manager) {
		g_object_unref (app->layout_manager);
		app->layout_manager = NULL;
	}
	if (app->status) {
		g_object_unref (G_OBJECT (app->status));
		app->status = NULL;
	}
	GNOME_CALL_PARENT(G_OBJECT_CLASS, dispose, (widget));
}

static void
anjuta_app_finalize (GObject *widget)
{
	AnjutaApp *app;
	
	g_return_if_fail (ANJUTA_IS_APP (widget));
	
	app = ANJUTA_APP (widget);

	gtk_widget_destroy (GTK_WIDGET (app->ui));
	gtk_widget_destroy (GTK_WIDGET (app->preferences));
	
	GNOME_CALL_PARENT(G_OBJECT_CLASS, finalize, (widget));
}

static void
anjuta_app_instance_init (AnjutaApp *app)
{
	gint merge_id;
	GtkWidget *toolbar_menu, *about_menu;
	GtkWidget *view_menu, *hbox;
	GtkWidget *dockbar;
	GdkGeometry size_hints = {
    	100, 100, 0, 0, 100, 100, 0, 0, 0.0, 0.0, GDK_GRAVITY_NORTH_WEST  
  	};
	
	DEBUG_PRINT ("Initializing Anjuta...");
	
	gtk_window_set_geometry_hints (GTK_WINDOW (app), GTK_WIDGET (app),
								   &size_hints, GDK_HINT_RESIZE_INC);
	gtk_window_set_resizable (GTK_WINDOW (app), TRUE);
	
	gnome_app_enable_layout_config (GNOME_APP (app), FALSE);
	
	app->layout_manager = NULL;
	
	app->values = NULL;
	app->widgets = NULL;

	/* configure dock */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gnome_app_set_contents (GNOME_APP (app), hbox);
	app->dock = gdl_dock_new ();
	gtk_widget_show (app->dock);
	gtk_box_pack_end(GTK_BOX (hbox), app->dock, TRUE, TRUE, 0);
	
	dockbar = gdl_dock_bar_new (GDL_DOCK(app->dock));
	gtk_widget_show (dockbar);
	gtk_box_pack_start(GTK_BOX (hbox), dockbar, FALSE, FALSE, 0);
	
	app->layout_manager = gdl_dock_layout_new (GDL_DOCK (app->dock));
	g_signal_connect (app->layout_manager, "notify::dirty",
					  G_CALLBACK (on_layout_dirty_notify), app);
	g_signal_connect (app->layout_manager->master, "notify::locked",
					  G_CALLBACK (on_layout_locked_notify), app);
	
	/* Create placeholders for default widget positions */
	gdl_dock_placeholder_new ("ph_top", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_TOP, FALSE);
	gdl_dock_placeholder_new ("ph_bottom", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_BOTTOM, FALSE);
	gdl_dock_placeholder_new ("ph_left", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_LEFT, FALSE);
	gdl_dock_placeholder_new ("ph_right", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_RIGHT, FALSE);
	gdl_dock_placeholder_new ("ph_center", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_CENTER, FALSE);
							  
	/* Status bar */
	app->status = ANJUTA_STATUS (anjuta_status_new ());
	gtk_widget_show (GTK_WIDGET (app->status));
	gnome_app_set_statusbar (GNOME_APP (app), GTK_WIDGET (app->status));
	g_object_ref (G_OBJECT (app->status));
	g_object_add_weak_pointer (G_OBJECT (app->status), (gpointer*)&app->status);
	
	/* Preferences */
	app->preferences = ANJUTA_PREFERENCES (anjuta_preferences_new ());
	g_object_add_weak_pointer (G_OBJECT (app->preferences),
							   (gpointer*)&app->preferences);
	anjuta_status_add_widget (ANJUTA_STATUS (app->status),
							  GTK_WIDGET (app->preferences));
	/* UI engine */
	app->ui = anjuta_ui_new ();
	gtk_window_add_accel_group (GTK_WINDOW (app),
								anjuta_ui_get_accel_group (app->ui));
	g_signal_connect (G_OBJECT (app->ui),
					  "add_widget", G_CALLBACK (on_add_merge_widget),
					  app);
	g_object_add_weak_pointer (G_OBJECT (app->ui), (gpointer*)&app->ui);
	
	/* Register actions */
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupFile", _("File"),
										menu_entries_file,
										G_N_ELEMENTS (menu_entries_file), app);
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupEdit", _("Edit"),
										menu_entries_edit,
										G_N_ELEMENTS (menu_entries_edit), app);
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupView", _("View"),
										menu_entries_view,
										G_N_ELEMENTS (menu_entries_view), app);
	anjuta_ui_add_toggle_action_group_entries (app->ui, "ActionGroupToggleView",
										_("Toggle View"),
										menu_entries_toggle_view,
										G_N_ELEMENTS (menu_entries_toggle_view),
										app);
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupSettings",
										_("Settings"),
										menu_entries_settings,
										G_N_ELEMENTS (menu_entries_settings),
										app);
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupHelp", _("Help"),
										menu_entries_help,
										G_N_ELEMENTS (menu_entries_help), app);

	/* Merge UI */
	merge_id = anjuta_ui_merge (app->ui, UI_FILE);

	/* create toolbar menus */
	toolbar_menu =
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(app->ui),
								  "/MenuMain/MenuView/Toolbars");
	if (toolbar_menu)
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (toolbar_menu),
								   app->toolbars_menu);
	else
		g_warning ("Cannot retrive main menu widget");

	/* Create widgets menu */
	view_menu = 
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(app->ui),
								  "/MenuMain/MenuView");
	app->view_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (view_menu));
	
	/* Create about plugins menu */
	about_menu = 
		gtk_ui_manager_get_widget (GTK_UI_MANAGER(app->ui),
								  "/MenuMain/PlaceHolderHelpMenus/MenuHelp/PlaceHolderHelpAbout/AboutPlugins");
	about_create_plugins_submenu (ANJUTA_SHELL (app), about_menu);
							  
	gtk_window_set_transient_for (GTK_WINDOW (app->preferences),
								  GTK_WINDOW (app));
	/*
	gtk_window_add_accel_group (GTK_WINDOW (app->preferences),
								app->accel_group);
	*/
	
	g_signal_connect (G_OBJECT (app), "save_session",
					  G_CALLBACK (on_session_save), app);
	g_signal_connect (G_OBJECT (app), "load_session",
					  G_CALLBACK (on_session_load), app);
}

static void
anjuta_app_class_init (AnjutaAppClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	object_class->finalize = anjuta_app_finalize;
	object_class->dispose = anjuta_app_dispose;
}

GtkWidget *
anjuta_app_new (void)
{
	AnjutaApp *app;

	app = ANJUTA_APP (g_object_new (ANJUTA_TYPE_APP, 
									"title", "Anjuta",
									NULL));
	return GTK_WIDGET (app);
}

gchar*
anjuta_app_get_geometry (AnjutaApp *app)
{
	gchar *geometry;
	gint width, height, posx, posy;
	
	g_return_val_if_fail (ANJUTA_IS_APP (app), NULL);
	
	geometry = NULL;
	width = height = posx = posy = 0;
	if (GTK_WIDGET(app)->window)
	{
		gtk_window_get_size (GTK_WINDOW (app), &width, &height);
		gtk_window_get_position (GTK_WINDOW(app), &posx, &posy);
		
		geometry = g_strdup_printf ("%dx%d+%d+%d", width, height, posx, posy);
	}
	return geometry;
}

void
anjuta_app_set_geometry (AnjutaApp *app, const gchar *geometry)
{
	gint width, height, posx, posy;
	gboolean geometry_set = FALSE;
	
	if (geometry && strlen (geometry) > 0)
	{
		DEBUG_PRINT ("Setting geometry: %s", geometry);
		
		if (sscanf (geometry, "%dx%d+%d+%d", &width, &height,
					&posx, &posy) == 4)
		{
			if (GTK_WIDGET_REALIZED (app))
			{
				gtk_window_resize (GTK_WINDOW (app), width, height);
			}
			else
			{
				gtk_window_set_default_size (GTK_WINDOW (app), width, height);
				gtk_window_move (GTK_WINDOW (app), posx, posy);
			}
			geometry_set = TRUE;
		}
		else
		{
			g_warning ("Failed to parse geometry: %s", geometry);
		}
	}
	if (!geometry_set)
	{
		posx = 10;
		posy = 10;
		width = gdk_screen_width () - 10;
		height = gdk_screen_height () - 25;
		width = (width < 790)? width : 790;
		height = (height < 575)? width : 575;
		if (GTK_WIDGET_REALIZED (app) == FALSE)
		{
			gtk_window_set_default_size (GTK_WINDOW (app), width, height);
			gtk_window_move (GTK_WINDOW (app), posx, posy);
		}
	}
}

static void
anjuta_app_layout_save (AnjutaApp *app, const gchar *filename,
						const gchar *name)
{
	g_return_if_fail (ANJUTA_IS_APP (app));
	g_return_if_fail (filename != NULL);

	gdl_dock_layout_save_layout (app->layout_manager, name);
	if (!gdl_dock_layout_save_to_file (app->layout_manager, filename))
		g_warning ("Saving dock layout to '%s' failed!", filename);
}

static void
anjuta_app_layout_load (AnjutaApp *app, const gchar *layout_filename,
						const gchar *name)
{
	g_return_if_fail (ANJUTA_IS_APP (app));

	if (!layout_filename ||
		!gdl_dock_layout_load_from_file (app->layout_manager, layout_filename))
	{
		gchar *datadir, *filename;
		datadir = anjuta_res_get_data_dir();
		
		filename = g_build_filename (datadir, "layout.xml", NULL);
		DEBUG_PRINT ("Layout = %s", filename);
		g_free (datadir);
		if (!gdl_dock_layout_load_from_file (app->layout_manager, filename))
			g_warning ("Loading layout from '%s' failed!!", filename);
		g_free (filename);
	}
	
	if (!gdl_dock_layout_load_layout (app->layout_manager, name))
		g_warning ("Loading layout failed!!");
}

void
anjuta_app_layout_reset (AnjutaApp *app)
{
	anjuta_app_layout_load (app, NULL, NULL);
}

/* AnjutaShell Implementation */

static void
on_value_removed_from_hash (gpointer value)
{
	g_value_unset ((GValue*)value);
	g_free (value);
}

static void
anjuta_app_add_value (AnjutaShell *shell, const char *name,
					  const GValue *value, GError **error)
{
	GValue *copy;
	AnjutaApp *app;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (name != NULL);
	g_return_if_fail (G_IS_VALUE(value));
	
	app = ANJUTA_APP (shell);
	
	if (app->values == NULL)
	{
		app->values = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, 
											 on_value_removed_from_hash);
	}
	anjuta_shell_remove_value (shell, name, error);
	
	copy = g_new0 (GValue, 1);
	g_value_init (copy, value->g_type);
	g_value_copy (value, copy);

	g_hash_table_insert (app->values, g_strdup (name), copy);
	g_signal_emit_by_name (shell, "value_added", name, copy);
}

static void
anjuta_app_get_value (AnjutaShell *shell, const char *name, GValue *value,
					  GError **error)
{
	GValue *val;
	AnjutaApp *app;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (name != NULL);
	/* g_return_if_fail (G_IS_VALUE (value)); */
	
	app = ANJUTA_APP (shell);
	
	val = NULL;
	if (app->values)
		val = g_hash_table_lookup (app->values, name);
	if (val)
	{
		if (!value->g_type)
		{
			g_value_init (value, val->g_type);
		}
		g_value_copy (val, value);
	}
	else
	{
		if (error)
		{
			*error = g_error_new (ANJUTA_SHELL_ERROR,
								  ANJUTA_SHELL_ERROR_DOESNT_EXIST,
								  _("Value doesn't exist"));
		}
	}
}

static void
anjuta_app_remove_value (AnjutaShell *shell, const char *name, GError **error)
{
	AnjutaApp *app;
	GValue *value;
	char *key;
	
	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (name != NULL);

	app = ANJUTA_APP (shell);
	
	/*
	g_return_if_fail (app->values != NULL);
	if (app->widgets && g_hash_table_lookup_extended (app->widgets, name, 
													  (gpointer*)&key,
													  (gpointer*)&w)) {
		GtkWidget *item;
		item = g_object_get_data (G_OBJECT (w), "dockitem");
		gdl_dock_item_hide_item (GDL_DOCK_ITEM (item));
		gdl_dock_object_unbind (GDL_DOCK_OBJECT (item));
		g_free (key);
	}
	*/
	
	if (app->values && g_hash_table_lookup_extended (app->values, name, 
													 (gpointer*)&key,
													 (gpointer*)&value)) {
		g_signal_emit_by_name (app, "value_removed", name);
		g_hash_table_remove (app->values, name);
	}
}

static gboolean
remove_from_widgets_hash (gpointer name, gpointer hash_widget, gpointer widget)
{
	if (hash_widget == widget)
		return TRUE;
	return FALSE;
}

static void
on_widget_destroy (GtkWidget *widget, AnjutaApp *app)
{
	DEBUG_PRINT ("Widget about to be destroyed");
	g_hash_table_foreach_remove (app->widgets, remove_from_widgets_hash,
								 widget);
}

static void
on_widget_remove (GtkWidget *container, GtkWidget *widget, AnjutaApp *app)
{
	GtkWidget *dock_item;

	dock_item = g_object_get_data (G_OBJECT (widget), "dockitem");
	if (dock_item)
	{
		g_signal_handlers_disconnect_by_func (G_OBJECT (dock_item),
					G_CALLBACK (on_widget_remove), app);
		gdl_dock_item_unbind (GDL_DOCK_ITEM(dock_item));
	}
	if (g_hash_table_foreach_remove (app->widgets,
									 remove_from_widgets_hash,
									 widget)){
		DEBUG_PRINT ("Widget removed from container");
	}
}

static void
on_widget_removed_from_hash (gpointer widget)
{
	AnjutaApp *app;
	GtkWidget *menuitem;
	GdlDockItem *dockitem;
	
	DEBUG_PRINT ("Removing widget from hash");
	
	app = g_object_get_data (G_OBJECT (widget), "app-object");
	dockitem = g_object_get_data (G_OBJECT (widget), "dockitem");
	menuitem = g_object_get_data (G_OBJECT (widget), "menuitem");
	
	gtk_widget_destroy (menuitem);
	
	g_object_set_data (G_OBJECT (widget), "dockitem", NULL);
	g_object_set_data (G_OBJECT (widget), "menuitem", NULL);

	g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
				G_CALLBACK (on_widget_destroy), app);
	g_signal_handlers_disconnect_by_func (G_OBJECT (dockitem),
				G_CALLBACK (on_widget_remove), app);
	
	g_object_unref (G_OBJECT (widget));
}

static void 
anjuta_app_add_widget (AnjutaShell *shell, 
					   GtkWidget *widget,
					   const char *name,
					   const char *title,
					   const char *stock_id,
					   AnjutaShellPlacement placement,
					   GError **error)
{
	AnjutaApp *app;
	GtkWidget *item;
	GtkCheckMenuItem* menuitem;
	GdlDockPlaceholder* placeholder;
	const gchar *placeholder_name;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (name != NULL);
	g_return_if_fail (title != NULL);

	app = ANJUTA_APP (shell);
	
	/*
	anjuta_shell_add (shell, name, G_TYPE_FROM_INSTANCE (widget),
					  widget, NULL);
	*/

	/* Add the widget to hash */
	if (app->widgets == NULL)
	{
		app->widgets = g_hash_table_new_full (g_str_hash, g_str_equal,
											  g_free,
											  on_widget_removed_from_hash);
	}
	g_hash_table_insert (app->widgets, g_strdup (name), widget);
	g_object_ref (widget);
	
	/* Add the widget to dock */
	if (stock_id == NULL)
		item = gdl_dock_item_new (name, title, GDL_DOCK_ITEM_BEH_NORMAL);
	else
		item = gdl_dock_item_new_with_stock (name, title, stock_id,
											 GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item), widget);
	
	if (placement != GDL_DOCK_FLOATING)
	{
		switch (placement)
		{
			case GDL_DOCK_TOP: placeholder_name = "ph_top"; break;
			case GDL_DOCK_BOTTOM: placeholder_name = "ph_bottom"; break;
			case GDL_DOCK_LEFT: placeholder_name = "ph_left"; break;
			case GDL_DOCK_RIGHT: placeholder_name = "ph_right"; break;
			default: placeholder_name = "ph_center";
		}
		
		placeholder = gdl_dock_get_placeholder_by_name (GDL_DOCK (app->dock), 
														placeholder_name);
		gtk_container_add (GTK_CONTAINER (placeholder), GTK_WIDGET (item));
	}
	else
	{
        gdl_dock_add_item (GDL_DOCK (app->dock),
                           GDL_DOCK_ITEM (item), GDL_DOCK_FLOATING);
	}
	gtk_widget_show_all (item);
	
	/* Add toggle button for the widget */
	menuitem = GTK_CHECK_MENU_ITEM (gtk_check_menu_item_new_with_label (title));
	gtk_widget_show (GTK_WIDGET (menuitem));
	gtk_check_menu_item_set_active (menuitem, TRUE);
	gtk_menu_append (GTK_MENU (app->view_menu), GTK_WIDGET (menuitem));
	
	g_object_set_data (G_OBJECT (widget), "app-object", app);
	g_object_set_data (G_OBJECT (widget), "menuitem", menuitem);
	g_object_set_data (G_OBJECT (widget), "dockitem", item);
	
	/* For toggling widget view on/off */
	g_signal_connect (G_OBJECT (menuitem), "toggled",
					  G_CALLBACK (on_toggle_widget_view), item);
	
	/*
	  Watch for widget removal/destruction so that it could be
	  removed from widgets hash.
	*/
	g_signal_connect (G_OBJECT (item), "remove",
					  G_CALLBACK (on_widget_remove), app);
	g_signal_connect_after (G_OBJECT (widget), "destroy",
					  G_CALLBACK (on_widget_destroy), app);
}

static void 
anjuta_app_remove_widget (AnjutaShell *shell, GtkWidget *widget,
						  GError **error)
{
	AnjutaApp *app;
	GtkWidget *dock_item;

	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	
	app = ANJUTA_APP (shell);

	g_return_if_fail (app->widgets != NULL);
	
	dock_item = g_object_get_data (G_OBJECT (widget), "dockitem");
	g_return_if_fail (dock_item != NULL);
	
	/* Following unbind will remove the widget from hash */
	gdl_dock_item_unbind (GDL_DOCK_ITEM (dock_item));
}

static void 
anjuta_app_present_widget (AnjutaShell *shell, GtkWidget *widget,
						   GError **error)
{
	AnjutaApp *app;
	GtkWidget *dock_item;
	GtkWidget *parent;
	
	g_return_if_fail (ANJUTA_IS_APP (shell));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	app = ANJUTA_APP (shell);
	
	g_return_if_fail (app->widgets != NULL);
	
	dock_item = g_object_get_data (G_OBJECT(widget), "dockitem");
	g_return_if_fail (dock_item != NULL);
	
	/* Hack to present the dock item if it's in a notebook dock item */
	parent = gtk_widget_get_parent (dock_item);
	if (GTK_IS_NOTEBOOK (parent))
	{
		gint pagenum;
		pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (parent), dock_item);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (parent), pagenum);
	}
	/* FIXME: If the item is hidden, show it */
	/* FIXME: If the item is floating, present the window */
}

static GObject*
anjuta_app_get_object  (AnjutaShell *shell, const char *iface_name,
					    GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	g_return_val_if_fail (iface_name != NULL, NULL);
	return anjuta_plugins_get_plugin (shell, iface_name);
}

static AnjutaStatus*
anjuta_app_get_status (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->status;
}

static AnjutaUI *
anjuta_app_get_ui  (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->ui;
}

static AnjutaPreferences *
anjuta_app_get_preferences  (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (ANJUTA_IS_APP (shell), NULL);
	return ANJUTA_APP (shell)->preferences;
}

static void
anjuta_shell_iface_init (AnjutaShellIface *iface)
{
	iface->add_widget = anjuta_app_add_widget;
	iface->remove_widget = anjuta_app_remove_widget;
	iface->present_widget = anjuta_app_present_widget;
	iface->add_value = anjuta_app_add_value;
	iface->get_value = anjuta_app_get_value;
	iface->remove_value = anjuta_app_remove_value;
	iface->get_object = anjuta_app_get_object;
	iface->get_status = anjuta_app_get_status;
	iface->get_ui = anjuta_app_get_ui;
	iface->get_preferences = anjuta_app_get_preferences;
}

ANJUTA_TYPE_BEGIN(AnjutaApp, anjuta_app, GNOME_TYPE_APP);
ANJUTA_TYPE_ADD_INTERFACE(anjuta_shell, ANJUTA_TYPE_SHELL);
ANJUTA_TYPE_END;

#if 0 /* FIXME: Implement progress bar in AnjutaShell */
gboolean
anjuta_app_progress_init (AnjutaApp *app, gchar * description,
						  gdouble full_value,
						  GnomeAppProgressCancelFunc cancel_cb,
						  gpointer data)
{
	if (app->in_progress)
		return FALSE;
	app->in_progress = TRUE;
	app->progress_value = 0.0;
	app->progress_full_value = full_value;
	app->progress_cancel_cb = cancel_cb;
	app->progress_key =
		gnome_app_progress_timeout (GNOME_APP (app),
					    description,
					    100,
					    on_anjuta_progress_cb,
					    on_anjuta_progress_cancel, data);
	return TRUE;
}

void
anjuta_app_progress_set (AnjutaApp *app, gdouble value)
{
	if (app->in_progress)
		app->progress_value = value / app->progress_full_value;
}

void
anjuta_app_progress_done (AnjutaApp *app, gchar * end_mesg)
{
	if (app->in_progress)
		gnome_app_progress_done (app->progress_key);
	anjuta_status (end_mesg);
	app->in_progress = FALSE;
}
#endif
