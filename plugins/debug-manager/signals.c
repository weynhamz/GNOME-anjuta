/*
    signals.c
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
#include <ctype.h>

#include <gnome.h>

#include <libanjuta/resources.h>

#include "utilities.h"
#include "signals.h"

static GtkWidget* create_signals_set_dialog (Signals *s);

static void
signals_update_controls (Signals * ew)
{
	gboolean R, Pr;
	
	R = ianjuta_debugger_get_status (ew->debugger, NULL) == IANJUTA_DEBUGGER_OK;
	Pr = ianjuta_debugger_get_status (ew->debugger, NULL) == IANJUTA_DEBUGGER_PROGRAM_RUNNING;
	gtk_widget_set_sensitive (ew->widgets.menu_signal, Pr);
	gtk_widget_set_sensitive (ew->widgets.menu_modify, R);
	gtk_widget_set_sensitive (ew->widgets.menu_update, R);
}

static void
signals_update (const GList * lines, gpointer data)
{
	Signals *sg;
	gint j, count;
	gchar *row[5], *str;
	gchar sig[32], stop[10], print[10], pass[10];
	GList *list, *node;
	
	sg = (Signals *) data;
	signals_clear (sg);
	list = gdb_util_remove_blank_lines (lines);
	if (g_list_length (list) < 2)
	{
		g_list_free (list);
		return;
	}
	gtk_clist_freeze (GTK_CLIST(sg->widgets.clist));
	node = list->next;
	while (node)
	{
		count =
			sscanf ((char *) node->data, "%s %s %s %s", sig, stop, print, pass);
		str = node->data;
		node = g_list_next (node);
		if (count != 4)
			continue;

		/* Do not worry. This is used to avoid the last line */
		if (node == NULL)
			break;
		row[0] = sig;
		row[1] = stop;
		row[2] = print;
		row[3] = pass;
		for (j = 0; j < 4; j++)
		{
			while (isspace (*str))
				str++;
			while (!isspace (*str))
				str++;
		}
		while (isspace (*str))
			str++;
		row[4] = str;
		gtk_clist_append (GTK_CLIST (sg->widgets.clist), row);
	}
	g_list_free (list);
	gtk_clist_thaw (GTK_CLIST(sg->widgets.clist));
}

static void
on_signals_clist_select_row (GtkCList *clist, gint row,
							 gint column, GdkEvent *event, gpointer user_data)
{
    Signals *s = (Signals*)user_data;
    s->idx = row;
}

static gint
on_signals_delete_event(GtkWidget* w, GdkEvent *event, gpointer data)
{
	Signals* cr = (Signals*) data;
	signals_hide (cr);
	return TRUE;
}

static gboolean
on_signals_key_press_event(GtkWidget *widget, GdkEventKey *event,
						   gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		Signals* cr = (Signals*) data;
  		signals_hide(cr);
		return TRUE;
	}
	return FALSE;
}

static void
on_signals_modify_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget* dialog;
    Signals *s = (Signals*)user_data;
	
	dialog = create_signals_set_dialog (s);
	if (dialog)
		gtk_widget_show (dialog);
}

static void
on_signals_send_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	#if 0
	gchar* sig;
    Signals *s = (Signals*)user_data;
	
	if (debugger_program_is_running (s->debugger) == FALSE) return;
	
	signals_show (s);
	gtk_clist_get_text(GTK_CLIST (s->widgets.clist), s->idx, 0, &sig);
	debugger_signal(sig, TRUE);
	#endif
}

static void
on_signals_update_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    Signals *s = (Signals*)user_data;
	ianjuta_debugger_info_signal (s->debugger, signals_update, s, NULL);
}

static gboolean
on_signals_event (GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	GdkEventButton *bevent;
    Signals *ew = (Signals*)user_data;
	
	if (event->type != GDK_BUTTON_PRESS) return FALSE;
	if (((GdkEventButton *)event)->button != 3) return FALSE;
	
	bevent =  (GdkEventButton *)event;
	bevent->button = 1;
	signals_update_controls(ew);
	gtk_menu_popup (GTK_MENU(ew->widgets.menu), NULL,
					NULL, NULL, NULL,
					bevent->button, bevent->time);
	return TRUE;
}

static void
on_signals_togglebutton1_toggled (GtkToggleButton *togglebutton,
								  gpointer user_data)
{
	Signals *sig = (Signals*) user_data;
	
	sig->stop = gtk_toggle_button_get_active (togglebutton);
	if (sig->stop)
		gtk_label_set_text (GTK_LABEL(GTK_BIN (togglebutton)->child), _("Yes"));
    else
		gtk_label_set_text (GTK_LABEL(GTK_BIN (togglebutton)->child), _("No"));
}

static void
on_signals_togglebutton2_toggled (GtkToggleButton *togglebutton,
								  gpointer user_data)
{
	Signals *sig = (Signals*) user_data;
	
	sig->print = gtk_toggle_button_get_active(togglebutton);
	if (sig->print)
		gtk_label_set_text (GTK_LABEL(GTK_BIN(togglebutton)->child), _("Yes"));
    else
		gtk_label_set_text (GTK_LABEL(GTK_BIN(togglebutton)->child), _("No"));
}

static void
on_signals_togglebutton3_toggled (GtkToggleButton *togglebutton,
								  gpointer user_data)
{
	Signals *sig = (Signals*) user_data;
	
	sig->pass = gtk_toggle_button_get_active (togglebutton);
	if (sig->pass)
		gtk_label_set_text (GTK_LABEL(GTK_BIN(togglebutton)->child), _("Yes"));
    else
		gtk_label_set_text (GTK_LABEL(GTK_BIN(togglebutton)->child), _("No"));
}

static void
on_signals_set_ok_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	Signals *s = (Signals*) user_data;

	if(s->stop)
	{
		gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->idx, 1, "Yes");
	}
	else
	{
		gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->idx, 1, "No");
	}
	if(s->print)
	{
		gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->idx, 2, "Yes");
	}
	else
	{
		gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->idx, 2, "No");
	}
	if(s->pass)
	{
		gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->idx, 3, "Yes");
	}
	else
	{
		gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->idx, 3, "No");
	}
	ianjuta_debugger_handle_signal (s->debugger, s->signal, s->stop, s->print, s->pass, NULL);
}

static GnomeUIInfo signals_menu_uiinfo[] =
{
	{
		GNOME_APP_UI_ITEM, N_("Modify Signal"),
		NULL,
		on_signals_modify_activate, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL
	},
	{
		GNOME_APP_UI_ITEM, N_("Send to process"),
		NULL,
		on_signals_send_activate, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL
	},
	{
		GNOME_APP_UI_ITEM, N_("Update"),
		NULL,
		on_signals_update_activate, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL
	},
	GNOMEUIINFO_END
};

static GtkWidget*
create_signals_menu (Signals *s)
{
	GtkWidget *signals_menu;
	
	signals_menu = gtk_menu_new ();
	
	signals_menu_uiinfo[0].user_data = s;
	signals_menu_uiinfo[1].user_data = s;
	signals_menu_uiinfo[2].user_data = s;
	
	gnome_app_fill_menu (GTK_MENU_SHELL (signals_menu), signals_menu_uiinfo,
						 NULL, FALSE, 0);
	return signals_menu;
}

static void
create_signals_gui (Signals *cr)
{
	GtkWidget *window3;
	GtkWidget *scrolledwindow4;
	GtkWidget *clist4;
	GtkWidget *label6, *label7, *label8, *label9, *label10;

	window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize (window3, 170, -2);
	gtk_window_set_title (GTK_WINDOW (window3), _("Kernel signals"));
	gtk_window_set_wmclass (GTK_WINDOW (window3), "signals", "Anjuta");
	gtk_window_set_default_size (GTK_WINDOW (window3), 240, 230);
	gnome_window_icon_set_from_default(GTK_WINDOW(window3));

	scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow4);
	gtk_container_add (GTK_CONTAINER (window3), scrolledwindow4);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	clist4 = gtk_clist_new (5);
	gtk_widget_show (clist4);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), clist4);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 0, 100);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 1, 50);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 2, 50);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 3, 50);
	gtk_clist_set_column_width (GTK_CLIST (clist4), 4, 80);
	gtk_clist_set_selection_mode (GTK_CLIST (clist4), GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist4));
	gtk_clist_set_column_auto_resize (GTK_CLIST(clist4), 4, TRUE);
	
	label6 = gtk_label_new (_("Signal"));
	gtk_widget_show (label6);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 0, label6);
	
	label7 = gtk_label_new (_("Stop"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 1, label7);
	
	label8 = gtk_label_new (_("Print"));
	gtk_widget_show (label8);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 2, label8);
	
	label9 = gtk_label_new (_("Pass"));
	gtk_widget_show (label9);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 3, label9);
	
	label10 = gtk_label_new (_("Description"));
	gtk_widget_show (label10);
	gtk_clist_set_column_widget (GTK_CLIST (clist4), 4, label10);
	
	gtk_signal_connect (GTK_OBJECT (window3), "delete_event",
						GTK_SIGNAL_FUNC (on_signals_delete_event), cr);
	gtk_signal_connect (GTK_OBJECT (window3), "key-press-event",
						GTK_SIGNAL_FUNC (on_signals_key_press_event), cr);
	
	gtk_signal_connect (GTK_OBJECT (clist4), "event",
						GTK_SIGNAL_FUNC (on_signals_event),
						cr);
	gtk_signal_connect (GTK_OBJECT (clist4), "select_row",
						GTK_SIGNAL_FUNC (on_signals_clist_select_row),
						cr);

	cr->widgets.window = window3;
	cr->widgets.clist = clist4;
	cr->widgets.menu = create_signals_menu (cr);
	cr->widgets.menu_modify = signals_menu_uiinfo[0].widget;
	cr->widgets.menu_signal = signals_menu_uiinfo[1].widget;
	cr->widgets.menu_update = signals_menu_uiinfo[2].widget;
}

static GtkWidget*
create_signals_set_dialog (Signals *s)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *hbox2;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *hseparator1;
	GtkWidget *hbox4;
	GtkWidget *label5;
	GtkWidget *label6;
	GtkWidget *label7;
	GtkWidget *hbox3;
	GtkWidget *togglebutton1;
	GtkWidget *togglebutton2;
	GtkWidget *togglebutton3;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	gchar *row[5];
	
	if(s->idx < 0)
		return NULL;
	
	gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->idx,0, &row[0]);
	gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->idx,1, &row[1]);
	gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->idx,2, &row[2]);
	gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->idx,3, &row[3]);
	gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->idx,4, &row[4]);
	s->signal = row[0];
	
	if(strcasecmp(row[1], "Yes")==0)
		s->stop = TRUE;
	else
		s->stop = FALSE;

	if(strcasecmp(row[2], "Yes")==0)
		s->print = TRUE;
	else
		s->print = FALSE;

	if(strcasecmp(row[3], "Yes")==0)
		s->pass = TRUE;
	else
		s->pass = FALSE;
	
	dialog1 = gnome_dialog_new (_("Set Signal Property"), NULL);
	gtk_window_set_transient_for (GTK_WINDOW(dialog1),
								  GTK_WINDOW(s->widgets.window));
	GTK_WINDOW (dialog1)->type = GTK_WINDOW_TOPLEVEL;
	gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_MOUSE);
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog1), "set_signal", "Anjuta");
	gnome_dialog_set_close (GNOME_DIALOG (dialog1), TRUE);
	
	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);
	
	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);
	
	label1 = gtk_label_new ("Signal: ");
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);
	
	label2 = gtk_label_new (row[0]);
	gtk_widget_show (label2);
	gtk_box_pack_start (GTK_BOX (hbox1), label2, FALSE, FALSE, 0);
	
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox2, TRUE, TRUE, 0);
	
	label3 = gtk_label_new ("Description: ");
	gtk_widget_show (label3);
	gtk_box_pack_start (GTK_BOX (hbox2), label3, FALSE, FALSE, 0);
	
	label4 = gtk_label_new (row[4]);
	gtk_widget_show (label4);
	gtk_box_pack_start (GTK_BOX (hbox2), label4, FALSE, FALSE, 0);
	
	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), hseparator1, TRUE, TRUE, 0);
	
	hbox4 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox4);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox4, TRUE, TRUE, 0);
	
	label5 = gtk_label_new (_(" Stop: "));
	gtk_widget_show (label5);
	gtk_box_pack_start (GTK_BOX (hbox4), label5, TRUE, TRUE, 0);
	
	label6 = gtk_label_new (_(" Print: "));
	gtk_widget_show (label6);
	gtk_box_pack_start (GTK_BOX (hbox4), label6, TRUE, TRUE, 0);
	
	label7 = gtk_label_new (_("Pass:"));
	gtk_widget_show (label7);
	gtk_box_pack_start (GTK_BOX (hbox4), label7, TRUE, TRUE, 0);
	
	hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox3);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox3, TRUE, TRUE, 0);
	
	togglebutton1 = gtk_toggle_button_new_with_label (row[1]);
	gtk_widget_show (togglebutton1);
	gtk_box_pack_start (GTK_BOX (hbox3), togglebutton1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (togglebutton1), 3);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton1), s->stop);
	
	togglebutton2 = gtk_toggle_button_new_with_label (row[2]);
	gtk_widget_show (togglebutton2);
	gtk_box_pack_start (GTK_BOX (hbox3), togglebutton2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (togglebutton2), 3);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton2), s->print);
	
	togglebutton3 = gtk_toggle_button_new_with_label (row[3]);
	gtk_widget_show (togglebutton3);
	gtk_box_pack_start (GTK_BOX (hbox3), togglebutton3, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (togglebutton3), 3);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton3), s->pass);
	
	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
							   GTK_BUTTONBOX_SPREAD);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);
	
	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
								GNOME_STOCK_BUTTON_HELP);
	button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
	
	gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
	button2 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button2);
	GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
	
	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
								GNOME_STOCK_BUTTON_CANCEL);
	button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);
	
	gtk_signal_connect (GTK_OBJECT (togglebutton1), "toggled",
						GTK_SIGNAL_FUNC (on_signals_togglebutton1_toggled),
						s);
	gtk_signal_connect (GTK_OBJECT (togglebutton2), "toggled",
						GTK_SIGNAL_FUNC (on_signals_togglebutton2_toggled),
						s);
	gtk_signal_connect (GTK_OBJECT (togglebutton3), "toggled",
						GTK_SIGNAL_FUNC (on_signals_togglebutton3_toggled),
						s);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
						GTK_SIGNAL_FUNC (on_signals_set_ok_clicked),
						s);
	return dialog1;
}

Signals *
signals_new (IAnjutaDebugger *debugger)
{
	Signals *ew;
	ew = g_malloc (sizeof (Signals));
	if (ew)
	{
		ew->debugger = debugger;
		g_object_ref (debugger);
		ew->is_showing = FALSE;
		ew->win_width = 460;
		ew->win_height = 320;
		ew->win_pos_x = 150;
		ew->win_pos_y = 130;
		ew->idx = -1;
		create_signals_gui (ew);
	}
	return ew;
}

void
signals_clear (Signals * sg)
{
	if (GTK_IS_CLIST (sg->widgets.clist))
		gtk_clist_clear (GTK_CLIST (sg->widgets.clist));
	sg->idx = -1;
}

void
signals_show (Signals * ew)
{
	if (ew)
	{
		if (ew->is_showing)
		{
			gdk_window_raise (ew->widgets.window->window);
		}
		else
		{
			gtk_widget_set_uposition (ew->widgets.window, ew->win_pos_x,
									  ew->win_pos_y);
			gtk_window_set_default_size (GTK_WINDOW (ew->widgets.window),
										 ew->win_width, ew->win_height);
			gtk_widget_show (ew->widgets.window);
			ew->is_showing = TRUE;
			ianjuta_debugger_info_signal (ew->debugger, signals_update, ew, NULL);
		}
	}
}

void
signals_hide (Signals * ew)
{
	if (ew)
	{
		if (ew->is_showing == FALSE)
			return;
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
									&ew->win_pos_y);
		gdk_window_get_size (ew->widgets.window->window, &ew->win_width,
							 &ew->win_height);
		gtk_widget_hide (ew->widgets.window);
		ew->is_showing = FALSE;
	}
}

void
signals_free (Signals * sg)
{
	if (sg)
	{
		signals_clear (sg);
		g_object_unref (sg->debugger);
		gtk_widget_destroy (sg->widgets.window);
		gtk_widget_destroy (sg->widgets.menu);
		g_free (sg);
	}
}
