#include <config.h>
#include <gtk/gtk.h>
#include "main.h"

static void preferences_cb_show_date(GtkWidget *chbox);
static void preferences_cb_do_tooltip(GtkWidget *chbox);
static void preferences_cb_auto_purge(GtkWidget *cb, GtkWidget *hbox);
static void preferences_cb_toggle_hl_today(GtkWidget *chbox);
static void preferences_cb_toggle_hl_due(GtkWidget *chbox);
static void preferences_cb_toggle_hl_indays(GtkWidget *chbox);
static void preferences_cb_toggle_show_notification(GtkWidget *chbox);
static void preferences_cb_toggle_show_category_column(GtkWidget *chbox);
static void preferences_cb_toggle_show_priority_column(GtkWidget *chbox);

void gui_preferences(void)
{
	GtkWidget *dialog;
	GtkWidget *notebook;
	
	notebook = preferences_widget ();
	dialog = gtk_dialog_new_with_buttons(_("To-do List Preferences"), GTK_WINDOW(mw.window), 
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, 
			GTK_RESPONSE_CANCEL,
			NULL);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), notebook);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	/* because there not directly visible I can go save the values now.. (not like do_tray) */
	// settings.auto_purge = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_auto_purge));
	// anjuta_preferences_set_bool(preferences,"gtodo.auto-purge",settings.auto_purge,NULL);
	// settings.purge_days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sp_purge_days));
	// anjuta_preferences_set_int(preferences,"gtodo.auto-purge-days",settings.purge_days,NULL);
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
		anjuta_preferences_get_bool(preferences, "gtodo.show-due-column"));
			
	g_signal_connect(G_OBJECT(cb_show_date), "toggled", G_CALLBACK(preferences_cb_show_date), NULL);

	cb = gtk_check_button_new_with_label(_("Show category column"));
	gtk_box_pack_start(GTK_BOX(vbox),cb, FALSE, TRUE, 0); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), anjuta_preferences_get_bool(preferences, "gtodo.show-category-column"));
	g_signal_connect(G_OBJECT(cb), "toggled", G_CALLBACK(preferences_cb_toggle_show_category_column), NULL);

	cb = gtk_check_button_new_with_label(_("Show priority column"));
	gtk_box_pack_start(GTK_BOX(vbox),cb, FALSE, TRUE, 0); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), anjuta_preferences_get_bool(preferences, "gtodo.show-priority-column"));
	g_signal_connect(G_OBJECT(cb), "toggled", G_CALLBACK(preferences_cb_toggle_show_priority_column), NULL);

	cb_list_tooltip = gtk_check_button_new_with_label(_("Tooltips in list"));
	gtk_box_pack_start(GTK_BOX(vbox),cb_list_tooltip, FALSE, TRUE, 0); 
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_list_tooltip),
		anjuta_preferences_get_bool(preferences, "gtodo.show-tooltip"));

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
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox2, gtk_label_new(_("Miscellaneous")));

	vbox = gtk_vbox_new(FALSE, 6);
	cb_auto_purge = gtk_check_button_new_with_label(_("Auto-purge completed items"));
	gtk_box_pack_start(GTK_BOX(vbox),cb_auto_purge, FALSE, TRUE, 0); 	


	hbox = gtk_hbox_new(FALSE, 6);
	/* Translators: First part of the sentence "Purge items after %d days"*/
	label = gtk_label_new(_("Purge items after"));
	gtk_box_pack_start(GTK_BOX(hbox),label, FALSE, TRUE, 0); 
	sp_purge_days = gtk_spin_button_new_with_range(1,365, 1);	
	gtk_box_pack_start(GTK_BOX(hbox),sp_purge_days, FALSE, TRUE, 0); 
	/* Translators: Second part of the sentence "Purge items after %d days"*/
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
	frame = gtk_frame_new(_("Auto-Purge"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);	
	tmp = g_strdup_printf("<b>%s</b>", _("Auto-Purge"));
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
	tmp = g_strdup_printf(ngettext("Allow to-do items to notify me when they are due in %i minute",
                                 "Allow to-do items to notify me when they are due in %i minutes",
                                 anjuta_preferences_get_int(preferences, "gtodo.notify_in_minutes")),
                                 anjuta_preferences_get_int(preferences, "gtodo.notify_in_minutes"));
	cb = gtk_check_button_new_with_label(tmp);
	g_free(tmp);
	gtk_box_pack_start(GTK_BOX(vbox),cb, FALSE, TRUE, 0); 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), anjuta_preferences_get_bool(preferences, "gtodo.do_notification"));
	g_signal_connect(G_OBJECT(cb), "toggled", G_CALLBACK(preferences_cb_toggle_show_notification), NULL);

	gtk_widget_show_all (notebook);
	return notebook;
}

static void preferences_cb_toggle_hl_today(GtkWidget *chbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.hl-today", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
	settings.hl_today =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox));
	category_changed();
}

static void preferences_cb_toggle_hl_due(GtkWidget *chbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.hl-due", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
	settings.hl_due =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox));
	category_changed();    
}

static void preferences_cb_toggle_hl_indays(GtkWidget *chbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.hl-indays",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
	settings.hl_indays =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox));
	category_changed();
}

static void preferences_cb_toggle_show_notification(GtkWidget *chbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.do_notification",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
}

static void preferences_cb_toggle_show_category_column(GtkWidget *chbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.show-category-column",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 3),
			anjuta_preferences_get_bool(preferences, "gtodo.show-category-column"));
}

static void preferences_cb_toggle_show_priority_column(GtkWidget *chbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.show-priority-column",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 1),
			anjuta_preferences_get_bool(preferences, "gtodo.show-priority-column"));
}

static void preferences_cb_show_date(GtkWidget *chbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.show-due-column",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
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
	anjuta_preferences_set_bool(preferences, "gtodo.show-tooltip", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbox)))

	{
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), FALSE);			
	}
	else		
	{
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), TRUE);
	}
}

static void preferences_cb_auto_purge(GtkWidget *cb, GtkWidget *hbox)
{
	anjuta_preferences_set_bool(preferences, "gtodo.auto-purge", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)))
		gtk_widget_set_sensitive(GTK_WIDGET(hbox), TRUE);
	else 
		gtk_widget_set_sensitive(GTK_WIDGET(hbox), FALSE);
}


static void pref_changed_show_tooltip(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mw.treeview), 
                                      !anjuta_preferences_get_bool (preferences, key));
}

static void  pref_changed_ask_delete_category(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.ask_delete_category = anjuta_preferences_get_bool (preferences, key);
}    

static void pref_changed_show_priority_column(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	gtk_tree_view_column_set_visible(
		gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 1),
		anjuta_preferences_get_bool(preferences, "gtodo.show-priority-column"));
}


static void pref_changed_show_category_column(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 3),
                                     anjuta_preferences_get_bool(preferences, "gtodo.show-category-column"));
}

static void pref_changed_show_due_column(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	gtk_tree_view_column_set_visible(
		gtk_tree_view_get_column(GTK_TREE_VIEW(mw.treeview), 2),
		anjuta_preferences_get_bool(preferences, "gtodo.show-due-column"));
}

static void  pref_changed_auto_purge(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.auto_purge = anjuta_preferences_get_bool (preferences, key);;
}    

#if 0
static void  pref_changed_auto_purge_days(GConfClient *client)
{
	settings.purge_days = anjuta_preferences_get_int(preferences, "gtodo.auto-purge-days",NULL);
}    
#endif 

static void  pref_changed_hl_today(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.hl_today = anjuta_preferences_get_bool (preferences, key);
	category_changed();
}

static void  pref_changed_hl_due(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.hl_due = anjuta_preferences_get_bool (preferences, key);
	category_changed();
}

static void  pref_changed_hl_indays(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.hl_indays= anjuta_preferences_get_bool (preferences, key);
	category_changed();
}

static void  pref_changed_hide_due(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.hide_due = anjuta_preferences_get_bool (preferences, key);
	category_changed();
}

static void  pref_changed_hide_done(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.hide_done = anjuta_preferences_get_bool (preferences, key);
	category_changed();
}

static void  pref_changed_hide_nodate(AnjutaPreferences *preferences, const char *key, gpointer data)
{
	settings.hide_nodate = anjuta_preferences_get_bool (preferences, key);
	category_changed();
}

void pref_set_notifications(AnjutaPreferences *preferences)
{
	anjuta_preferences_notify_add(preferences,"gtodo.show-tooltip",
			pref_changed_show_tooltip,
			NULL);
	anjuta_preferences_notify_add(preferences,"gtodo.ask-delete-category",
			pref_changed_ask_delete_category,
			NULL);
	anjuta_preferences_notify_add(preferences,"gtodo.show-due-column",
			pref_changed_show_due_column,
			NULL);
	anjuta_preferences_notify_add(preferences,"gtodo.show-category-column",
			pref_changed_show_category_column,
			NULL);
	anjuta_preferences_notify_add(preferences,"gtodo.show-priority-column",
			pref_changed_show_priority_column,
			NULL);
	

	anjuta_preferences_notify_add(preferences,"gtodo.auto-purge",
			pref_changed_auto_purge,
			NULL);
	
	anjuta_preferences_notify_add(preferences,"gtodo.hl-today",
			pref_changed_hl_today,
			NULL);	

	anjuta_preferences_notify_add(preferences,"gtodo.hl-due",
			pref_changed_hl_due,
			NULL);	
	anjuta_preferences_notify_add(preferences,"gtodo.hl-indays",
			pref_changed_hl_indays,
			NULL);
	
	anjuta_preferences_notify_add(preferences,"gtodo.hide-done",
			pref_changed_hide_done,
			NULL);	

	anjuta_preferences_notify_add(preferences,"gtodo.hide-due",
			pref_changed_hide_due,
			NULL);	
	anjuta_preferences_notify_add(preferences,"gtodo.hide-nodate",
			pref_changed_hide_nodate,
			NULL);
}
