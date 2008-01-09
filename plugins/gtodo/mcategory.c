#include <gtk/gtk.h>
#include "main.h"
void category_manager_delete_item(GtkWidget *button, GtkWidget *treeview);
void category_manager_add_item(GtkWidget *button, GtkWidget *treeview);
void category_manager_move_item_up(GtkWidget *button, GtkWidget *treeview);
void category_manager_move_item_down(GtkWidget *button, GtkWidget *treeview);
void tree_edited_string(GtkCellRendererText *cell, const char *path_string, const char *new_text, GtkWidget *treeview);

void category_manager(void)
{
	GtkWidget *dialog;
	GtkWidget *hbox, *vbutbox, *treeview, *sw;
	GtkListStore *list;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GtkWidget *newb, *deleteb, *upb, *downb;
	int i;
	gtodo_client_block_changed_callback(cl);
	dialog = gtk_dialog_new_with_buttons(_("Edit Categories"), GTK_WINDOW(mw.window), 
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
			NULL);

	hbox = gtk_hbox_new(FALSE,12);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	/* the list */
	list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);    

	renderer =  gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text" , 0, "editable", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);    

	g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(tree_edited_string), treeview);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(sw), treeview);

	gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);

	/* the buttonbox */
	vbutbox = gtk_vbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(vbutbox), GTK_BUTTONBOX_START);    
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(vbutbox), 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbutbox, FALSE, TRUE, 0);


	newb = gtk_button_new_from_stock(GTK_STOCK_NEW);

	deleteb = gtk_button_new_from_stock(GTK_STOCK_DELETE);

	upb =  gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	downb =  gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	g_signal_connect(G_OBJECT(deleteb), "clicked", G_CALLBACK(category_manager_delete_item), treeview);
	g_signal_connect(G_OBJECT(newb), "clicked", G_CALLBACK(category_manager_add_item), treeview);
	g_signal_connect(G_OBJECT(upb), "clicked", G_CALLBACK(category_manager_move_item_up), treeview);
	g_signal_connect(G_OBJECT(downb), "clicked", G_CALLBACK(category_manager_move_item_down), treeview);
	gtk_box_pack_start(GTK_BOX(vbutbox), newb, FALSE, FALSE,6);
	gtk_box_pack_start(GTK_BOX(vbutbox), deleteb, FALSE, FALSE,6);
	gtk_box_pack_start(GTK_BOX(vbutbox), gtk_hseparator_new(), FALSE, FALSE,6);    
	gtk_box_pack_start(GTK_BOX(vbutbox), upb, FALSE, FALSE,6);    
	gtk_box_pack_start(GTK_BOX(vbutbox), downb, FALSE, FALSE,6);    
	/* add the categorys */
	for(i=0; i< categorys; i++)
	{
		gtk_list_store_append(list, &iter);
		gtk_list_store_set(list, &iter, 0, mw.mitems[i]->date, 1, 1, -1);
	}   

	gtk_widget_set_usize(GTK_WIDGET(dialog), 350, 250);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	read_categorys();
	gtodo_client_unblock_changed_callback(cl);
}


void category_manager_move_item_up(GtkWidget *button, GtkWidget *treeview)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	GtkTreePath *patha;
	GtkTreeIter iter, iterb;
	gchar *category;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));	
	if(!gtk_tree_selection_get_selected(selection, &model, &iter))return;
	gtk_tree_model_get(model, &iter, 0, &category, -1);
	patha = gtk_tree_model_get_path(model, &iter);
	if(!gtk_tree_path_prev(patha)) return;
	if(!gtk_tree_model_get_iter(model, &iterb, patha))
	{
		gtk_tree_path_free(patha);
		return;
	}
	gtk_tree_path_free(patha);    

	gtodo_client_category_move_up(cl, category);    
	gtk_list_store_swap(GTK_LIST_STORE(model), &iterb, &iter);
}

void category_manager_move_item_down(GtkWidget *button, GtkWidget *treeview)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	GtkTreePath *patha;
	GtkTreeIter iter, iterb;
	gchar *category;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));	
	if(!gtk_tree_selection_get_selected(selection, &model, &iter))return;
	gtk_tree_model_get(model, &iter, 0, &category, -1);
	patha = gtk_tree_model_get_path(model, &iter);
	gtk_tree_path_next(patha);
	if(!gtk_tree_model_get_iter(model, &iterb, patha))
	{
		gtk_tree_path_free(patha);
		return;
	}
	gtk_tree_path_free(patha);    

	gtodo_client_category_move_down(cl, category);    
	gtk_list_store_swap(GTK_LIST_STORE(model), &iterb, &iter);
}

void category_manager_delete_item(GtkWidget *button, GtkWidget *treeview)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	GtkTreeIter iter;
	gchar *tm = NULL, *category;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));	
	if(!gtk_tree_selection_get_selected(selection, &model, &iter))return;
	gtk_tree_model_get(model, &iter, 0, &category, -1);

	tm = g_strdup_printf(_("When you delete the category \"%s\", all containing items are lost"), category);
	if(!message_box(tm,_("Delete"),GTK_MESSAGE_WARNING))
	{
		g_free(tm);
		return;
	}
	g_free(tm);
	gtodo_client_category_remove(cl, category);    
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}


void category_manager_add_item(GtkWidget *button, GtkWidget *treeview)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	GtkTreeIter iter;
	GtkTreeSelection *selection;    
	GtkTreePath *path;
	gchar* new_name = NULL;

	gtk_list_store_append(GTK_LIST_STORE(model), &iter);

	/* find the smallest available number to avoid name repetition */
	int number = 0;
	do
	{
		number ++;
		g_free(new_name);
		new_name = g_strdup_printf(_("<New category (%d)>"), number);
	}
	while(gtodo_client_category_exists(cl, new_name));

  /* This is shown in an editable treeview column to show the user
     he should enter the category name here */
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, new_name, 1, 1, -1);
	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path, NULL, FALSE, 0, 0);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_select_iter(selection, &iter);	
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path, gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), 0), TRUE);
	gtk_tree_path_free(path);
	gtodo_client_category_new(cl, new_name);
	g_free(new_name);
}

void tree_edited_string(GtkCellRendererText *cell, const char *path_string, const char *new_text, GtkWidget *treeview)
{
	GtkTreeIter iter;
	char *name;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	GtkTreePath *path= gtk_tree_path_new_from_string(path_string);

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free(path);
	gtk_tree_model_get(model, &iter, 0, &name, -1);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, new_text, -1);
	gtodo_client_category_edit(cl, name, (gchar *)new_text);
	read_categorys();
}
