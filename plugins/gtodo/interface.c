#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-preferences.h>
#include "debug_printf.h"

/* the main window struct */
mwindow mw;

/* settings struct */
sets settings;
AnjutaPreferences *preferences;

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
}

void gtodo_load_settings ()
{
	debug_printf(DEBUG_INFO, "Creating anjuta preferences client");
	preferences = anjuta_preferences_default ();
	
	debug_printf(DEBUG_INFO, "Loading settings from anjuta preferences");

	settings.ask_delete_category 	=  anjuta_preferences_get_bool(preferences, "gtodo-ask-delete-category");
	/* this is kinda (thanks gtk :-/) buggy.. gtk need fix first */
//	settings.list_tooltip		= anjuta_preferences_get_bool(preferences, "gtodo-show-tooltip");
	/* auto purge is default on.. */
	settings.auto_purge		= anjuta_preferences_get_bool(preferences, "gtodo-auto-purge");
	/* set default auto purge to a week */	
	settings.purge_days		= anjuta_preferences_get_int(preferences, "gtodo-auto-purge-days");
	settings.due_color 		= anjuta_preferences_get(preferences, "gtodo-due-color");
	settings.due_today_color 	= anjuta_preferences_get(preferences, "gtodo-due-today-color");
	settings.due_in_days_color 	= anjuta_preferences_get(preferences, "gtodo-due-in-days-color");
	settings.due_days 		= anjuta_preferences_get_int(preferences, "gtodo-due-days");
	settings.sorttype 		= anjuta_preferences_get_int(preferences, "gtodo-sort-type");
	settings.sortorder 		= anjuta_preferences_get_int(preferences, "gtodo-sort-order");
	/* treeview hide preferences.. */
	settings.hide_done 		= anjuta_preferences_get_bool(preferences, "gtodo-hide-done");
	settings.hide_due 		= anjuta_preferences_get_bool(preferences, "gtodo-hide-due");
	settings.hide_nodate 		=  anjuta_preferences_get_bool(preferences, "gtodo-hide-nodate");
	settings.hl_indays		= anjuta_preferences_get_bool(preferences,"gtodo-hl-indays");
	settings.hl_due			= anjuta_preferences_get_bool(preferences,"gtodo-hl-due");
	settings.hl_today		= anjuta_preferences_get_bool(preferences,"gtodo-hl-today");
	pref_set_notifications(preferences);
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
		int i =  anjuta_preferences_get_int(preferences, "gtodo-last-category");
		debug_printf(DEBUG_INFO, "Reading categories");
		read_categorys();
		gtk_combo_box_set_active(GTK_COMBO_BOX(mw.option),i);
	}
	/* nasty thing to fix the tooltip in the list */
	if(anjuta_preferences_get_bool(preferences, "gtodo-show-tooltip"))
	{
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), FALSE);			
	}
	
	if(gtodo_client_get_read_only(cl)) 
	{
		debug_printf(DEBUG_WARNING, "Read only file detected, disabling severall options");
		set_read_only();
	}
	
	gtodo_client_set_changed_callback(cl,(void *)backend_changed ,NULL);
	g_timeout_add_seconds (300, (GSourceFunc)check_for_notification_event, NULL);

	check_for_notification_event();
}

static void on_export_clicked_cb (GtkWidget *foo, gpointer user_data)
{
	export_gui();
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
	if (!cl)
		return NULL;
	
	stock_icons();
	/* add an verticall box */
	mw.vbox = gtk_vbox_new(FALSE, 0);

	mw.toolbar = gtk_hbox_new(FALSE, 6);    
	gtk_box_pack_end(GTK_BOX(mw.vbox), mw.toolbar, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mw.toolbar), 6);

	/* add drop down menu's */
	mw.option = gtk_combo_box_new_text();
	// gtk_widget_set_size_request(mw.option, 120 , -1);
	gtk_box_pack_start(GTK_BOX(mw.toolbar), mw.option, FALSE, FALSE, 0); 

	gtk_combo_box_append_text (GTK_COMBO_BOX (mw.option), _("All"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (mw.option), "");
	mw.mitems = g_malloc(1*sizeof(GtkWidget *));
	mw.mitems[0] = NULL;
	/* shand can be removed?? */
	gtk_combo_box_set_active (GTK_COMBO_BOX (mw.option), 0);
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

	mw.tbexport = anjuta_util_button_new_with_stock_image (_("_Export"), GTK_STOCK_SAVE_AS);
	gtk_size_group_add_widget(sgroup, mw.tbaddbut);
	gtk_box_pack_start (GTK_BOX (mw.toolbar), mw.tbexport, FALSE, FALSE, 0);

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
	g_signal_connect(G_OBJECT(mw.tbexport), "clicked", G_CALLBACK(on_export_clicked_cb), NULL);


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
	if(!anjuta_preferences_get_bool(preferences, "gtodo-show-priority-column")) gtk_tree_view_column_set_visible(column, FALSE);

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Due date"), renderer, "text" , F_DATE,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR,NULL);
	gtk_tree_view_column_set_sort_column_id (column, F_DATE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);    
	if(!anjuta_preferences_get_bool(preferences, "gtodo-show-due-column")) gtk_tree_view_column_set_visible(column, FALSE);

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text" , CATEGORY,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR,NULL);
	gtk_tree_view_column_set_sort_column_id (column, CATEGORY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);   	
	if(!anjuta_preferences_get_bool(preferences, "gtodo-show-category-column")) gtk_tree_view_column_set_visible(column, FALSE);

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
	icon_source = gtk_icon_source_new();
	icons = gtk_icon_set_new();
	gtk_icon_source_set_filename(icon_source, PIXMAP_PATH"/gtodo-edit.png");
	gtk_icon_set_add_source(icons, icon_source);
	gtk_icon_factory_add(factory, "gtodo-edit", icons);

	icons = gtk_icon_set_new();
	gtk_icon_source_set_filename(icon_source, PIXMAP_PATH"/"ICON_FILE);
	gtk_icon_set_add_source(icons, icon_source);
	gtk_icon_factory_add(factory, "gtodo", icons);

	/* added this because there isn no about availible in the default stock set. */
	/* I call it gnome about so it still uses the one out off the current icon theme */
	icons = gtk_icon_set_new();
	gtk_icon_source_set_filename(icon_source, PIXMAP_PATH"/gtodo-about.png");
	gtk_icon_set_add_source(icons, icon_source);
	gtk_icon_factory_add(factory, "gnome-stock-about", icons);
	gtk_icon_factory_add_default (factory);

	gtk_icon_source_free (icon_source);

}

void gtodo_set_hide_done(gboolean hide_it)
{
	/*    settings.hide_done = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM (gtk_item_factory_get_widget(mw.item_factory, N_("/View/Todo List/Hide Completed Items"))));*/
	settings.hide_done = hide_it;
	anjuta_preferences_set_bool(preferences, "gtodo-hide-done", settings.hide_done);
}

void gtodo_set_hide_nodate(gboolean hide_it)
{
	settings.hide_nodate = hide_it;
	anjuta_preferences_set_bool(preferences, "gtodo-hide-nodate", settings.hide_nodate);
}

void gtodo_set_hide_due(gboolean hide_it)
{
	settings.hide_due = hide_it;
	anjuta_preferences_set_bool(preferences, "gtodo-hide-due", settings.hide_due);
}

gboolean gtodo_get_hide_done()
{
	return settings.hide_done;
}

gboolean gtodo_get_hide_nodate()
{
	return settings.hide_nodate;
}

gboolean gtodo_get_hide_due()
{
	return settings.hide_due;
}

void gtodo_set_sorting_order (gboolean ascending_sort)
{
	settings.sortorder = ascending_sort;
	anjuta_preferences_set_int(preferences, "gtodo-sort-order",settings.sortorder);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (mw.sortmodel),0, settings.sortorder);
}

void gtodo_set_sorting_type (int sorting_type)
{
	settings.sorttype = sorting_type;
	anjuta_preferences_set_int(preferences, "gtodo-sort-type",sorting_type);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (gtk_tree_view_get_model(GTK_TREE_VIEW(mw.treeview))),0, settings.sortorder);
	category_changed();
}
