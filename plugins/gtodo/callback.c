#include <gtk/gtk.h>
#include "main.h"

void remove_todo_item(GtkWidget *fake, gboolean internall){
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model = mw.sortmodel;
	gint value;

	/* testing blocking the file changed */
	/* it seems to work.. so I keep it here */
	gtodo_client_block_changed_callback(cl);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(mw.treeview));	
	if(!gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		if(!internall)message_box( _("You need to select a todo item before you can remove it"),"",GTK_MESSAGE_INFO);
		return; 
	}
	if(!internall) if(!message_box( _("Are you sure you want to remove the selected todo item?"), _("Remove"), GTK_MESSAGE_WARNING))
	{
		return;
	}

	gtk_tree_model_get(model,&iter, ID, &value,  -1);

	gtodo_client_delete_todo_by_id(cl,value);
	gtk_list_store_clear(mw.list);
	load_category();

	/* end test */
	gtodo_client_unblock_changed_callback(cl);
}

/* this is the function called if the user switches to another category.. */
/* can't hurt to change the name here. */

void category_changed(void)
{
	if(cl != NULL)
	{
		int i = gtk_option_menu_get_history(GTK_OPTION_MENU(mw.option));
		if(i != 0)if( mw.mitems == NULL || mw.mitems[i-2] == NULL) return;
		if(i == categorys+3)
		{
			int j =  gconf_client_get_int(client, "/apps/gtodo/view/last-category",NULL);
			category_manager();
			if(j < categorys+3 && mw.mitems != NULL && mw.mitems[j-2] != NULL) gtk_option_menu_set_history(GTK_OPTION_MENU(mw.option),j);
			gtk_list_store_clear(mw.list);
			load_category();
			return;
		}
		gtk_list_store_clear(mw.list);
		load_category();
		gconf_client_set_int(client, "/apps/gtodo/view/last-category",i,NULL);
	}
}


void list_toggled_done(GtkCellRendererToggle *cell, gchar *path_str)
{
	GtkTreeIter iter, childiter;
	GtkTreeModel *model = mw.sortmodel;
	gint done, id;
	GTodoItem *item = NULL;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	if(gtodo_client_get_read_only(cl))
	{
		gtk_tree_path_free(path);	      
		return;
	}
	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(mw.sortmodel), &childiter, &iter);
	gtk_tree_path_free(path);	      
	gtk_tree_model_get(model, &iter,ID, &id,DONE, &done, -1);
	gtk_list_store_set(mw.list, &childiter,DONE, !done, -1);
	item = gtodo_client_get_todo_item_from_id(cl, id);
	if(item == NULL) return;
	if(done== 0)gtodo_todo_item_set_done(item, TRUE);
	if(done == 1) gtodo_todo_item_set_done(item,FALSE);
	/* block the callback and reset the changed dbase, because I don't want to refresh tree view */
	gtodo_client_block_changed_callback(cl);
	gtodo_client_edit_todo_item(cl, item);
	gtodo_client_reset_changed_callback(cl);
	gtodo_client_unblock_changed_callback(cl);
}

void purge_category(void)
{
	gint done, value;
	GtkTreeIter iter;
	GtkTreeModel *model = mw.sortmodel;
	gchar *category;
	gchar *tm;

	if(gtk_option_menu_get_history(GTK_OPTION_MENU(mw.option))== 0)tm = g_strdup_printf(_("Are you sure you want to remove all the completed todo items?"));
	else tm = g_strdup_printf(_("Are you sure you want to remove all the completed todo items in the category \"%s\"?"), mw.mitems[gtk_option_menu_get_history(GTK_OPTION_MENU(mw.option))-2]->date);

	if(!message_box( tm, _("Remove"), GTK_MESSAGE_WARNING))
	{
		g_free(tm);
		return;
	}
	g_free(tm);
	gtodo_client_block_changed_callback(cl);	
	if(gtk_tree_model_get_iter_first(model, &iter))
		do
		{
			gtk_tree_model_get(model,&iter,DONE, &done,  ID, &value,CATEGORY, &category, -1);
			if(done)
			{
				gtodo_client_delete_todo_by_id(cl, value);
			}
			g_free(category);
		}while(gtk_tree_model_iter_next(model, &iter));

	gtodo_client_unblock_changed_callback(cl);	
	gtk_list_store_clear(mw.list);
	load_category();
}


int message_box(gchar *text, gchar *buttext, GtkMessageType type)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(GTK_WINDOW(mw.window), 
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			type,
			GTK_BUTTONS_NONE,
			"%s", text
			);
	if(type == GTK_MESSAGE_WARNING)
	{
		gtk_dialog_add_buttons(GTK_DIALOG(dialog),GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, buttext, GTK_RESPONSE_OK, NULL);
	}
	else gtk_dialog_add_buttons(GTK_DIALOG(dialog),GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_widget_show_all(dialog);
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_OK:
		case GTK_RESPONSE_YES:
			gtk_widget_destroy(dialog);
			return 1;
		default:
			gtk_widget_destroy(dialog);
			return 0;	
	}
	return 0;
}


/* the custom sorthing function..  I think I have all sort possiblilties here.. */

gint sort_function_test(GtkTreeModel *model,GtkTreeIter *a,GtkTreeIter *b,gpointer user_data)
{
	gint prioritya=0, priorityb=0;
	gint donea=0, doneb=0;
	guint64 datea=0, dateb=0;
	if(a == NULL || b == NULL) return 0;

	gtk_tree_model_get(model,a, DONE, &donea,PRIORITY,&prioritya,END_DATE, &datea, -1);
	gtk_tree_model_get(model,b, DONE, &doneb,PRIORITY,&priorityb,END_DATE, &dateb, -1);

	if(settings.sorttype == 0)
	{
		/* done, date, priority */

		if((donea - doneb)!= 0) return doneb-donea;
		if(dateb == GTODO_NO_DUE_DATE && datea != GTODO_NO_DUE_DATE) return 1;
		if(datea == GTODO_NO_DUE_DATE && dateb != GTODO_NO_DUE_DATE) return -1;
		if((datea - dateb) != 0)return dateb - datea;
		if((prioritya - priorityb) != 0) return prioritya -priorityb;
	}
	else if(settings.sorttype == 1)
	{
		/* done, priority, date */
		if((donea - doneb)!= 0) return doneb-donea;
		if((prioritya - priorityb) != 0) return prioritya -priorityb;
		if((datea - dateb) != 0)return dateb - datea;
	}
	else if(settings.sorttype == 2)
	{
		/* priority,date, done */
		if((prioritya - priorityb) != 0) return prioritya -priorityb;
		if((datea - dateb) != 0)return dateb - datea;
		if((donea - doneb)!= 0) return doneb-donea;
	}
	else if(settings.sorttype == 3)
	{
		/* priority,done, date */
		if((prioritya - priorityb) != 0) return prioritya -priorityb;
		if((donea - doneb)!= 0) return doneb-donea;
		if((datea - dateb) != 0)return dateb - datea;
	}
	else if(settings.sorttype == 4)
	{
		/* date, priority, done */
		if((datea - dateb) != 0)return dateb - datea;
		if((prioritya - priorityb) != 0) return prioritya -priorityb;
		if((donea - doneb)!= 0) return doneb-donea;
	}
	else if(settings.sorttype == 5)
	{
		/* date, done, priority */
		if((datea - dateb) != 0)return dateb - datea;
		if((donea - doneb)!= 0) return doneb-donea;
		if((prioritya - priorityb) != 0) return prioritya -priorityb;
	}		
	return 0;
}
