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

#include "anjuta.h"
#include "pixmaps.h"
#include "resources.h"
#include "main_menubar.h"
#include "dnd.h"
#include "anjuta-ui.h"
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
	anjuta_ui_add_action_group_entries (ui, "ActionGroupPrint", _("Print"),
										menu_entries_print,
										G_N_ELEMENTS (menu_entries_print));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEdit", _("Edit"),
										menu_entries_edit,
										G_N_ELEMENTS (menu_entries_edit));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupTransform", _("Transform"),
										menu_entries_transform,
										G_N_ELEMENTS (menu_entries_transform));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSelect", _("Select"),
										menu_entries_select,
										G_N_ELEMENTS (menu_entries_select));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupInsert", _("Insert"),
										menu_entries_insert,
										G_N_ELEMENTS (menu_entries_insert));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupComment", _("Comment"),
										menu_entries_comment,
										G_N_ELEMENTS (menu_entries_comment));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSearch", _("Search"),
										menu_entries_search,
										G_N_ELEMENTS (menu_entries_search));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupNavigate", _("Navigate"),
										menu_entries_navigation,
										G_N_ELEMENTS (menu_entries_navigation));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupView", _("View"),
										menu_entries_view,
										G_N_ELEMENTS (menu_entries_view));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupProject", _("Project"),
										menu_entries_project,
										G_N_ELEMENTS (menu_entries_project));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupFormat", _("Format"),
										menu_entries_format,
										G_N_ELEMENTS (menu_entries_format));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupStyles", _("Syntax hilighting styles"),
										menu_entries_style,
										G_N_ELEMENTS (menu_entries_style));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupBuild", _("Build"),
										menu_entries_build,
										G_N_ELEMENTS (menu_entries_build));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupBookmark", _("Bookmark"),
										menu_entries_bookmark,
										G_N_ELEMENTS (menu_entries_bookmark));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSettings", _("Settings"),
										menu_entries_settings,
										G_N_ELEMENTS (menu_entries_settings));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupHelp", _("Help"),
										menu_entries_help,
										G_N_ELEMENTS (menu_entries_help));
	
	group = egg_action_group_new ("ActionGroupNavigation");
	
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditGotoLineEntry",
						   "label", _("Goto line"),
						   "tooltip", _("Enter the line number to jump and press enter"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 50,
							NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_goto_clicked), ui);
	egg_action_group_add_action (group, action);
	
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditSearchEntry",
						   "label", _("Search"),
						   "tooltip", _("Incremental search"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 150,
							NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_find_clicked), ui);
	g_signal_connect (action, "changed",
					  G_CALLBACK (on_toolbar_find_incremental), ui);
	g_signal_connect (action, "focus-in",
					  G_CALLBACK (on_toolbar_find_incremental_start), ui);
	g_signal_connect (action, "focus_out",
					  G_CALLBACK (on_toolbar_find_incremental_end), ui);
	egg_action_group_add_action (group, action);
	
	anjuta_ui_add_action_group (ui, "ActionGroupNavigation",
								N_("Navigation"), group);
	
	merge_id = anjuta_ui_merge (ui, UI_FILE);
	gtk_window_add_accel_group (GTK_WINDOW (app),
								anjuta_ui_get_accel_group (ui));
}

static
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
		static count = 0;
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
			item = gnome_app_get_dock_item_by_name (GNOME_APP (app->widgets.window),
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
create_anjuta_gui (AnjutaApp * appl)
{
	GtkWidget *accel_dialog;
	GtkWidget *anjuta_gui;
	GtkWidget *dock1;
	GtkWidget *vbox0;
	GtkWidget *toolbar1;
	GtkWidget *toolbar2;
	GtkWidget *toolbar3;
	GtkWidget *toolbar4;
	GtkWidget *toolbar5;
	GtkWidget *vpaned1;
	GtkWidget *hpaned1;
	GtkWidget *vbox1;
	GtkWidget *hbox3;
	GtkWidget *button1;
	GtkWidget *frame1;
	GtkWidget *vbox3;
	GtkWidget *pixmap;
	GtkWidget *hseparator1;
	GtkWidget *hseparator2;
	GtkWidget *hseparator3;
	GtkWidget *button3;
	GtkWidget *hbox2;
	GtkWidget *vbox2;
	GtkWidget *button4;
	GtkWidget *frame2;
	GtkWidget *hbox4;
	GtkWidget *vseparator3;
	GtkWidget *vseparator2;
	GtkWidget *vseparator1;
	GtkWidget *button2;
	GtkWidget *anjuta_notebook;
	GtkWidget *appbar1;
	GtkTooltips *tooltips;

	gnome_window_icon_set_default_from_file(PACKAGE_PIXMAPS_DIR "/" ANJUTA_PIXMAP_ICON);
	tooltips = gtk_tooltips_new ();
	anjuta_gui = gnome_app_new ("Anjuta", "Anjuta");
	gtk_widget_set_uposition (anjuta_gui, 0, 0);
	gtk_widget_set_usize (anjuta_gui, 500, 116);
	gtk_window_set_default_size (GTK_WINDOW (anjuta_gui), 700, 400);
	gtk_window_set_policy (GTK_WINDOW (anjuta_gui), FALSE, TRUE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (anjuta_gui), "mainide", "Anjuta");
	
	/* Add file drag and drop support */
	dnd_drop_init(anjuta_gui, on_anjuta_dnd_drop, NULL,
		"text/plain", "text/html", "text/source", NULL);

	dock1 = GNOME_APP (anjuta_gui)->dock;
	gtk_widget_show (dock1);

	/* Setup User Interface */
	app->ui = ANJUTA_UI (anjuta_ui_new (anjuta_gui,
										G_CALLBACK (on_add_merge_widget),
										G_CALLBACK (on_remove_merge_widget)));
	// gtk_widget_show (GTK_WIDGET (app->ui));
	create_anjuta_ui (app->ui, anjuta_gui);

	gtk_window_set_transient_for (GTK_WINDOW (app->ui),
								  GTK_WINDOW (anjuta_gui));

	/*
	accel_dialog = egg_accel_dialog_new (anjuta_ui_get_menu_merge (app->ui));
	gtk_widget_show (accel_dialog);
	gtk_window_set_transient_for (GTK_WINDOW (accel_dialog),
								  GTK_WINDOW (anjuta_gui));
	*/
	/* Toolbars are created and added */

	toolbar1 =
		create_main_toolbar (anjuta_gui,
				     &(appl->widgets.toolbar.main_toolbar));
	// gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar1),
	// 		       ANJUTA_MAIN_TOOLBAR,
	//		       BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL,
	//		       BONOBO_DOCK_TOP, 1, 0, 0);
	//gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar1), 5);
	//gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar1),
	//			     GTK_TOOLBAR_SPACE_LINE);
	//gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar1),
	//			       GTK_RELIEF_NONE);
	
	toolbar2 =
		create_browser_toolbar (anjuta_gui,
					&(appl->widgets.toolbar.
					  browser_toolbar));
	// gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar2),
	//		       ANJUTA_BROWSER_TOOLBAR,
	//		       BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL,
	//		       BONOBO_DOCK_TOP, 2, 0, 0);
	//gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar2), 5);
	//gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2),
	//			     GTK_TOOLBAR_SPACE_LINE);
	//gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2),
	//			       GTK_RELIEF_NONE);

	toolbar3 =
		create_debug_toolbar (anjuta_gui,
					 &(appl->widgets.toolbar.
					   debug_toolbar));
	// gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar3),
	//		       ANJUTA_DEBUG_TOOLBAR,
	//		       BONOBO_DOCK_ITEM_BEH_NORMAL,
	//		       BONOBO_DOCK_TOP, 3, 0, 0);

	toolbar4 =
		create_format_toolbar (anjuta_gui,
				      &(appl->widgets.toolbar.format_toolbar));
	// gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar4),
	//		       ANJUTA_FORMAT_TOOLBAR,
	//		       BONOBO_DOCK_ITEM_BEH_NORMAL,
	//		       BONOBO_DOCK_TOP, 3, 1, 0);

	toolbar5 =
		create_extended_toolbar (anjuta_gui,
				       &(appl->widgets.toolbar.
					 extended_toolbar));
	// gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar5),
	//		       ANJUTA_EXTENDED_TOOLBAR,
	//		       BONOBO_DOCK_ITEM_BEH_NORMAL,
	//		       BONOBO_DOCK_TOP, 4, 0, 0);
	/* Set toolbar style */
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar3), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar4), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar5), GTK_TOOLBAR_ICONS);
	//gtk_toolbar_set_icon_size (GTK_TOOLBAR(toolbar1), 22);
	//gtk_toolbar_set_icon_size (GTK_TOOLBAR(toolbar2), 22);
	//gtk_toolbar_set_icon_size (GTK_TOOLBAR(toolbar3), 22);
	//gtk_toolbar_set_icon_size (GTK_TOOLBAR(toolbar4), 22);
	//gtk_toolbar_set_icon_size (GTK_TOOLBAR(toolbar5), 22);
	
	//	 Use a vbox, not an eventbox, to support gtk_widget_reparent() stuff
	//	done when hiding/showing project and message panes
	
	vbox0 = gtk_vbox_new(FALSE,0);
	gtk_widget_ref (vbox0);
	gtk_widget_show (vbox0);
	gnome_app_set_contents (GNOME_APP (anjuta_gui), vbox0);

	vpaned1 = gtk_vpaned_new ();
	gtk_widget_ref (vpaned1);
	gtk_widget_show (vpaned1);
	gtk_container_add (GTK_CONTAINER (vbox0), vpaned1);
	//gtk_paned_set_gutter_size (GTK_PANED (vpaned1), 8);
	//gtk_paned_set_handle_size (GTK_PANED (vpaned1), 8);

	hpaned1 = gtk_hpaned_new ();
	gtk_widget_ref (hpaned1);
	gtk_widget_show (hpaned1);
	gtk_container_add (GTK_CONTAINER (vpaned1), hpaned1);
	//gtk_paned_set_gutter_size (GTK_PANED (hpaned1), 8);
	//gtk_paned_set_handle_size (GTK_PANED (hpaned1), 8);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_ref (vbox1);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (hpaned1), vbox1);

	hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox3);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox3, FALSE, FALSE, 0);

	button1 = gtk_button_new ();
	gtk_widget_show (button1);
	gtk_box_pack_start (GTK_BOX (hbox3), button1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button1), 1);
	gtk_tooltips_set_tip (tooltips, button1,
			      _("Click here to undock this window"), NULL);

	pixmap =
		anjuta_res_get_image ("handle_undock.xpm");
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button1), pixmap);

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (hbox3), frame1, TRUE, TRUE, 1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);

	vbox3 = gtk_vbox_new (TRUE, 1);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (frame1), vbox3);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox3), hseparator1, TRUE, TRUE, 0);

	hseparator2 = gtk_hseparator_new ();
	gtk_widget_show (hseparator2);
	gtk_box_pack_start (GTK_BOX (vbox3), hseparator2, TRUE, TRUE, 0);

	hseparator3 = gtk_hseparator_new ();
	gtk_widget_show (hseparator3);
	gtk_box_pack_start (GTK_BOX (vbox3), hseparator3, TRUE, TRUE, 0);

	button3 = gtk_button_new ();
	gtk_widget_show (button3);
	gtk_box_pack_start (GTK_BOX (hbox3), button3, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button3), 1);
	gtk_tooltips_set_tip (tooltips, button3,
			      _("Click here to hide this window"), NULL);


	pixmap = anjuta_res_get_image ("handle_hide.xpm");
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button3), pixmap);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_ref (hbox2);
	gtk_widget_show (hbox2);
	gtk_container_add (GTK_CONTAINER (vpaned1), hbox2);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (hbox2), vbox2, FALSE, FALSE, 0);

	button4 = gtk_button_new ();
	gtk_widget_show (button4);

	gtk_box_pack_start (GTK_BOX (vbox2), button4, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button4), 1);
	gtk_tooltips_set_tip (tooltips, button4,
			      _("Click here to hide this window"), NULL);

	pixmap = anjuta_res_get_image ("handle_hide.xpm");
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button4), pixmap);

	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (vbox2), frame2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_NONE);

	hbox4 = gtk_hbox_new (TRUE, 1);
	gtk_widget_show (hbox4);
	gtk_container_add (GTK_CONTAINER (frame2), hbox4);

	vseparator3 = gtk_vseparator_new ();
	gtk_widget_show (vseparator3);
	gtk_box_pack_start (GTK_BOX (hbox4), vseparator3, TRUE, TRUE, 0);

	vseparator2 = gtk_vseparator_new ();
	gtk_widget_show (vseparator2);
	gtk_box_pack_start (GTK_BOX (hbox4), vseparator2, TRUE, TRUE, 0);

	vseparator1 = gtk_vseparator_new ();
	gtk_widget_show (vseparator1);
	gtk_box_pack_start (GTK_BOX (hbox4), vseparator1, TRUE, TRUE, 0);

	button2 = gtk_button_new ();
	gtk_widget_show (button2);
	gtk_box_pack_start (GTK_BOX (vbox2), button2, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button2), 1);
	gtk_tooltips_set_tip (tooltips, button2,
			      _("Click here to undock this window"), NULL);

	pixmap =
		anjuta_res_get_image ("handle_undock.xpm");
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button2), pixmap);

	anjuta_notebook = gtk_notebook_new ();
	gtk_widget_ref (anjuta_notebook);
	gtk_widget_show (anjuta_notebook);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (anjuta_notebook), TRUE);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (anjuta_notebook), TRUE);

	/* Notebook is not added now. It will be added later */
	appbar1 = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_USER);
	gtk_widget_ref (appbar1);
	gtk_widget_show (appbar1);
	gnome_app_set_statusbar (GNOME_APP (anjuta_gui), appbar1);

	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "focus_in_event",
			    GTK_SIGNAL_FUNC (on_anjuta_window_focus_in_event),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "key_press_event",
			    GTK_SIGNAL_FUNC (on_anjuta_window_key_press_event),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "key_release_event",
          GTK_SIGNAL_FUNC (on_anjuta_window_key_release_event),
          NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "delete_event",
			    GTK_SIGNAL_FUNC (on_anjuta_delete), NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "destroy",
			    GTK_SIGNAL_FUNC (on_anjuta_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_notebook), "switch_page",
			    GTK_SIGNAL_FUNC (on_anjuta_notebook_switch_page),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_prj_list_undock_button_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_prj_list_hide_button_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_hide_button_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_mesg_win_undock_button_clicked), NULL);

	app->accel_group = GNOME_APP (anjuta_gui)->accel_group;
	app->widgets.window = anjuta_gui;
	app->widgets.client_area = vbox0;
	app->widgets.notebook = anjuta_notebook;
	app->widgets.appbar = appbar1;
	app->widgets.hpaned = hpaned1;
	app->widgets.vpaned = vpaned1;
	app->widgets.mesg_win_container = hbox2;
	app->widgets.project_dbase_win_container = vbox1;

	// main_menu_install_hints (anjuta_gui);
}
