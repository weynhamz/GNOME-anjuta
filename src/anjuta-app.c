/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta.c Copyright (C) 2000 Naba Kumar
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
#include <libanjuta/pixmaps.h>
#include <libanjuta/anjuta-stock.h>
#include <libanjuta/plugins.h>

#include "anjuta-app.h"
#include "anjuta-callbacks.h"
#include "anjuta-actions.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta.ui"

// static GdkCursor *app_cursor;

static gboolean anjuta_app_save_layout_to_file (AnjutaApp *app);
static void on_toggle_widget_view (GtkCheckMenuItem *menuitem, GtkWidget *dockitem);

gpointer parent_class;

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = anjuta_res_get_pixbuf (icon); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	//g_object_unref (pixbuf);

static void
create_stock_icons (AnjutaUI *ui)
{
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	
	icon_factory = anjuta_ui_get_icon_factory (ui);
	
//	REGISTER_ICON (ANJUTA_PIXMAP_PROJECT, ANJUTA_STOCK_PROJECT);
//	REGISTER_ICON (ANJUTA_PIXMAP_OPEN_PROJECT, ANJUTA_STOCK_OPEN_PROJECT);
//	REGISTER_ICON (ANJUTA_PIXMAP_CLOSE_PROJECT, ANJUTA_STOCK_CLOSE_PROJECT);
//	REGISTER_ICON (ANJUTA_PIXMAP_SAVE_PROJECT, ANJUTA_STOCK_SAVE_PROJECT);
	REGISTER_ICON (ANJUTA_PIXMAP_COMPILE, ANJUTA_STOCK_COMPILE);
	REGISTER_ICON (ANJUTA_PIXMAP_BUILD, ANJUTA_STOCK_BUILD);
	REGISTER_ICON (ANJUTA_PIXMAP_BUILD_ALL, ANJUTA_STOCK_BUILD_ALL);
	REGISTER_ICON (ANJUTA_PIXMAP_CONFIGURE, ANJUTA_STOCK_CONFIGURE);
	REGISTER_ICON (ANJUTA_PIXMAP_DEBUG, ANJUTA_STOCK_DEBUG);
}

static void 
layout_dirty_notify (GObject    *object,
		     GParamSpec *pspec,
		     gpointer    user_data)
{
	if (!strcmp (pspec->name, "dirty")) {
		gboolean dirty;
		g_object_get (object, "dirty", &dirty, NULL);
		if (dirty) {
			/* user_data is the AnjutaApp */
			g_idle_add (
				(GSourceFunc) anjuta_app_save_layout_to_file,
				user_data);
			
		}
	}
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
#ifdef DEBUG
	g_message ("Adding UI item ...");
#endif
	if (GTK_IS_MENU_BAR (widget))
	{
		gnome_app_set_menus (GNOME_APP (ui_container), GTK_MENU_BAR (widget));
		gtk_widget_show (widget);
		// create_recent_files ();
	}
	else
	{
		static gint count = 0;
		const gchar *toolbarname;
		gchar* key;
		AnjutaPreferences *pr;
		GtkAction *action;
		GtkWidget *menuitem;
		
		gtk_toolbar_set_icon_size (GTK_TOOLBAR (widget),
								   GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_toolbar_set_show_arrow (GTK_TOOLBAR (widget), FALSE);
		
		toolbarname = gtk_widget_get_name (widget);
		g_message ("Adding toolbar: %s", toolbarname);
		gtk_widget_show (widget);
		g_object_set_data (G_OBJECT (widget), "app", ui_container);
		g_object_set_data (G_OBJECT (widget), "band", GINT_TO_POINTER(count+1));
		
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

		// Show/hide toolbar
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
	//else
	//	g_warning ("Unknow UI widget: Can not add in container");
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

/* FIXME: Something doesnt work here */
static void
on_dock_item_show (GtkWidget *dockitem, GtkCheckMenuItem *menuitem)
{
//	g_signal_handlers_block_by_func (G_OBJECT (menuitem),
//									 on_toggle_widget_view, dockitem);
	gtk_check_menu_item_set_active (menuitem, TRUE);
//	g_signal_handlers_unblock_by_func (G_OBJECT (menuitem),
//									   on_toggle_widget_view, dockitem);
}

static void
on_dock_item_hide (GtkWidget *dockitem, GtkCheckMenuItem *menuitem)
{
//	g_signal_handlers_block_by_func (G_OBJECT (menuitem),
//									 on_toggle_widget_view, dockitem);
	gtk_check_menu_item_set_active (menuitem, FALSE);
//	g_signal_handlers_unblock_by_func (G_OBJECT (menuitem),
//									   on_toggle_widget_view, dockitem);
}

static void
on_toggle_widget_view (GtkCheckMenuItem *menuitem, GtkWidget *dockitem)
{
	gboolean state;
	state = gtk_check_menu_item_get_active (menuitem);
	if (state)
	{
		g_signal_handlers_block_by_func (G_OBJECT (dockitem),
										 on_dock_item_show, menuitem);
		gtk_widget_show (dockitem);
		g_signal_handlers_unblock_by_func (G_OBJECT (dockitem),
										   on_dock_item_show, menuitem);
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (dockitem),
										 on_dock_item_hide, menuitem);
		gtk_widget_hide (dockitem);
		g_signal_handlers_unblock_by_func (G_OBJECT (dockitem),
										   on_dock_item_hide, menuitem);
	}
}

static void
on_add_widget_to_menu (gpointer key, gpointer value, gpointer user_data)
{
	const gchar *name = key;
	GtkWidget *wid = value;
	AnjutaApp *app = user_data;
	GtkCheckMenuItem *menuitem;
	gpointer dockitem;
	
	menuitem = GTK_CHECK_MENU_ITEM (gtk_check_menu_item_new_with_label (name));
	gtk_widget_show (GTK_WIDGET (menuitem));
	gtk_check_menu_item_set_active (menuitem, TRUE);
	gtk_menu_append (GTK_MENU (app->view_menu), GTK_WIDGET (menuitem));
	dockitem = g_object_get_data (G_OBJECT (wid), "dockitem");
	
	g_signal_connect (G_OBJECT (menuitem), "toggled",
					  G_CALLBACK (on_toggle_widget_view), dockitem);
	g_signal_connect (G_OBJECT (dockitem), "show",
					  G_CALLBACK (on_dock_item_show), menuitem);
	g_signal_connect (G_OBJECT (dockitem), "hide",
					  G_CALLBACK (on_dock_item_hide), menuitem);
}

static void
anjuta_app_instance_init (AnjutaApp *app)
{
	gint merge_id;
	GtkWidget *toolbar_menu;
	GtkWidget *view_menu, *hbox;
	GtkWidget *dockbar;
	
	g_message ("Initializing Anjuta...");
	gnome_app_enable_layout_config (GNOME_APP (app), FALSE);
	
	app->layout_manager = NULL;
	app->values = g_hash_table_new (g_str_hash, g_str_equal);
	app->widgets = g_hash_table_new (g_str_hash, g_str_equal);

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
	
	/* create placeholders for default widget positions (since an
	   initial host is provided they are automatically bound) */
	gdl_dock_placeholder_new ("ph_top", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_TOP, FALSE);
	gdl_dock_placeholder_new ("ph_bottom", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_BOTTOM, FALSE);
	gdl_dock_placeholder_new ("ph_left", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_LEFT, FALSE);
	gdl_dock_placeholder_new ("ph_right", GDL_DOCK_OBJECT (app->dock),
							  GDL_DOCK_RIGHT, FALSE);

	//gtk_widget_realize (GTK_WIDGET(app));
	
	//gtk_widget_queue_draw (GTK_WIDGET(app));
	//gtk_widget_queue_resize (GTK_WIDGET(app));
	
	/* Preferencesnces */
	app->preferences = ANJUTA_PREFERENCES (anjuta_preferences_new ());
	anjuta_preferences_initialize (app->preferences);

	/* UI engine */
	app->ui = anjuta_ui_new ();
	g_signal_connect (G_OBJECT (app->ui),
					  "add_widget", G_CALLBACK (on_add_merge_widget),
					  app);
	
	/* Create stock icons */
	create_stock_icons (app->ui);

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
	anjuta_ui_add_action_group_entries (app->ui, "ActionGroupSettings", _("Settings"),
										menu_entries_settings,
										G_N_ELEMENTS (menu_entries_settings), app);
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
	// g_object_set_data (G_OBJECT (app), "view_menu", app->view_menu);
	g_hash_table_foreach (app->widgets, on_add_widget_to_menu, app);
	
	gtk_window_set_transient_for (GTK_WINDOW (app->preferences),
								  GTK_WINDOW (app));
	// gtk_window_add_accel_group (GTK_WINDOW (app->preferences),
	//							app->accel_group);
	// g_signal_connect (G_OBJECT (app->preferences), "changed",
	//				  G_CALLBACK (anjuta_apply_preferences), app);

	app->shutdown_in_progress = FALSE;
	app->win_pos_x = 10;
	app->win_pos_y = 10;
	app->win_width = gdk_screen_width () - 10;
	app->win_height = gdk_screen_height () - 25;
	app->win_width = (app->win_width < 790)? app->win_width : 790;
	app->win_height = (app->win_height < 575)? app->win_width : 575;
		
	app->in_progress = FALSE;
	app->busy_count = 0;

	// app->icon_set = gdl_icons_new(24, 16.0);

	gtk_widget_set_uposition (GTK_WIDGET (app), app->win_pos_x, app->win_pos_y);
	gtk_window_set_default_size (GTK_WINDOW (app), app->win_width,
								 app->win_height);
	// anjuta_apply_preferences(ANJUTA_PREFERENCES (app->preferences), app);
	
	/* Load layout */
	// anjuta_app_load_layout (app, NULL);
}

static void
anjuta_app_add_value (AnjutaShell *shell,
			 const char *name,
			 const GValue *value,
			 GError **error)
{
	GValue *copy;
	AnjutaApp *window = ANJUTA_APP (shell);

	anjuta_shell_remove_value (shell, name, error);
	
	copy = g_new0 (GValue, 1);
	g_value_init (copy, value->g_type);
	g_value_copy (value, copy);

	g_hash_table_insert (window->values, g_strdup (name), copy);
	g_signal_emit_by_name (shell, "value_added", name, copy);
}

static void
anjuta_app_get_value (AnjutaShell *shell,
			 const char *name,
			 GValue *value,
			 GError **error)
{
	GValue *val;
	AnjutaApp *app = ANJUTA_APP (shell);
	
	val = g_hash_table_lookup (app->values, name);
	
	if (val) {
		if (!value->g_type) {
			g_value_init (value, val->g_type);
		}
		g_value_copy (val, value);
	} else {
		if (error) {
			*error = g_error_new (ANJUTA_SHELL_ERROR,
					      ANJUTA_SHELL_ERROR_DOESNT_EXIST,
					      _("Value doesn't exist"));
		}
	}
}

static void 
anjuta_app_add_widget (AnjutaShell *shell, 
					   GtkWidget *w, 
					   const char *name,
					   const char *title,
					   const char *stock_id,
					   AnjutaShellPlacement placement,
					   GError **error)
{
	AnjutaApp *window = ANJUTA_APP (shell);
	GtkWidget *item;

	g_return_if_fail (w != NULL);

	anjuta_shell_add (shell, name, G_TYPE_FROM_INSTANCE (w), w, NULL);

	g_hash_table_insert (window->widgets, g_strdup (name), w);

	if (stock_id == NULL)
		item = gdl_dock_item_new (name, title, GDL_DOCK_ITEM_BEH_NORMAL);
	else
		item = gdl_dock_item_new_with_stock (name, title, stock_id,
											 GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item), w);
	g_object_set_data (G_OBJECT (w), "dockitem", item);
	gdl_dock_add_item (GDL_DOCK (window->dock), 
					   GDL_DOCK_ITEM (item), (GdlDockPlacement)placement);
	on_add_widget_to_menu ((gpointer)title, w, window);
	gtk_widget_show_all (item);	
}

static gboolean
remove_from_widgets_hash (gpointer key, gpointer value, gpointer data)
{
	if (value == data) {
		g_free (key);
		return TRUE;
	}
	return FALSE;
}

static void 
anjuta_app_remove_widget (AnjutaShell *shell, 
						  GtkWidget *w, 
						  GError **error)
{
	AnjutaApp *window = ANJUTA_APP (shell);
	GtkWidget *dock_item;

	g_return_if_fail (w != NULL);

	g_hash_table_foreach_steal (window->widgets, remove_from_widgets_hash, w);

	dock_item = g_object_get_data (G_OBJECT(w), "dockitem");
	g_return_if_fail (dock_item != NULL);
	gdl_dock_item_unbind(GDL_DOCK_ITEM(dock_item));
}

static void
anjuta_app_remove_value (AnjutaShell *shell, 
			    const char *name, 
			    GError **error)
{
	AnjutaApp *window = ANJUTA_APP (shell);
	GValue *value;
	GtkWidget *w;
	char *key;
	
	if (g_hash_table_lookup_extended (window->widgets, name, 
					  (gpointer*)&key, (gpointer*)&w)) {
		GtkWidget *item;
		item = g_object_get_data (G_OBJECT (w), "dockitem");
		gdl_dock_item_hide_item (GDL_DOCK_ITEM (item));
		gdl_dock_object_unbind (GDL_DOCK_OBJECT (item));
		g_free (key);
	}
	
	if (g_hash_table_lookup_extended (window->values, name, 
					  (gpointer*)&key, (gpointer*)&value)) {
		g_hash_table_remove (window->values, name);
		g_signal_emit_by_name (window, "value_removed", name);
		g_value_unset (value);
		g_free (value);
	}
}

static GObject*
anjuta_app_get_object  (AnjutaShell *shell, const char *iface_name,
					    GError **error)
{
	return anjuta_plugins_get_plugin (shell, iface_name);
}

static AnjutaUI *
anjuta_app_get_ui  (AnjutaShell *shell, GError **error)
{
	return ANJUTA_APP (shell)->ui;
}

static AnjutaPreferences *
anjuta_app_get_preferences  (AnjutaShell *shell, GError **error)
{
	return ANJUTA_APP (shell)->preferences;
}

static void
ensure_layout_manager (AnjutaApp *window)
{
	gchar *filename;

	if (!window->layout_manager) {
		/* layout manager */
		window->layout_manager = gdl_dock_layout_new (GDL_DOCK (window->dock));
		
		/* FIXME: Always load system layout for now */
		/* load xml layout definitions */
		/* filename = gnome_util_prepend_user_home (".anjuta/layout.xml");
		g_message ("Layout = %s", filename);
		if (!gdl_dock_layout_load_from_file (window->layout_manager, filename)) */{
			gchar *datadir;
			datadir = anjuta_res_get_data_dir();
			/* g_free (filename); */
			filename = g_build_filename (datadir, "/layout.xml", NULL);
			g_message ("Layout = %s", filename);
			g_free (datadir);
			if (!gdl_dock_layout_load_from_file (window->layout_manager, filename))
				g_warning ("Loading layout from failed!!");
		}
		g_free (filename);
		
		g_signal_connect (window->layout_manager, "notify::dirty",
				  (GCallback) layout_dirty_notify, window);
	}
}

static gboolean
anjuta_app_save_layout_to_file (AnjutaApp *window)
{
	char *dir;
	char *filename;

	g_message ("Saving Layout ... ");
	dir = gnome_util_prepend_user_home (".anjuta");
	if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) {
		mkdir (dir, 0755);
		if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) {
			anjuta_util_dialog_error (GTK_WINDOW (window),
									  "Could not create .anjuta directory.");
			return FALSE;
		}
	}
	g_free (dir);

	ensure_layout_manager (window);
	
	filename = gnome_util_prepend_user_home (".anjuta/layout.xml");
	if (!gdl_dock_layout_save_to_file (window->layout_manager, filename))
		anjuta_util_dialog_error (GTK_WINDOW (window),
								  "Could not save layout.");
	g_free (filename);
	return FALSE;
}

void
anjuta_app_save_layout (AnjutaApp *window, const gchar *name)
{
	g_return_if_fail (window != NULL);

	ensure_layout_manager (window);
	g_message ("Saving layout ... ");
	gdl_dock_layout_save_layout (window->layout_manager, name);
	anjuta_app_save_layout_to_file (window);
}

void
anjuta_app_load_layout (AnjutaApp *window, const gchar *name)
{
	g_return_if_fail (window != NULL);

	ensure_layout_manager (window);
	if (!gdl_dock_layout_load_layout (window->layout_manager, name))
		g_warning ("Loading layout failed!!");
}

static void
anjuta_app_dispose (GObject *widget)
{
	AnjutaApp *window;
	// GtkAllocation *alloc;
	// AnjutaWindowState *state;
	// AnjutaSession *session;

	g_assert (ANJUTA_IS_APP (widget));
	window = ANJUTA_APP (widget);

	if (window->layout_manager) {
		g_object_unref (window->layout_manager);
		window->layout_manager = NULL;
	};
}

static void
anjuta_app_show (GtkWidget *wid)
{
	AnjutaApp *app;
	// PropsID pr;
	GtkAction *action;

	app = ANJUTA_APP (wid);
	GNOME_CALL_PARENT(GTK_WIDGET_CLASS, show, (wid));
	
	// pr = ANJUTA_PREFERENCES (app->preferences)->props;

	//start_with_dialog_show (GTK_WINDOW (app),
	//						app->preferences, FALSE);
}

static void
anjuta_app_class_init (AnjutaAppClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	object_class->dispose = anjuta_app_dispose;
	//	widget_class->show = anjuta_app_show;
}

static void
anjuta_shell_iface_init (AnjutaShellIface *iface)
{
	iface->add_widget = anjuta_app_add_widget;
	iface->remove_widget = anjuta_app_remove_widget;
	iface->add_value = anjuta_app_add_value;
	iface->get_value = anjuta_app_get_value;
	iface->remove_value = anjuta_app_remove_value;
	iface->get_object = anjuta_app_get_object;
	iface->get_ui = anjuta_app_get_ui;
	iface->get_preferences = anjuta_app_get_preferences;
}

ANJUTA_TYPE_BEGIN(AnjutaApp, anjuta_app, GNOME_TYPE_APP);
ANJUTA_INTERFACE(anjuta_shell, ANJUTA_TYPE_SHELL);
ANJUTA_TYPE_END;

/****************************************/
#if 0

void
anjuta_session_restore (GnomeClient* client)
{
	anjuta_load_cmdline_files();
}

void
anjuta_app_save_settings (AnjutaApp *app)
{
	gchar *buffer;
	FILE *stream;

	g_return_if_fail (ANJUTA_IS_APP (app));
	
	buffer = g_strconcat (app->dirs->settings, "/session.properties", NULL);
	stream = fopen (buffer, "w");
	g_free (buffer);
	if (stream)
	{
		anjuta_save_yourself (stream);
		fclose (stream);
	}
}

gboolean
anjuta_app_save_yourself (AnjutaApp *app, FILE * stream)
{
	PropsID pr;
	gchar* key;

	pr = ANJUTA_PREFERENCES (app->preferences)->props;

	gdk_window_get_root_origin (GTK_WIDGET (app)->window,
							    &app->win_pos_x, &app->win_pos_y);
	gdk_window_get_size (GTK_WIDGET (app)->window, &app->win_width,
					     &app->win_height);
	fprintf (stream,
		 _("# * DO NOT EDIT OR RENAME THIS FILE ** Generated by Anjuta **\n"));
	fprintf (stream, "anjuta.version=%s\n", VERSION);
	fprintf (stream, "anjuta.win.pos.x=%d\n", app->win_pos_x);
	fprintf (stream, "anjuta.win.pos.y=%d\n", app->win_pos_y);
	fprintf (stream, "anjuta.win.width=%d\n", app->win_width);
	fprintf (stream, "anjuta.win.height=%d\n", app->win_height);

	key = g_strconcat (ANJUTA_MAIN_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_EXTENDED_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_FORMAT_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_DEBUG_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	key = g_strconcat (ANJUTA_BROWSER_TOOLBAR, ".visible", NULL);
	fprintf (stream, "%s=%d\n", key,
		anjuta_preferences_get_int_with_default (ANJUTA_PREFERENCES
												 (app->preferences), key, 1));
	g_free (key);

	fprintf (stream, "view.eol=%d\n", prop_get_int (pr, "view.eol", 0));
	fprintf (stream, "view.indentation.guides=%d\n",
		 prop_get_int (pr, "view.indentation.guides", 0));
	fprintf (stream, "view.whitespace=%d\n",
		 prop_get_int (pr, "view.whitespace", 0));
	fprintf (stream, "view.indentation.whitespace=%d\n",
		 prop_get_int (pr, "view.indentation.whitespace", 0));
	fprintf (stream, "view.line.wrap=%d\n",
		 prop_get_int (pr, "view.line.wrap", 1));
	fprintf (stream, "margin.linenumber.visible=%d\n",
		 prop_get_int (pr, "margin.linenumber.visible", 0));
	fprintf (stream, "margin.marker.visible=%d\n",
		 prop_get_int (pr, "margin.marker.visible", 0));
	fprintf (stream, "margin.fold.visible=%d\n",
		 prop_get_int (pr, "margin.fold.visible", 0));
	fprintf (stream, "anjuta.recent.files=");
	node = app->recent_files;
	while (node)
	{
		fprintf (stream, "\\\n%s ", (gchar *) node->data);
		node = g_list_next (node);
	}
	
	fprintf (stream, "\n\n");

	fprintf (stream, "anjuta.recent.projects=");
	node = app->recent_projects;
	while (node)
	{
		fprintf (stream, "\\\n%s ", (gchar *) node->data);
		node = g_list_next (node);
	}

	fprintf (stream, "\n\n");
	fprintf (stream, "%s=", ANJUTA_LAST_OPEN_PROJECT );
	anjuta_preferences_save (ANJUTA_PREFERENCES (app->preferences), stream);
	return TRUE;
}

gboolean
anjuta_app_load_yourself (AnjutaApp *app, PropsID pr)
{
	app->win_pos_x = prop_get_int (pr, "anjuta.win.pos.x", app->win_pos_x);
	app->win_pos_y = prop_get_int (pr, "anjuta.win.pos.y", app->win_pos_y);
	app->win_width = prop_get_int (pr, "anjuta.win.width", app->win_width);
	app->win_height = prop_get_int (pr, "anjuta.win.height", app->win_height);
	return TRUE;
}

void
anjuta_app_apply_preferences (AnjutaApp *app, AnjutaPreferences *pr)
{
	gint i;
	gint no_tag;
	gint show_tooltips;

	show_tooltips = anjuta_preferences_get_int (pr, SHOW_TOOLTIPS);
	show_hide_tooltips(show_tooltips);
}

void
anjuta_app_set_status (AnjutaApp *app, gchar * mesg, ...)
{
	gchar* message;
	va_list args;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gnome_app_flash (GNOME_APP (app), message);
	g_free(message);
}

gboolean
anjuta_app_progress_init (AnjutaApp *app, gchar * description, gdouble full_value,
		      GnomeAppProgressCancelFunc cancel_cb, gpointer data)
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

void
anjuta_app_set_busy (AnjutaApp *app)
{
	app->busy_count++;
	if (app->busy_count > 1)
		return;
	if (app_cursor)
		gdk_cursor_destroy (app_cursor);
	app_cursor = gdk_cursor_new (GDK_WATCH);
	if (GTK_WIDGET (app)->window)
		gdk_window_set_cursor (GTK_WIDGET (app)->window, app_cursor);
	gdk_flush ();
}

void
anjuta_app_set_active (AnjutaApp *app)
{
	app->busy_count--;
	if (app->busy_count > 0)
		return;
	app->busy_count = 0;
	if (app_cursor)
		gdk_cursor_destroy (app_cursor);
	app_cursor = gdk_cursor_new (GDK_TOP_LEFT_ARROW);
	gdk_window_set_cursor (GTK_WIDGET (app)->window, app_cursor);
}

void
anjuta_app_toolbar_set_view (AnjutaApp *app, gchar* toolbar_name, gboolean view,
							 gboolean resize, gboolean set_in_props)
{
	gchar* key;
	BonoboDock* dock;
	BonoboDockItem* item;

	if (set_in_props)
	{
		key = g_strconcat (toolbar_name, ".visible", NULL);
		anjuta_preferences_set_int (ANJUTA_PREFERENCES (app->preferences),
									key, view);
		g_free (key);
	}

	item = gnome_app_get_dock_item_by_name (GNOME_APP (app), toolbar_name);

	g_return_if_fail(toolbar_name != NULL);
	g_return_if_fail (item != NULL);
	g_return_if_fail (BONOBO_IS_DOCK_ITEM (item));

	if (view)
	{
		gtk_widget_show(GTK_WIDGET (item));
		gtk_widget_show(GTK_BIN(item)->child);
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET (item));
		gtk_widget_hide(GTK_BIN(item)->child);
	}
	dock = gnome_app_get_dock(GNOME_APP(app));
	g_return_if_fail (dock != NULL);
	if (resize)
		gtk_widget_queue_resize (GTK_WIDGET (dock));
}

void
anjuta_load_this_project( const gchar * szProjectPath )
{
	/* Give the use a chance to save an existing project */
	if (app->project_dbase->project_is_open)
	{
		project_dbase_close_project (app->project_dbase);
	}
	if( app->project_dbase->project_is_open )
		return ;
	fileselection_set_filename (app->project_dbase->fileselection_open,
								(gchar*)szProjectPath);
	project_dbase_load_project (app->project_dbase, 
								(const gchar*)szProjectPath, TRUE);
}

void
anjuta_open_project( )
{
	/* Give the use a chance to save an existing project */
	if (app->project_dbase->project_is_open)
	{
		project_dbase_close_project (app->project_dbase);
	}
	if( app->project_dbase->project_is_open )
		return ;
	project_dbase_open_project (app->project_dbase);
}

void 
anjuta_load_last_project()
{
	//printf ("anjuta_load_last_project");
	
	if ((NULL != app->last_open_project) 
		&&	strlen (app->last_open_project))
	{
#ifdef	DEBUG_LOAD_PROJECTS	/* debug code, may return useful */
		char	szppp[1024];
		sprintf (szppp, "%d, >>%s<<", strlen (app->last_open_project),
				 app->last_open_project );
		MessageBox (szppp);
#endif
		anjuta_load_this_project( app->last_open_project );
	}
}

typedef struct _order_struct order_struct;
struct _order_struct
{
	gchar *m_label;
	GtkWidget *m_widget;
};


void anjuta_search_sources_for_symbol(const gchar *s)
{
	gchar command[BUFSIZ];

	if ((NULL == s) || (isspace(*s) || ('\0' == *s)))
		return;

	// exclude my own tag and backup files
	snprintf(command, BUFSIZ, "grep -FInHr --exclude=\"CVS\" --exclude=\"tm.tags\" --exclude=\"tags.cache\" --exclude=\".*\" --exclude=\"*~\" --exclude=\"*.o\" '%s' %s", s,
			 project_dbase_get_dir(app->project_dbase));
	
	g_signal_connect (app->launcher, "child-exited",
					  G_CALLBACK (find_in_files_terminated), NULL);
	
	if (anjuta_launcher_execute (app->launcher, command,
								 find_in_files_mesg_arrived, NULL) == FALSE)
		return;
	
	anjuta_update_app_status (TRUE, _("Looking up symbol"));
	an_message_manager_clear (app->messages, MESSAGE_FIND);
	an_message_manager_append (app->messages, _("Finding in Files ....\n"),
							   MESSAGE_FIND);
	an_message_manager_show (app->messages, MESSAGE_FIND);
}
#endif
