/*
    anjuta1.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libegg/menu/egg-accel-dialog.h>
#include <libegg/menu/egg-entry-action.h>
#include <libegg/toolbar/eggtoolbar.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/pixmaps.h>
#include <libanjuta/resources.h>

#include "anjuta.h"
#include "main_menubar.h"
#include "dnd.h"
#include "toolbar_callbacks.h"
#include "mainmenu_callbacks.h"
#include "search-replace.h"
#include "fileselection.h"
#include "widget-registry.h"
#include "print.h"
#include "lexer.h"
#include "anjuta-actions.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta.ui"

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
	
	REGISTER_ICON (ANJUTA_PIXMAP_UNDOCK, ANJUTA_STOCK_UNDOCK);
	REGISTER_ICON (ANJUTA_PIXMAP_PROJECT, ANJUTA_STOCK_PROJECT);
	REGISTER_ICON (ANJUTA_PIXMAP_MESSAGES, ANJUTA_STOCK_MESSAGES);
	REGISTER_ICON (ANJUTA_PIXMAP_BOOKMARK_TOGGLE, ANJUTA_STOCK_TOGGLE_BOOKMARK);
	REGISTER_ICON (ANJUTA_PIXMAP_ERROR_NEXT, ANJUTA_STOCK_NEXT_MESSAGE);
	REGISTER_ICON (ANJUTA_PIXMAP_ERROR_PREV, ANJUTA_STOCK_PREV_MESSAGE);
	REGISTER_ICON (ANJUTA_PIXMAP_BLOCK_START, ANJUTA_STOCK_BLOCK_START);
	REGISTER_ICON (ANJUTA_PIXMAP_BLOCK_END, ANJUTA_STOCK_BLOCK_END);
	REGISTER_ICON (ANJUTA_PIXMAP_OPEN_PROJECT, ANJUTA_STOCK_OPEN_PROJECT);
	REGISTER_ICON (ANJUTA_PIXMAP_CLOSE_PROJECT, ANJUTA_STOCK_CLOSE_PROJECT);
	REGISTER_ICON (ANJUTA_PIXMAP_SAVE_PROJECT, ANJUTA_STOCK_SAVE_PROJECT);
	REGISTER_ICON (ANJUTA_PIXMAP_COMPILE, ANJUTA_STOCK_COMPILE);
	REGISTER_ICON (ANJUTA_PIXMAP_BUILD, ANJUTA_STOCK_BUILD);
	REGISTER_ICON (ANJUTA_PIXMAP_BUILD_ALL, ANJUTA_STOCK_BUILD_ALL);
	REGISTER_ICON (ANJUTA_PIXMAP_CONFIGURE, ANJUTA_STOCK_CONFIGURE);
	REGISTER_ICON (ANJUTA_PIXMAP_DEBUG, ANJUTA_STOCK_DEBUG);
	REGISTER_ICON (ANJUTA_PIXMAP_FOLD_TOGGLE, ANJUTA_STOCK_FOLD_TOGGLE);
	REGISTER_ICON (ANJUTA_PIXMAP_FOLD_OPEN, ANJUTA_STOCK_FOLD_OPEN);
	REGISTER_ICON (ANJUTA_PIXMAP_FOLD_CLOSE, ANJUTA_STOCK_FOLD_CLOSE);
	REGISTER_ICON (ANJUTA_PIXMAP_BLOCK_SELECT, ANJUTA_STOCK_BLOCK_SELECT);
	REGISTER_ICON (ANJUTA_PIXMAP_INDENT_INC, ANJUTA_STOCK_INDENT_INC);
	REGISTER_ICON (ANJUTA_PIXMAP_INDENT_DCR, ANJUTA_STOCK_INDENT_DCR);
	REGISTER_ICON (ANJUTA_PIXMAP_INDENT_AUTO, ANJUTA_STOCK_INDENT_AUTO);
	REGISTER_ICON (ANJUTA_PIXMAP_AUTOFORMAT_SETTING, ANJUTA_STOCK_AUTOFORMAT_SETTINGS);
	REGISTER_ICON (ANJUTA_PIXMAP_AUTOCOMPLETE, ANJUTA_STOCK_AUTOCOMPLETE);
}

static void
create_anjuta_ui (AnjutaUI *ui, GtkWidget *app)
{
	gint merge_id;
	EggActionGroup *group;
	EggAction * action;

	create_stock_icons (ui);	
	anjuta_ui_add_action_group_entries (ui, "ActionGroupFile", _("File"),
										menu_entries_file,
										G_N_ELEMENTS (menu_entries_file));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEdit", _("Edit"),
										menu_entries_edit,
										G_N_ELEMENTS (menu_entries_edit));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupNavigate", _("Navigate"),
										menu_entries_navigation,
										G_N_ELEMENTS (menu_entries_navigation));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupView", _("View"),
										menu_entries_view,
										G_N_ELEMENTS (menu_entries_view));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupProject", _("Project"),
										menu_entries_project,
										G_N_ELEMENTS (menu_entries_project));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupBuild", _("Build"),
										menu_entries_build,
										G_N_ELEMENTS (menu_entries_build));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSettings", _("Settings"),
										menu_entries_settings,
										G_N_ELEMENTS (menu_entries_settings));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupHelp", _("Help"),
										menu_entries_help,
										G_N_ELEMENTS (menu_entries_help));
	
	merge_id = anjuta_ui_merge (ui, UI_FILE);
	gtk_window_add_accel_group (GTK_WINDOW (app),
								anjuta_ui_get_accel_group (ui));
}

static void
create_recent_files ()
{
	GtkWidget *menu;
	GtkWidget *submenu =
		create_submenu (_("Recent Files    "), app->recent_files,
				GTK_SIGNAL_FUNC
				(on_recent_files_menu_item_activate));
	menu = egg_menu_merge_get_widget (anjuta_ui_get_menu_merge (app->ui),
									  "/MenuMain/MenuFile/RecentFiles");
	if (GTK_IS_MENU_ITEM (menu))
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu), submenu);
	
	submenu =
		create_submenu (_("Recent Projects "), app->recent_projects,
				GTK_SIGNAL_FUNC
				(on_recent_projects_menu_item_activate));
	menu = egg_menu_merge_get_widget (anjuta_ui_get_menu_merge (app->ui),
									  "/MenuMain/MenuFile/RecentProjects");
	if (GTK_IS_MENU_ITEM (menu))
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu), submenu);
}

static void
on_add_merge_widget (GtkWidget *merge, GtkWidget *widget,
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
		static gchar *toolbar_name[] =
		{
			ANJUTA_MAIN_TOOLBAR,
			ANJUTA_BROWSER_TOOLBAR,
			ANJUTA_EXTENDED_TOOLBAR,
			ANJUTA_FORMAT_TOOLBAR,
			ANJUTA_DEBUG_TOOLBAR,
			NULL,
		};
		static gchar *action_name[] =
		{
			"ActionViewToolbarMain",
			"ActionViewToolbarBrowser",
			"ActionViewToolbarExtended",
			"ActionViewToolbarFormat",
			"ActionViewToolbarDebug",
			NULL,
		};
		static gint count = 0;
		gchar *toolbarname;
		BonoboDockItem* item;
		gchar* key;
		PropsID pr;
		EggAction *action;
		egg_toolbar_set_icon_size (EGG_TOOLBAR (widget), GTK_ICON_SIZE_SMALL_TOOLBAR);
 		//egg_toolbar_set_show_arrow (EGG_TOOLBAR (widget), TRUE);
		if (count < 5)
		{
			toolbarname = g_strdup (toolbar_name[count]);
		}
		else
		{
			toolbarname = g_strdup_printf ("toolbar%d", count + 1);
		}
		g_message ("Adding toolbar: %s", toolbarname);
		gnome_app_add_docked (GNOME_APP (ui_container), widget,
							  toolbarname,
							  BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL,
							  BONOBO_DOCK_TOP, count + 1, 0, 0);
		
		// Show/hide toolbar
		gtk_widget_show (widget);
		if (count < 5)
		{
			pr = ANJUTA_PREFERENCES (app->preferences)->props;
			key = g_strconcat (toolbarname, ".visible", NULL);
			action = anjuta_ui_get_action (app->ui, "ActionGroupView",
										   action_name[count]);
			egg_toggle_action_set_active (EGG_TOGGLE_ACTION (action),
										  prop_get_int (pr, key, 1));
			g_free (key);
		} else {
			item = gnome_app_get_dock_item_by_name (GNOME_APP (app),
													toolbarname);
			gtk_widget_show (GTK_WIDGET (item));
			gtk_widget_show (GTK_BIN(item)->child);
		}
		g_free (toolbarname);
		count ++;
	}
	//else
	//	g_warning ("Unknow UI widget: Can not add in container");
}

static void
on_remove_merge_widget (GtkWidget *merge, GtkWidget *widget,
					 GtkWidget *ui_container)
{
}

void
create_anjuta_gui (AnjutaApp * app)
{
	GtkWidget *dock1;
	GtkWidget *toolbar1;
	GtkWidget *toolbar2;
	GtkWidget *toolbar3;
	GtkWidget *toolbar4;
	GtkWidget *toolbar5;
	GtkWidget *anjuta_notebook;
	GtkWidget *appbar1;
	GtkWidget *ui;
	GtkTooltips *tooltips;

	gnome_window_icon_set_default_from_file(PACKAGE_PIXMAPS_DIR "/" ANJUTA_PIXMAP_ICON);
	tooltips = gtk_tooltips_new ();

	gtk_widget_set_uposition (GTK_WIDGET (app), 0, 0);
	gtk_widget_set_usize (GTK_WIDGET (app), 500, 116);
	gtk_window_set_default_size (GTK_WINDOW (app), 700, 400);
	gtk_window_set_policy (GTK_WINDOW (app), FALSE, TRUE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (app), "mainide", "Anjuta");
	
	/* Add file drag and drop support */
	dnd_drop_init(GTK_WIDGET (app), on_anjuta_dnd_drop, NULL,
		"text/plain", "text/html", "text/source", NULL);

	dock1 = GNOME_APP (app)->dock;
	gtk_widget_show (dock1);

	/* Setup User Interface */
	ui = anjuta_ui_new (GTK_WIDGET (app),
						G_CALLBACK (on_add_merge_widget),
						G_CALLBACK (on_remove_merge_widget));
	app->ui = ANJUTA_UI (ui);
	
	create_anjuta_ui (app->ui, GTK_WIDGET (app));

	gtk_window_set_transient_for (GTK_WINDOW (app->ui),
								  GTK_WINDOW (app));

	/* Toolbars are created and added */
	toolbar1 =
		create_main_toolbar (GTK_WIDGET (app), &(app->toolbar.main_toolbar));
	
	toolbar2 =
		create_browser_toolbar (GTK_WIDGET (app), &(app->toolbar.browser_toolbar));
	toolbar3 =
		create_debug_toolbar (GTK_WIDGET (app), &(app->toolbar.debug_toolbar));
	toolbar4 =
		create_format_toolbar (GTK_WIDGET (app), &(app->toolbar.format_toolbar));
	toolbar5 =
		create_extended_toolbar (GTK_WIDGET (app), &(app->toolbar.extended_toolbar));
	/* Set toolbar style */
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar3), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar4), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar5), GTK_TOOLBAR_ICONS);

	anjuta_notebook = gtk_notebook_new ();
	gtk_widget_ref (anjuta_notebook);
	gtk_widget_show (anjuta_notebook);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (anjuta_notebook), TRUE);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (anjuta_notebook), TRUE);
	
	anjuta_shell_add_widget (ANJUTA_SHELL (app), anjuta_notebook,
							 "AnjutaDocumentManager", "Documents", NULL);
	
	appbar1 = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_USER);
	gtk_widget_ref (appbar1);
	gtk_widget_show (appbar1);
	gnome_app_set_statusbar (GNOME_APP (app), appbar1);

	gtk_signal_connect (GTK_OBJECT (app), "focus_in_event",
			    GTK_SIGNAL_FUNC (on_anjuta_window_focus_in_event),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (app), "key_press_event",
			    GTK_SIGNAL_FUNC (on_anjuta_window_key_press_event),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (app), "key_release_event",
          GTK_SIGNAL_FUNC (on_anjuta_window_key_release_event),
          NULL);
	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			    GTK_SIGNAL_FUNC (on_anjuta_delete), NULL);
	gtk_signal_connect (GTK_OBJECT (app), "destroy",
			    GTK_SIGNAL_FUNC (on_anjuta_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_notebook), "switch_page",
			    GTK_SIGNAL_FUNC (on_anjuta_notebook_switch_page),
			    NULL);
	app->accel_group = GNOME_APP (app)->accel_group;
	app->notebook = anjuta_notebook;
	app->appbar = appbar1;

	// main_menu_install_hints (anjuta_gui);
}
