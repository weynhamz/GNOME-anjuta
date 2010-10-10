#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-preferences.h>
#include "debug_printf.h"

#define GSETTINGS_SCHEMA "org.gnome.anjuta.gtodo"
#define PREF_ASK_DELETE_CATEGORY "gtodo-ask-delete-category"
#define PREF_SHOW_TOOLTIP "gtodo-show-tooltip"
#define PREF_AUTO_PURGE "gtodo-auto-purge"
#define PREF_AUTO_PURGE_DAYS "gtodo-auto-purge-days"
#define PREF_DUE_COLOR "gtodo-due-color"
#define PREF_DUE_TODAY_COLOR "gtodo-due-today-color"
#define PREF_DUE_IN_DAYS_COLOR "gtodo-due-in-days-color"
#define PREF_DUE_DAYS "gtodo-due-days"
#define PREF_SORT_TYPE "gtodo-sort-type"
#define PREF_SORT_ORDER "gtodo-sort-order"
#define PREF_HIDE_DONE "gtodo-hide-done"
#define PREF_HIDE_DUE "gtodo-hide-due"
#define PREF_HIDE_NODATE "gtodo-hide-nodate"
#define PREF_HL_INDAYS "gtodo-hl-indays"
#define PREF_HL_DUE "gtodo-hl-due"
#define PREF_HL_TODAY "gtodo-hl-today"

/* the main window struct */
mwindow mw;

/* settings struct */
sets settings;

gulong shand;
GdkEventMotion *ovent;

static void stock_icons(void);

void gtodo_load_settings ()
{
	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	settings.ask_delete_category 	=  g_settings_get_boolean (gsettings, PREF_ASK_DELETE_CATEGORY);
	/* this is kinda (thanks gtk :-/) buggy.. gtk need fix first */
//	settings.list_tooltip		= g_settings_get_boolean (settings, PREF_SHOW_TOOLTIP);
	/* auto purge is default on.. */
	settings.auto_purge		= g_settings_get_boolean (gsettings, PREF_AUTO_PURGE);
	/* set default auto purge to a week */	
	settings.purge_days		= g_settings_get_int (gsettings, PREF_AUTO_PURGE_DAYS);
	settings.due_color 		= g_settings_get_string (gsettings, PREF_DUE_COLOR);
	settings.due_today_color 	= g_settings_get_string (gsettings, PREF_DUE_TODAY_COLOR);
	settings.due_in_days_color 	= g_settings_get_string (gsettings, PREF_DUE_IN_DAYS_COLOR);
	settings.due_days 		= g_settings_get_int (gsettings, PREF_DUE_DAYS);
	settings.sorttype 		= g_settings_get_int (gsettings, PREF_SORT_TYPE);
	settings.sortorder 		= g_settings_get_int (gsettings, PREF_SORT_ORDER);
	/* treeview hide preferences.. */
	settings.hide_done 		= g_settings_get_boolean (gsettings, PREF_HIDE_DONE);
	settings.hide_due 		= g_settings_get_boolean (gsettings, PREF_HIDE_DUE);
	settings.hide_nodate 		=  g_settings_get_boolean (gsettings, PREF_HIDE_NODATE);
	settings.hl_indays		= g_settings_get_boolean (gsettings, PREF_HL_INDAYS);
	settings.hl_due			= g_settings_get_boolean (gsettings, PREF_HL_DUE);
	settings.hl_today		= g_settings_get_boolean (gsettings, PREF_HL_TODAY);
	pref_set_notifications(gsettings);
}

void gtodo_update_settings()
{
	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	if(settings.auto_purge)
	{
		get_all_past_purge();
	}
	
	/* read the categorys */
	{
		int i =  g_settings_get_int (gsettings, "gtodo-last-category");
		debug_printf(DEBUG_INFO, "Reading categories");
		read_categorys();
		gtk_combo_box_set_active(GTK_COMBO_BOX(mw.option),i);
	}
	/* nasty thing to fix the tooltip in the list */
	if(g_settings_get_boolean (gsettings, "gtodo-show-tooltip"))
	{
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), FALSE);			
	}
	g_object_unref (gsettings);
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

	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	
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
	if(!g_settings_get_boolean (gsettings, "gtodo-show-priority-column")) gtk_tree_view_column_set_visible(column, FALSE);

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Due date"), renderer, "text" , F_DATE,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR,NULL);
	gtk_tree_view_column_set_sort_column_id (column, F_DATE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);    
	if(!g_settings_get_boolean (gsettings, "gtodo-show-due-column")) gtk_tree_view_column_set_visible(column, FALSE);

	renderer =  gtk_cell_renderer_text_new();
	g_object_set (renderer, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text" , CATEGORY,"strikethrough", DONE, "foreground-set", DUE,"foreground", COLOR,NULL);
	gtk_tree_view_column_set_sort_column_id (column, CATEGORY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mw.treeview), column);   	
	if(!g_settings_get_boolean (gsettings, "gtodo-show-category-column")) gtk_tree_view_column_set_visible(column, FALSE);

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
	g_object_unref (gsettings);
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
	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	settings.hide_done = hide_it;
	g_settings_set_boolean (gsettings, PREF_HIDE_DONE, settings.hide_done);
	g_object_unref (gsettings);
}

void gtodo_set_hide_nodate(gboolean hide_it)
{
	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	settings.hide_nodate = hide_it;
	g_settings_set_boolean (gsettings, PREF_HIDE_NODATE, settings.hide_done);
	g_object_unref (gsettings);
}

void gtodo_set_hide_due(gboolean hide_it)
{
	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	settings.hide_due = hide_it;
	g_settings_set_boolean (gsettings, PREF_HIDE_NODATE, settings.hide_due);
	g_object_unref (gsettings);
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
	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	settings.sortorder = ascending_sort;
	g_settings_set_int (gsettings, PREF_SORT_ORDER, settings.sortorder);
	g_object_unref (gsettings);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (mw.sortmodel),0, settings.sortorder);
}

void gtodo_set_sorting_type (int sorting_type)
{
	GSettings* gsettings = g_settings_new (GSETTINGS_SCHEMA);
	settings.sorttype = sorting_type;
	g_settings_set_int (gsettings, PREF_SORT_TYPE, settings.sorttype);
	g_object_unref (gsettings);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (gtk_tree_view_get_model(GTK_TREE_VIEW(mw.treeview))),0, settings.sortorder);
	category_changed();
}
