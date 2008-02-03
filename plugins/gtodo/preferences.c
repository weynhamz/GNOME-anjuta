#include <config.h>
#include <gtk/gtk.h>
#include <tray-icon.h>
#include "main.h"

void preferences_cb_do_tray(GtkWidget *chbox, GtkWidget *hwin);
static void preferences_cb_show_date(GtkWidget *chbox);
static void preferences_cb_do_tooltip(GtkWidget *chbox);
static void preferences_cb_auto_purge(GtkWidget *cb, GtkWidget *hbox);
static void preferences_cb_toggle_hl_today(GtkWidget *chbox);
static void preferences_cb_toggle_hl_due(GtkWidget *chbox);
static void preferences_cb_toggle_hl_indays(GtkWidget *chbox);
static void preferences_cb_toggle_show_notification(GtkWidget *chbox);
static void preferences_cb_toggle_show_category_column(GtkWidget *chbox);
static void preferences_cb_toggle_show_priority_column(GtkWidget *chbox);
static void preferences_cb_toggle_enable_tray(GtkWidget *chbox);

void gui_preferences(void)
{
	GtkWidget *dialog;
	GtkWidget *notebook;
	
	notebook = preferences_widget ();
	dialog = gtk_dialog_new_with_buttons(_("Todo List Preferences"), GTK_WINDOW(mw.window), 
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, 
			GTK_RESPONSE_CANCEL,
			NULL);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), notebook);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	/* because there not directly visible I can go save the values now.. (not like do_tray) */
	// settings.auto_purge = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_auto_purge));
	// gconf_client_set_bool(client,"/apps/gtodo/prefs/auto-purge",settings.auto_purge,NULL);
	// settings.purge_days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sp_purge_days));
	// gconf_client_set_int(client,"/apps/gtodo/prefs/auto-purge-days",settings.purge_days,NULL);
	gtk_widget_destroy(dialog);
	/* save the settings */
}

static GtkWidget *cb_show_date = NULL;
static GtkWidget *cb_list_tooltip = NULL;
static GtkWidget *cb = NULL;
static GtkWidget *cb_auto_purge = NULL;
static GtkWidget *sp_purge_days = NULL;
static GtkWidget *cb_hl_due = NULL;
static GtkWidget *cb_hl_today = NULL;
static GtkWidget *cb_hl_indays = NULL;
static GtkWidget *hbox = NULL;

void preferences_remove_signals()
{
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb_show_date), 
		G_CALLBACK(preferences_cb_show_date), NULL);
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb),
		G_CALLBACK(preferences_cb_toggle_show_category_column), NULL);	
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb),
		G_CALLBACK(preferences_cb_toggle_show_priority_column), NULL);
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb_list_tooltip),
		G_CALLBACK(preferences_cb_do_tooltip), NULL);
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb_hl_today), 
		G_CALLBACK( preferences_cb_toggle_hl_today), NULL);
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb_hl_due),
		G_CALLBACK( preferences_cb_toggle_hl_due), NULL);	
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb_auto_purge),
		G_CALLBACK(preferences_cb_auto_purge), hbox);
	g_signal_handlers_disconnect_by_func(G_OBJECT(cb),
		G_CALLBACK(preferences_cb_toggle_show_notification), NULL);
}
			
GtkWidget *preferences_widget()
{
	GtkWidget *vbox,  *vbox2;
	GtkWidget *frame, *label;
	
	GtkWidget *notebook;
	gchar *tmp;

	notebook = gtk_notebook_new();

	/********* FIRST TAB ***************/
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 12);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox2, gtk_label_new(_("Interface")));

	/** second tab **/
	vbox = gtk_vbox_new(FALSE, 6);
	cb_show_date = gtk_check_button_new_with_label(_("Show due date column"));
	gtk_box_pack_start(GTK_BOX(vbox),cb_show_date, FALSE, TRUE, 0); 
	/* set it to current value and add signal handler */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_show_date), 
		gconf_client_get_bool(client, "/apps/gtodo/prefs/show-due-column",NULL));
			
	g_signal_connect(G_OBJECT(cb_show_date), "toggled", G_CALLBACK(preferences_cb_show_date), NULL);

	cb = gtk_check_button_new_with_label(_("Show category column"));
	gtk_box_pack_start(GTK_BOX(vbox),cb, FALSE, TRUE, 0); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), gconf_client_get_bool(client, "/apps/gtodo/view/show-category-column",NULL));
	g_signal_connect(G_OBJECT(cb), "toggled", G_CALLBACK(preferences_cb_toggle_show_category_column), NULL);

	cb = gtk_check_button_new_with_label(_("Show priority column"));
	gtk_box_pack_start(GTK_BOX(vbox),cb, FALSE, TRUE, 0); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), gconf_client_get_bool(client, "/apps/gtodo/view/show-priority-column",NULL));
	g_signal_connect(G_OBJECT(cb), "toggled", G_CALLBACK(preferences_cb_toggle_show_priority_column), NULL);

	cb_list_tooltip = gtk_check_button_new_with_label(_("Tooltips in list"));
	gtk_box_pack_start(GTK_BOX(vbox),cb_list_tooltip, FALSE, TRUE, 0); 
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_list_tooltip),
		gconf_client_get_bool(client, "/apps/gtodo/prefs/show-tooltip",NULL));      

	g_signal_connect(G_OBJECT(cb_list_tooltip), "toggled", G_CALLBACK(preferences_cb_do_tooltip), NULL);



	/** add the page **/
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	frame = gtk_frame_new(_("Show in main window"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);	
	tmp = g_strdup_printf("<b>%s</b>", _("Show in main window"));
	gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))), tmp);
	g_free(tmp);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	gtk_box_pack_start(GTK_BOX(vbox2),frame, FALSE, TRUE, 0); 



	vbox = gtk_vbox_new(FALSE, 0);
	frame = gtk_frame_new(_("Highlight"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);	
	tmp = g_strdup_printf("<b>%s</b>", _("Highlight"));
	gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))), tmp);
	g_free(tmp);
	
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_box_pack_start(GTK_BOX(vbox2), frame, TRUE, TRUE, 0);

	/* tb for highlighting due today */
	cb_hl_today = 	gtk_check_button_new_with_label(_("Items that are due today"));
	gtk_box_pack_start(GTK_BOX(vbox),cb_hl_today, FALSE, TRUE, 6); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_hl_today), settings.hl_today);
	g_signal_connect(G_OBJECT(cb_hl_today),"toggled", G_CALLBACK( preferences_cb_toggle_hl_today), NULL);

	/* tb for highlighting due */
	cb_hl_due = gtk_check_button_new_with_label(_("Items that are past due"));
	gtk_box_pack_start(GTK_BOX(vbox),cb_hl_due, FALSE, TRUE, 6); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_hl_due), settings.hl_due);
	g_signal_connect(G_OBJECT(cb_hl_due), "toggled",G_CALLBACK( preferences_cb_toggle_hl_due), NULL);	

	/* tb for highlighting in x days */
	tmp = g_strdup_printf(ngettext("Items that are due in the next %i day",
                                 "Items that are due in the next %i days", settings.due_days),
                                 settings.due_days);
	cb_hl_indays = gtk_check_button_new_with_label(tmp);
	g_free(tmp);
	gtk_box_pack_start(GTK_BOX(vbox),cb_hl_indays, FALSE, TRUE, 6); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_hl_indays), settings.hl_indays);
	g_signal_connect(G_OBJECT(cb_hl_indays),"toggled", G_CALLBACK( preferences_cb_toggle_hl_indays), NULL);



	/************ Second Tab *****************/
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 12);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox2, gtk_label_new(_("Misc")));

	vbox = gtk_vbox_new(FALSE, 6);
	cb_auto_purge = gtk_check_button_new_with_label(_("Auto purge completed items"));
	gtk_box_pack_start(GTK_BOX(vbox),cb_auto_purge, FALSE, TRUE, 0); 	


	hbox = gtk_hbox_new(FALSE, 6); 
	label = gtk_label_new(_("Purge items after"));
	gtk_box_pack_start(GTK_BOX(hbox),label, FALSE, TRUE, 0); 
	sp_purge_days = gtk_spin_button_new_with_range(1,365, 1);	
	gtk_box_pack_start(GTK_BOX(hbox),sp_purge_days, FALSE, TRUE, 0); 
	label = gtk_label_new(_("days."));
	gtk_box_pack_start(GTK_BOX(hbox),label, FALSE, TRUE, 0); 

	gtk_box_pack_start(GTK_BOX(vbox),hbox, FALSE, TRUE, 0); 
	g_signal_connect(G_OBJECT(cb_auto_purge), "toggled", G_CALLBACK(preferences_cb_auto_purge), hbox);

	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_auto_purge), settings.auto_purge);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(sp_purge_days), settings.purge_days);
	/* make sure the sensitive is ok */
	preferences_cb_auto_purge(cb_auto_purge, hbox);
	/** add the page **/
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	frame = gtk_frame_new(_("Auto Purge"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);	
	tmp = g_strdup_printf("<b>%s</b>", _("Auto Purge"));
	gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))), tmp);
	g_free(tmp);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE,FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 6);

	/** add the page **/
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	frame = gtk_frame_new(_("Notification"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);	
	tmp = g_strdup_printf("<b>%s</b>", _("Notification"));
	gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))), tmp);
	g_free(tmp);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 0);
	tmp = g_strdup_printf(ngettext("Allow todo items to notifiy me when they are due in %i minute",
                                 "Allow todo items to notifiy me when they are due in %i minutes",
                                 gconf_client_get_int(client, "/apps/gtodo/prefs/notify_in_minutes",NULL)),
                                 gconf_client_get_int(client, "/apps/gtodo/prefs/notify_in_minutes",NULL));
	cb = gtk_check_button_new_with_label(tmp);
	g_free(tmp);
	gtk_box_pack_start(GTK_BOX(vbox),cb, FALSE, TRUE, 0); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), gconf_client_get_bool(client, "/apps/gtodo/prefs/do_notification",NULL));
	g_signal_connect(G_OBJECT(cb), "toggled", G_CALLBACK(preferences_cb_toggle_show_notification), NULL);

	cb = gtk_check_button_new_with_label(_("Show Notification Tray Icon"));
	gtk_box_pack_start(GTK_BOX(vbox),cb, FALSE, TRUE, 0); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), 
		gconf_client_get_bool(client, "/apps/gtodo/view/enable-tray",NULL));
	g_signal_connect(G_OBJECT(cb), "toggled", 
			G_CALLBACK(preferences_cb_toggle_enable_tray), NULL);
	gtk_widget_show_all (notebook);
	return notebook;
}

static void preferences_cb_toggle_hl_today(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/prefs/hl-today", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);	
	settings.hl_today =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox));
	category_changed();
}

static void preferences_cb_toggle_hl_due(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/prefs/hl-due", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);	
	settings.hl_due =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox));
	category_changed();    
}

static void preferences_cb_toggle_hl_indays(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/prefs/hl-indays", 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);
	settings.hl_indays =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox));
	category_changed();
}

static void preferences_cb_toggle_enable_tray(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/view/enable-tray", 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);
}

static void preferences_cb_toggle_show_notification(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/prefs/do_notification", 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);	
}

static void preferences_cb_toggle_show_category_column(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/view/show-category-column", 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);	
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 3),
			gconf_client_get_bool(client, "/apps/gtodo/view/show-category-column",NULL));
}

static void preferences_cb_toggle_show_priority_column(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/view/show-priority-column", 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);	
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 1),
			gconf_client_get_bool(client, "/apps/gtodo/view/show-priority-column",NULL));
}

static void preferences_cb_show_date(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/prefs/show-due-column", 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);	
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)))
	{
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 2),TRUE);
	}
	else	
	{
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 2),FALSE);
	}		
}

static void preferences_cb_do_tooltip(GtkWidget *chbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/prefs/show-tooltip", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)),NULL);	
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)))

	{
		message_box(_(	"Showing Tooltips in the todo list is still very alpha.\n"
				"Because of some weird behaviour in gtk it only works with the column headers disabled.\n"
				"I hope to get this fixed soon"),"", GTK_MESSAGE_INFO);
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), FALSE);			
	}
	else		
	{

		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), TRUE);
	}		
}

static void preferences_cb_auto_purge(GtkWidget *cb, GtkWidget *hbox)
{
	gconf_client_set_bool(client, "/apps/gtodo/prefs/auto-purge", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)),NULL);	
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)))
		gtk_widget_set_sensitive(GTK_WIDGET(hbox), TRUE);
	else 
		gtk_widget_set_sensitive(GTK_WIDGET(hbox), FALSE);
}


static void pref_gconf_changed_show_tooltip(GConfClient *client)
{
	if(gconf_client_get_bool(client, "/apps/gtodo/prefs/show-tooltip", NULL))
	{
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), FALSE);			    
	}
	else gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), TRUE);			    
}

static void  pref_gconf_changed_restore_size(GConfClient *client)
{
	settings.size = gconf_client_get_bool(client, /* "/schemas*/"/apps/gtodo/prefs/restore-size",NULL);
}    
static void  pref_gconf_changed_restore_place(GConfClient *client)
{
	settings.place = gconf_client_get_bool(client,/* "/schemas*/"/apps/gtodo/prefs/restore-position",NULL);
}    
static void  pref_gconf_changed_ask_delete_category(GConfClient *client)
{
	settings.ask_delete_category = gconf_client_get_bool(client,/* "/schemas*/"/apps/gtodo/prefs/ask-delete-category",NULL);
}    

static void pref_gconf_changed_show_priority_column(GtkWidget *chbox)
{
	gtk_tree_view_column_set_visible(
		gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 1),
		gconf_client_get_bool(client, "/apps/gtodo/view/show-priority-column",NULL));
}


static void pref_gconf_changed_show_category_column(GtkWidget *chbox)
{
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 3),gconf_client_get_bool(client, "/apps/gtodo/view/show-category-column",NULL));
}

static void pref_gconf_changed_show_due_column(GtkWidget *chbox)
{
	gtk_tree_view_column_set_visible(
		gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 2),
		gconf_client_get_bool(client, "/apps/gtodo/prefs/show-due-column",NULL));
}

static void  pref_gconf_changed_auto_purge(GConfClient *client)
{
	settings.auto_purge = gconf_client_get_bool(client,"/apps/gtodo/prefs/auto-purge",NULL);
}    

#if 0
static void  pref_gconf_changed_auto_purge_days(GConfClient *client)
{
	settings.purge_days = gconf_client_get_int(client, "/apps/gtodo/prefs/auto-purge-days",NULL);
}    
#endif 

static void  pref_gconf_changed_hl_today(GConfClient *client)
{
	settings.hl_today = gconf_client_get_bool(client,"/apps/gtodo/prefs/hl-today",NULL);
	category_changed();
}

static void  pref_gconf_changed_hl_due(GConfClient *client)
{
	settings.hl_due = gconf_client_get_bool(client, "/apps/gtodo/prefs/hl-due",NULL);
	category_changed();
}

static void  pref_gconf_changed_enable_tray(GConfClient *client)
{
	if(gconf_client_get_bool(client, "/apps/gtodo/view/enable-tray",NULL))
	{
	    if (mw.window)
		tray_init(mw.window);	
	}
	else
	{
		tray_icon_remove();
	}
}

static void  pref_gconf_changed_hl_indays(GConfClient *client)
{
	settings.hl_indays= gconf_client_get_bool(client,"/apps/gtodo/prefs/hl-indays",NULL);
	category_changed();
}

void pref_gconf_set_notifications(GConfClient *client)
{
	gconf_client_notify_add(client, "/apps/gtodo/prefs/restore-size",
			(GConfClientNotifyFunc)pref_gconf_changed_restore_size,
			NULL,
			NULL, NULL);

	gconf_client_notify_add(client,"/apps/gtodo/prefs/restore-position",
			(GConfClientNotifyFunc) pref_gconf_changed_restore_place,
			NULL,
			NULL, NULL);
	gconf_client_notify_add(client,"/apps/gtodo/prefs/show-tooltip",
			(GConfClientNotifyFunc) pref_gconf_changed_show_tooltip,
			NULL,
			NULL, NULL);
	gconf_client_notify_add(client,"/apps/gtodo/prefs/ask-delete-category",
			(GConfClientNotifyFunc) pref_gconf_changed_ask_delete_category,
			NULL,
			NULL, NULL);
	gconf_client_notify_add(client,"/apps/gtodo/prefs/show-due-column",
			(GConfClientNotifyFunc) pref_gconf_changed_show_due_column,
			NULL,
			NULL, NULL);
	gconf_client_notify_add(client,"/apps/gtodo/view/show-category-column",
			(GConfClientNotifyFunc) pref_gconf_changed_show_category_column,
			NULL,
			NULL, NULL);
	gconf_client_notify_add(client,"/apps/gtodo/view/show-priority-column",
			(GConfClientNotifyFunc) pref_gconf_changed_show_priority_column,
			NULL,
			NULL, NULL);
	

	gconf_client_notify_add(client,"/apps/gtodo/prefs/auto-purge",
			(GConfClientNotifyFunc) pref_gconf_changed_auto_purge,
			NULL,
			NULL, NULL);
	
	gconf_client_notify_add(client,"/apps/gtodo/prefs/hl-today",
			(GConfClientNotifyFunc) pref_gconf_changed_hl_today,
			NULL,
			NULL, NULL);	

	gconf_client_notify_add(client,"/apps/gtodo/prefs/hl-due",
			(GConfClientNotifyFunc) pref_gconf_changed_hl_due,
			NULL,
			NULL, NULL);	
	gconf_client_notify_add(client,"/apps/gtodo/prefs/hl-indays",
			(GConfClientNotifyFunc) pref_gconf_changed_hl_indays,
			NULL,
			NULL, NULL);	

	gconf_client_notify_add(client,"/apps/gtodo/view/enable-tray",
			(GConfClientNotifyFunc) pref_gconf_changed_enable_tray,
			NULL,
			NULL, NULL);	
	
}
