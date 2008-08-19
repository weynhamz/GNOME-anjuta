#include <gtk/gtk.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "main.h"

void create_notification_window(GTodoItem *item);
void notification_window_cancel(GtkWidget *but_cancel);
void notification_window_remove_notification(GtkWidget *but_cancel, gpointer data);
void notification_window_set_notification(GtkWidget *check_but, gpointer data);
GArray *table;

typedef struct 
{
	GtkWidget *dialog;
	gint id;
} not_window;

int check_for_notification_event(void)
{
	GTodoList *list = NULL;
	int min_b_not = gconf_client_get_int(client, "/apps/gtodo/prefs/notify_in_minutes",NULL);
	if(!gconf_client_get_bool(client, "/apps/gtodo/prefs/do_notification",NULL)) return TRUE;
	list = gtodo_client_get_todo_item_list(cl, NULL);
	if(list == NULL) return TRUE;
	do{
		GTodoItem *item = gtodo_client_get_todo_item_from_list(list);
		if(!gtodo_todo_item_get_done(item) && gtodo_todo_item_get_notify(item))
		{
			if((gtodo_todo_item_check_due(item) == 0 && gtodo_todo_item_check_due_time_minutes_left(item) <= min_b_not)
					|| (gtodo_todo_item_check_due(item) > 0 && gtodo_todo_item_get_due_date_as_julian(item) != GTODO_NO_DUE_DATE))	    
			{
				create_notification_window(item);
			}
		}
	}while(gtodo_client_get_list_next(list));
	gtodo_client_free_todo_item_list(cl, list);
	return TRUE;
}



void create_notification_window(GTodoItem *item)
{
	GtkWidget *dialog, *but_cancel, *image, *ck_but;
	GtkWidget *hbox, *label, *vbox;
	gchar *buffer, *tempstr;
	if(table == NULL) table = g_array_new(TRUE, TRUE, sizeof(GtkWidget *));
	else{
		not_window *test=NULL;
		int i=0;
		do
		{
			test = g_array_index(table, not_window *, i);
			if(test == NULL) break;
			if(test->id == gtodo_todo_item_get_id(item))
			{
				gtk_window_present(GTK_WINDOW(test->dialog));
				return;
			}
			i++;
		}while(test != NULL);
	}
	if(gtodo_todo_item_check_due(item) == 0 && gtodo_todo_item_check_due_time_minutes_left(item) > 0 )
	{
    int minutes = gtodo_todo_item_check_due_time_minutes_left(item);
		tempstr = g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s</span>\n\"%s\"",
			ngettext("The following item is due in %i minute:",
               "The following item is due in %i minutes:", minutes),
      gtodo_todo_item_get_summary(item));
		buffer = g_strdup_printf(tempstr, minutes);
		g_free(tempstr);
	}
	else 
	{	
		buffer = g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s</span>\n\"%s\"",
			_("The following item is due:"),
			gtodo_todo_item_get_summary(item));
	}
	/* create image and more stuff in dialog */
	dialog = gtk_dialog_new();
	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)), 6);
	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 12);
	/* hig stuff */
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(mw.window)); 
	gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW(dialog),"Warning");
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);

	hbox = gtk_hbox_new(FALSE,6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
	vbox= gtk_vbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE,0);
	/* Add the image */
	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	label = gtk_alignment_new(0.5, 0,0,0);
	gtk_container_add(GTK_CONTAINER(label), image);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE,0);
	/* add an vbox*/
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE,12);


	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), buffer);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	image = gtk_alignment_new(0.5,0,0,0);
	gtk_container_add(GTK_CONTAINER(image), label);
	gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE,0);
/*	gtk_container_set_border_width(GTK_CONTAINER(hbox), 9);*/

	ck_but = gtk_check_button_new_with_mnemonic(_("_Do not show again"));
	label = gtk_alignment_new(1, 1,0,0);
	gtk_container_add(GTK_CONTAINER(label), ck_but);
	gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, FALSE,12);    

	g_signal_connect(G_OBJECT(ck_but), "toggled", G_CALLBACK(notification_window_set_notification), GINT_TO_POINTER(gtodo_todo_item_get_id(item)));

	but_cancel = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OPEN, GTK_RESPONSE_CANCEL);

	g_signal_connect(G_OBJECT(but_cancel), "clicked", G_CALLBACK(notification_window_remove_notification),GINT_TO_POINTER(gtodo_todo_item_get_id(item)));

	but_cancel = gtk_dialog_add_button(GTK_DIALOG(dialog),GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL);
	g_signal_connect(G_OBJECT(but_cancel), "clicked", G_CALLBACK(notification_window_cancel), NULL);
	
	/* set window signal */
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(notification_window_cancel), NULL);
	
	g_free(buffer);    
	gtk_widget_show_all(dialog);
	{
		not_window *nw = g_malloc(sizeof(not_window));
		nw->dialog = dialog;
		nw->id = gtodo_todo_item_get_id(item);
		g_array_append_vals(table, &nw, 1);
	}
}

/* destroy the notification window */
void notification_window_cancel(GtkWidget *but_cancel)
{
	GtkWidget *dialog = gtk_widget_get_toplevel(but_cancel);
	not_window *test = NULL;
	int i=0;
	do
	{
		test = g_array_index(table, not_window *, i);
		if(test == NULL) break;
		if(test->dialog == dialog)
		{
			g_free(test);
			g_array_remove_index(table, i);
		}
		i++;
	}while(test != NULL);

	gtk_widget_destroy(dialog);
}

void notification_window_remove_notification(GtkWidget *but_cancel, gpointer data)
{
	gint id = GPOINTER_TO_INT(data);
	notification_window_cancel(but_cancel);
	gui_add_todo_item(NULL, GINT_TO_POINTER(1), id);
}

void notification_window_set_notification(GtkWidget *check_but, gpointer data)
{
	gint id = GPOINTER_TO_INT(data);
	GTodoItem *item = gtodo_client_get_todo_item_from_id(cl, id);
	if(item != NULL)
	{
		gtodo_todo_item_set_notify(item, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_but)));
		gtodo_client_edit_todo_item(cl, item);
	}    
}
