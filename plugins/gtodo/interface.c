#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "tray-icon.h"
#include "debug_printf.h"

/* the main window struct */
mwindow mw;

/* settings struct */
sets settings;
GConfClient* client;    

/* the connection to the xml file */
GTodoClient *cl = NULL;

gulong shand;
GdkEventMotion *ovent;

static void stock_icons(void);

static void backend_changed()
{
	debug_printf(DEBUG_INFO, "Backend data changed, updating interface");
    	read_categorys();
	category_changed();    
}

static void set_read_only()
{
	gtk_widget_set_sensitive (mw.tbdelbut, FALSE);                                                                                 
	gtk_label_set_text_with_mnemonic(GTK_LABEL(mw.tbeditlb),_("_View"));
	gtk_widget_set_sensitive(mw.tbaddbut, FALSE);
	gtk_widget_set_sensitive (gtk_item_factory_get_widget(GTK_ITEM_FACTORY (mw.item_factory), N_("/Item/Edit")), FALSE);
	gtk_widget_set_sensitive (gtk_item_factory_get_widget(GTK_ITEM_FACTORY (mw.item_factory), N_("/Item/Remove")), FALSE);        
	gtk_widget_set_sensitive (gtk_item_factory_get_widget(GTK_ITEM_FACTORY (mw.item_factory), N_("/Item/Add")), FALSE);         
	gtk_widget_set_sensitive (gtk_item_factory_get_widget(GTK_ITEM_FACTORY (mw.item_factory), N_("/Item/Remove Completed Items")), FALSE);         
	gtk_widget_set_sensitive (gtk_item_factory_get_widget(GTK_ITEM_FACTORY (mw.item_factory), N_("/ToDo/Edit Categories")), FALSE);         
	// gtk_widget_show_all(mw.window);
}

void gtodo_load_settings ()
{
	debug_printf(DEBUG_INFO, "Creating gconf client");
	client = gconf_client_get_default();
	
	gconf_client_add_dir(client,
			"/apps/gtodo/prefs",
			GCONF_CLIENT_PRELOAD_NONE,
			NULL);
	gconf_client_add_dir(client,
			"/apps/gtodo/view",
			GCONF_CLIENT_PRELOAD_NONE,
			NULL);

	debug_printf(DEBUG_INFO, "Loading settings from gconf");

	/* Enable tray by default..  user won't know if he/she doesnt have tray */
	/* this is just for people who don't want it in there tray */
	settings.place = gconf_client_get_bool(client, "/apps/gtodo/prefs/restore-position",NULL);;
	settings.size =  gconf_client_get_bool(client, "/apps/gtodo/prefs/restore-size",NULL);;
	settings.ask_delete_category 	=  gconf_client_get_bool(client, "/apps/gtodo/prefs/ask-delete-category",NULL);
	/* this is kinda (thanks gtk :-/) buggy.. gtk need fix first */
//	settings.list_tooltip		= gconf_client_get_bool(client, "/apps/gtodo/prefs/show-tooltip",NULL);
	/* auto purge is default on.. */
	settings.auto_purge		= gconf_client_get_bool(client, "/apps/gtodo/prefs/auto-purge",NULL);
	/* set default auto purge to a week */	
	settings.purge_days		= gconf_client_get_int(client, "/apps/gtodo/prefs/auto-purge-days",NULL);
	settings.due_color 		= gconf_client_get_string(client, "/apps/gtodo/prefs/due-color",NULL);
	settings.due_today_color 	= gconf_client_get_string(client, "/apps/gtodo/prefs/due-today-color",NULL);
	settings.due_in_days_color 	= gconf_client_get_string(client, "/apps/gtodo/prefs/due-in-days-color",NULL);
	settings.due_days 		= gconf_client_get_int(client, "/apps/gtodo/prefs/due-days",NULL);
	settings.sorttype 		= gconf_client_get_int(client, "/apps/gtodo/prefs/sort-type",NULL);	
	settings.sortorder 		= gconf_client_get_int(client, "/apps/gtodo/prefs/sort-order",NULL);	
	/* treeview hide preferences.. */
	settings.hide_done 		= gconf_client_get_bool(client, "/apps/gtodo/prefs/hide-done",NULL);
	settings.hide_due 		= gconf_client_get_bool(client, "/apps/gtodo/prefs/hide-due",NULL);
	settings.hide_nodate 		=  gconf_client_get_bool(client, "/apps/gtodo/prefs/hide-nodate",NULL);
	settings.hl_indays		= gconf_client_get_bool(client,"/apps/gtodo/prefs/hl-indays",NULL);
	settings.hl_due			= gconf_client_get_bool(client,"/apps/gtodo/prefs/hl-due",NULL);
	settings.hl_today		= gconf_client_get_bool(client,"/apps/gtodo/prefs/hl-today",NULL);
	pref_gconf_set_notifications(client);
}

void gtodo_update_settings()
{
	if(settings.auto_purge && !gtodo_client_get_read_only(cl))
	{
		debug_printf(DEBUG_INFO, "Purging items that are past purge date");
		get_all_past_purge();
	}
	
	/* read the categorys */
	{
		int i =  gconf_client_get_int(client, "/apps/gtodo/view/last-category",NULL);
		debug_printf(DEBUG_INFO, "Reading categories");
		read_categorys();
		gtk_option_menu_set_history(GTK_OPTION_MENU(mw.option),i);
	}
	/* nasty thing to fix the tooltip in the list */
	if(gconf_client_get_bool(client, "/apps/gtodo/prefs/show-tooltip",NULL))
	{
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), FALSE);			
	}
	
	if(gtodo_client_get_read_only(cl)) 
	{
		debug_printf(DEBUG_WARNING, "Read only file detected, disabling severall options");
		set_read_only();
	}
	
	gtodo_client_set_changed_callback(cl,(void *)backend_changed ,NULL);
	g_timeout_add(300000, (GSourceFunc)check_for_notification_event, NULL);

	check_for_notification_event();
}


GtkWidget * gui_create_todo_widget()
{
	GtkTreeViewColumn  *column;
	GtkCellRenderer *renderer;
	GtkWidget *lb, *hbox;
	static GtkWidget  *sw;
	/* used for the buttons @ the bottom */
	GtkSizeGroup *sgroup;
	/* TODO: make this compile time  (path is now)*/
	
	if (!cl)
		cl = gtodo_client_new_default (NULL);
	stock_icons();
	/* add an verticall box */
	mw.vbox = gtk_vbox_new(FALSE, 0);

	mw.toolbar = gtk_hbox_new(FALSE, 6);    
	gtk_box_pack_end(GTK_BOX(mw.vbox), mw.toolbar, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mw.toolbar), 6);

	/* add drop down menu's */
	mw.option = gtk_option_menu_new();
	mw.menu = gtk_menu_new();
	// gtk_widget_set_usize(mw.option, 120 , -1);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(mw.option), mw.menu);
	gtk_box_pack_start(GTK_BOX(mw.toolbar), mw.option, FALSE, FALSE, 0); 

	gtk_menu_shell_insert (GTK_MENU_SHELL (mw.menu), gtk_menu_item_new_with_label (_("All")), 0);
	gtk_menu_shell_insert (GTK_MENU_SHELL (mw.menu), gtk_separator_menu_item_new(), 1);
	mw.mitems = g_malloc(1*sizeof(GtkWidget *));
	mw.mitems[0] = NULL;
	/* shand can be removed?? */
	shand = g_signal_connect(G_OBJECT(mw.option), "changed", G_CALLBACK(category_changed), NULL);

	/* the buttonbox buttons */
	/* set the button's same size */ 
	sgroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	/*buttons */
	mw.tbaddbut = gtk_button_new_from_stock(GTK_STOCK_ADD);
	mw.tbdelbut = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	mw.tbeditbut = gtk_button_new();
	hbox = gtk_hbox_new(FALSE, 6);
	lb = gtk_alignment_new(0.5,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(lb), hbox);
	gtk_container_add(GTK_CONTAINER(mw.tbeditbut), lb);


	lb = gtk_image_new_from_stock("gtodo-edit", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(hbox), lb, FALSE, TRUE, 0);
	mw.tbeditlb = gtk_label_new_with_mnemonic(_("_Edit"));
	gtk_box_pack_start(GTK_BOX(hbox), mw.tbeditlb, FALSE, TRUE,0);

	/* add them */
	gtk_box_pack_end(GTK_BOX(mw.toolbar), mw.tbaddbut, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(mw.toolbar), mw.tbdelbut, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(mw.toolbar), mw.tbeditbut, FALSE, FALSE, 0);
	gtk_size_group_add_widget(sgroup, mw.tbaddbut);
	gtk_size_group_add_widget(sgroup, mw.tbeditbut);
	gtk_size_group_add_widget(sgroup, mw.tbdelbut);

	/* signals */
	g_signal_connect(G_OBJECT(mw.tbaddbut), "clicked", G_CALLBACK(gui_add_todo_item), GINT_TO_POINTER(0));
	g_signal_connect(G_OBJECT(mw.tbeditbut), "clicked", G_CALLBACK(gui_add_todo_item), GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(mw.tbdelbut), "clicked", G_CALLBACK(remove_todo_item), FALSE);


	/* add the todo list */
	mw.list = gtk_list_store_new(N_COL, G_TYPE_UINT64, G_TYPE_INT,G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_BOOLEAN);
	mw.sortmodel = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(mw.list));
	mw.treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(mw.sortmodel));

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sw), mw.treeview);

	gtk_box_pack_end(GTK_BOX(mw.vbox), sw, TRUE, TRUE, 0);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mw.treeview), TRUE);

	/* add the columns to the list */
	renderer =  gtk_cell_renderer_toggle_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes("", renderer, "active",DONE,"activatable", EDITABLE, NULL);
	gtk_tree_view_column_set_sort_column_id (column, DONE);
	g_signal_connect(renderer, "toggled", G_CALLBACK(list_toggled_done), NULL);    
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);    

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Priority"), renderer, "text" , PRIOSTR,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR, NULL);
	gtk_tree_view_column_set_sort_column_id (column, PRIOSTR);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);    
	if(!gconf_client_get_bool(client, "/apps/gtodo/view/show-priority-column",NULL)) gtk_tree_view_column_set_visible(column, FALSE);

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Due date"), renderer, "text" , F_DATE,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR,NULL);
	gtk_tree_view_column_set_sort_column_id (column, F_DATE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);    
	if(!gconf_client_get_bool(client, "/apps/gtodo/prefs/show-due-column",NULL)) gtk_tree_view_column_set_visible(column, FALSE);

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text" , CATEGORY,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR,NULL);
	gtk_tree_view_column_set_sort_column_id (column, CATEGORY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);   	
	if(!gconf_client_get_bool(client, "/apps/gtodo/view/show-category-column",NULL)) gtk_tree_view_column_set_visible(column, FALSE);

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, "wrap-mode", PANGO_WRAP_WORD, "wrap-width", 500, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Summary"), renderer, "markup",SUMMARY,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR,NULL);
	gtk_tree_view_column_set_sort_column_id (column, SUMMARY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);    

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE (mw.sortmodel),0, (GtkTreeIterCompareFunc)sort_function_test, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (mw.sortmodel),0, settings.sortorder);	    

	// gtk_tree_view_column_set_fixed_width(column, 10);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (mw.treeview), column);

	/* handlers */
	g_signal_connect(G_OBJECT(mw.treeview), "motion-notify-event", G_CALLBACK(mw_motion_cb), NULL);
	g_signal_connect(G_OBJECT(mw.treeview), "leave-notify-event", G_CALLBACK(mw_leave_cb), NULL);

	g_signal_connect(G_OBJECT (mw.treeview), "row-activated", G_CALLBACK (gui_add_todo_item), GINT_TO_POINTER(1));
	// FIXME: set_sorting_menu_item();
	gtodo_update_settings();
	return mw.vbox;
}

static void stock_icons()
{
	GtkIconFactory *factory;
	GtkIconSet *icons;
	GtkIconSource *icon_source;

	/* testing icon factory.. hope it works a little */
	factory = gtk_icon_factory_new();
	/* random icon */
	icons = gtk_icon_set_new();
	icon_source = gtk_icon_source_new();
	gtk_icon_source_set_filename(icon_source, PIXMAP_PATH"/gtodo-edit.png");
	gtk_icon_set_add_source(icons, icon_source);
	gtk_icon_factory_add(factory, "gtodo-edit", icons);

	icons = gtk_icon_set_new();
	icon_source = gtk_icon_source_new();
	gtk_icon_source_set_filename(icon_source, PIXMAP_PATH"/gtodo.png");
	gtk_icon_set_add_source(icons, icon_source);
	gtk_icon_factory_add(factory, "gtodo", icons);

	icons = gtk_icon_set_new();
	icon_source = gtk_icon_source_new();
	gtk_icon_source_set_filename(icon_source, PIXMAP_PATH"/gtodo_tray.png");
	gtk_icon_set_add_source(icons, icon_source);
	gtk_icon_factory_add(factory, "gtodo-tray", icons);

	/* added this because there isn no about availible in the default stock set. */
	/* I call it gnome about so it still uses the one out off the current icon theme */
	icons = gtk_icon_set_new();
	icon_source = gtk_icon_source_new();
	gtk_icon_source_set_filename(icon_source, PIXMAP_PATH"/gtodo-about.png");
	gtk_icon_set_add_source(icons, icon_source);
	gtk_icon_factory_add(factory, "gnome-stock-about", icons);
	gtk_icon_factory_add_default (factory);

}

void gtodo_set_hide_done(gboolean hide_it)
{
	/*    settings.hide_done = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM (gtk_item_factory_get_widget(mw.item_factory, N_("/View/Todo List/Hide Completed Items"))));*/
	settings.hide_done = hide_it;
	gconf_client_set_bool(client, "/apps/gtodo/prefs/hide-done", settings.hide_done,NULL);
}

void gtodo_set_hide_nodate(gboolean hide_it)
{
	settings.hide_nodate = hide_it;
	gconf_client_set_bool(client, "/apps/gtodo/prefs/hide-nodate", settings.hide_nodate, NULL);
}

void gtodo_set_hide_due(gboolean hide_it)
{
	settings.hide_due = hide_it;
	gconf_client_set_bool(client, "/apps/gtodo/prefs/hide-due", settings.hide_due, NULL);
}

void gtodo_set_sorting_order (gboolean ascending_sort)
{
	settings.sortorder = ascending_sort;
	gconf_client_set_int(client, "/apps/gtodo/prefs/sort-order",settings.sortorder,NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (mw.sortmodel),0, settings.sortorder);
}

void gtodo_set_sorting_type (int sorting_type)
{
	settings.sorttype = sorting_type;
	gconf_client_set_int(client, "/apps/gtodo/prefs/sort-type",sorting_type,NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (gtk_tree_view_get_model(GTK_TREE_VIEW(mw.treeview))),0, settings.sortorder);
	category_changed();
}
