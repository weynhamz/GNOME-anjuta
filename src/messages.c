/*
    messages.c
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
#include "anjuta.h"
#include "messages.h"
#include "utilities.h"
#include "pixmaps.h"
#include "resources.h"
#include "properties.h"

static void on_mesg_win_but1_clicked (GtkButton * but, gpointer data);

static void on_mesg_win_but2_clicked (GtkButton * but, gpointer data);

static void on_mesg_win_but3_clicked (GtkButton * but, gpointer data);

static void on_mesg_win_but4_clicked (GtkButton * but, gpointer data);

static void
on_mesg_win_orien_changed (GtkToolbar * t, GtkOrientation or, gpointer data);

gboolean
on_mesg_win_but_event (GtkWidget * widget,
		       GdkEvent * event, gpointer user_data);

Messages *
messages_new ()
{
	Messages *m;
	m = g_malloc (sizeof (Messages));
	if (m)
	{
		gint i;
		for (i = 0; i < MESSAGE_TYPE_END; i++)
			m->line_buffer[i] =
				(gchar *) g_malloc (sizeof (gchar) *
						    (FILE_BUFFER_SIZE + 1));

		if (m->line_buffer[0] && m->line_buffer[1]
		    && m->line_buffer[2] && m->line_buffer[3])
		{
			for (i = 0; i < MESSAGE_TYPE_END; i++)
				m->cur_char_pos[i] = 0;

			create_mesg_gui (m);

			m->cur_type = MESSAGE_BUILD;
			m->is_showing = TRUE;
			m->is_docked = TRUE;
			m->win_pos_x = 50;
			m->win_pos_y = 50;
			m->win_width = 600;
			m->win_height = 300;
			m->toolbar_pos = GNOME_DOCK_LEFT;

			for (i = 0; i < MESSAGE_TYPE_END; i++)
				m->data[i] = NULL;
			for (i = 0; i < MESSAGE_TYPE_END; i++)
				m->current_pos[i] = -1;

			m->color_red.pixel = 16;
			m->color_red.red = (guint16) - 1;
			m->color_red.green = 0;
			m->color_red.blue = 0;
			m->color_green.pixel = 16;
			m->color_green.red = 0;
			m->color_green.green = (guint16) - 1;
			m->color_green.blue = 0;
			m->color_blue.pixel = 16;
			m->color_blue.red = 0;
			m->color_blue.green = 0;
			m->color_blue.blue = (guint16) - 1;
			m->color_black.pixel = 16;
			m->color_black.red = 0;
			m->color_black.green = 0;
			m->color_black.blue = 0;
		}
	}
	return m;
}

void
messages_destroy (Messages * m)
{
	if (m)
	{
		gint i;
		messages_clear (m, MESSAGE_BUILD);
		messages_clear (m, MESSAGE_DEBUG);
		messages_clear (m, MESSAGE_FIND);
		messages_clear (m, MESSAGE_CVS);

		for (i = 0; i < MESSAGE_TYPE_END; i++)
			if (m->line_buffer[i])
				g_free (m->line_buffer[i]);

		gtk_widget_unref (m->GUI);
		gtk_widget_unref (m->client_area);
		gtk_widget_unref (m->client);
		gtk_widget_unref (m->extra_toolbar);

		for (i = 0; i < MESSAGE_TYPE_END; i++)
			gtk_widget_unref (GTK_WIDGET (m->scrolledwindow[i]));
		for (i = 0; i < MESSAGE_TYPE_END; i++)
			gtk_widget_unref (GTK_WIDGET (m->clist[i]));
		for (i = 0; i < MESSAGE_TYPE_END; i++)
			gtk_widget_unref (GTK_WIDGET (m->but[i]));

		if (m->GUI)
			gtk_widget_destroy (m->GUI);
		g_free (m);
		m = NULL;
	}
}

void
messages_show (Messages * m, MessageType type)
{
	if (m)
	{
		if (type < MESSAGE_TYPE_END)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (m->but[type]), TRUE);
		}
		if (m->is_showing)
		{
			if (m->is_docked == FALSE)
				gdk_window_raise (m->GUI->window);
			return;
		}
		if (m->is_docked)
		{
			messages_attach (m);
		}
		else		/* Is not docked */
		{
			gtk_widget_set_uposition (m->GUI, m->win_pos_x,
						  m->win_pos_y);
			gtk_window_set_default_size (GTK_WINDOW (m->GUI),
						     m->win_width,
						     m->win_height);
			gtk_widget_show (m->GUI);
		}
		m->is_showing = TRUE;
	}
}

void
messages_hide (Messages * m)
{
	if (m)
	{
		if (m->is_showing == FALSE)
			return;
		if (m->is_docked == TRUE)
		{
			messages_detach (m);
		}
		else		/* Is not docked */
		{
			gdk_window_get_root_origin (m->GUI->window,
						    &m->win_pos_x,
						    &m->win_pos_y);
			gdk_window_get_size (m->GUI->window, &m->win_width,
					     &m->win_height);
			gtk_widget_hide (m->GUI);
		}
		m->is_showing = FALSE;
	}
}

void
messages_clear (Messages * m, MessageType type)
{
	g_return_if_fail (m != NULL);
	g_return_if_fail (type < MESSAGE_TYPE_END);

	glist_strings_free(m->data[type]);
	m->data[type] = NULL;

	m->current_pos[type] = -1;
	gtk_clist_freeze (m->clist[type]);
	gtk_clist_clear (m->clist[type]);
	gtk_clist_thaw (m->clist[type]);
}

void
messages_append (Messages * m, gchar * string, MessageType type)
{
	gint i;
	gboolean update_adj;
	GtkAdjustment *adj;

	if (type >= MESSAGE_TYPE_END)
	{
		g_warning (_("Unknown type in Message window\n"));
		return;
	}

	gtk_clist_freeze (m->clist[type]);

	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
						     (m->scrolledwindow[type]));

	/* If the scrollbar is almost at the end, */
	/* then only we scroll to the end */
	update_adj = FALSE;
	if (adj->value > 0.95 * (adj->upper-adj->page_size) || adj->value == 0)
		update_adj = TRUE;

	for (i = 0; i < strlen (string); i++)
	{
		if (string[i] == '\n'|| m->cur_char_pos[type] >= FILE_BUFFER_SIZE)
		{
			m->line_buffer[type][m->cur_char_pos[type]] = '\0';
			messages_add_line (m, type);
			if (m->cur_char_pos[type] >= FILE_BUFFER_SIZE)
			{
				strcpy (m->line_buffer[type], "      ");
				m->cur_char_pos[type] = strlen ("      ") - 1;
			}
			else
			{
				m->cur_char_pos[type] = 0;
			}
		}
		else
		{
			m->line_buffer[type][m->cur_char_pos[type]] =
				string[i];
			m->cur_char_pos[type]++;
		}
	}
	gtk_clist_thaw (m->clist[type]);
	if (update_adj) 
		gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

void
messages_add_line (Messages * m, MessageType type)
{
	gchar *item, *str1, *str2, *str3;
	gchar *dummy_fn;
	gint dummy_int;
	gint i, truncat_mesg, mesg_first, mesg_last;
	Preferences *p;

	p = app->preferences;
	g_return_if_fail (m != NULL);
	g_return_if_fail (type <MESSAGE_TYPE_END);
	
	truncat_mesg = preferences_get_int (p, TRUNCAT_MESSAGES);
	mesg_first = preferences_get_int (p, TRUNCAT_MESG_FIRST);
	mesg_last = preferences_get_int (p, TRUNCAT_MESG_LAST);

	item = g_strdup (m->line_buffer[type]);
	m->data[type] = g_list_append (m->data[type], item);

	if (truncat_mesg == FALSE
	    || strlen (item) <= (mesg_first + mesg_last))
	{
		if (m->clist[type])
			gtk_clist_append (m->clist[type], &item);
	}
	else
	{
		str1 = strdup (item);
		str1[mesg_first] = '\0';
		str2 = &item[strlen (item) - mesg_last];
		str3 =
			g_strconcat (str1, " ................... ", str2,
				     NULL);
		if (m->clist[type])
			gtk_clist_append (m->clist[type], &str3);
		g_free (str1);
		g_free (str3);
	}
	if (parse_error_line (item, &dummy_fn, &dummy_int))
	{
		gtk_clist_set_foreground (m->clist[type],
					  g_list_length (m->data[type]) - 1,
					  &(m->color_red));
		g_free (dummy_fn);
	}
	else
	{
		for (i = 0; i < strlen (item); i++)
		{
			if (item[i] == ':')
			{
				gtk_clist_set_foreground (m->clist[type],
							  g_list_length (m->
									 data
									 [type])
							  - 1,
							  &(m->color_blue));
				break;
			}
		}
		if (i >= strlen (item))
			gtk_clist_set_foreground (m->clist[type],
						  g_list_length (m->
								 data[type]) -
						  1, &(m->color_black));
	}
	m->cur_char_pos[type] = 0;
}

gboolean messages_save_yourself (Messages * m, FILE * stream)
{
	if (!m)
		return FALSE;

	fprintf (stream, "messages.is.docked=%d\n", m->is_docked);
	if (m->is_showing && !m->is_docked)
	{
		gdk_window_get_root_origin (m->GUI->window, &m->win_pos_x,
					    &m->win_pos_y);
		gdk_window_get_size (m->GUI->window, &m->win_width,
				     &m->win_height);
	}
	fprintf (stream, "messages.win.pos.x=%d\n", m->win_pos_x);
	fprintf (stream, "messages.win.pos.y=%d\n", m->win_pos_y);
	fprintf (stream, "messages.win.width=%d\n", m->win_width);
	fprintf (stream, "messages.win.height=%d\n", m->win_height);
	return TRUE;
}

gboolean messages_load_yourself (Messages * m, PropsID props)
{
	gboolean dock_flag;
	if (!m)
		return FALSE;

	dock_flag = prop_get_int (props, "messages.is.docked", 0);
	m->win_pos_x = prop_get_int (props, "messages.win.pos.x", 50);
	m->win_pos_y = prop_get_int (props, "messages.win.pos.y", 50);
	m->win_width = prop_get_int (props, "messages.win.width", 600);
	m->win_height = prop_get_int (props, "messages.win.height", 300);
	if (dock_flag)
		messages_dock (m);
	else
		messages_undock (m);
	return TRUE;
}

void
messages_update (Messages * m)
{
	gchar *item, *str1, *str2, *str3;
	gchar *dummy_fn;
	gint dummy_int;
	gint i, j;
	MessageType type;
	Preferences *p;
	GtkAdjustment *adj;
	guint adj_value_save;
	gint truncat_mesg, mesg_first, mesg_last;

	p = app->preferences;

	if (!m)
		return;
	truncat_mesg = preferences_get_int (p, TRUNCAT_MESSAGES);
	mesg_first = preferences_get_int (p, TRUNCAT_MESG_FIRST);
	mesg_last = preferences_get_int (p, TRUNCAT_MESG_LAST);
	for (type = MESSAGE_BUILD; type < MESSAGE_TYPE_END; type++)
	{
		gtk_clist_freeze (GTK_CLIST (m->clist[type]));
		adj =
			gtk_scrolled_window_get_vadjustment
			(GTK_SCROLLED_WINDOW (m->scrolledwindow[type]));
		adj_value_save = adj->value;
		gtk_clist_clear (GTK_CLIST (m->clist[type]));
		for (j = 0; j < g_list_length (m->data[type]); j++)
		{
			item = g_list_nth_data (m->data[type], j);
			if (truncat_mesg == FALSE
			    || strlen (item) <= (mesg_first + mesg_last))
			{
				if (m->clist[type])
					gtk_clist_append (m->clist[type],
							  &item);
			}
			else
			{
				str1 = strdup (item);
				str1[mesg_first] = '\0';
				str2 = &item[strlen (item) - mesg_last];
				str3 =
					g_strconcat (str1,
						     " ................... ",
						     str2, NULL);
				if (m->clist[type])
					gtk_clist_append (m->clist[type],
							  &str3);
				g_free (str1);
				g_free (str3);
			}
			if (parse_error_line (item, &dummy_fn, &dummy_int))
			{
				gtk_clist_set_foreground (m->clist[type], j,
							  &(m->color_red));
				g_free (dummy_fn);
			}
			else
			{
				for (i = 0; i < strlen (item); i++)
				{
					if (item[i] == ':')
					{
						gtk_clist_set_foreground (m->
									  clist
									  [type],
									  j,
									  &
									  (m->
									   color_blue));
						break;
					}
				}
				if (i >= strlen (item))
					gtk_clist_set_foreground (m->
								  clist[type],
								  j,
								  &(m->
								    color_black));
			}
		}
		gtk_adjustment_set_value (adj, adj_value_save);
		gtk_clist_thaw (GTK_CLIST (m->clist[type]));
	}
}

void
create_mesg_gui (Messages * m)
{
	GtkWidget *mesg_gui;
	GtkWidget *eventbox1;
	GtkWidget *dock1;
	GtkWidget *dock_item1;
	GtkWidget *toolbar1;
	GtkWidget *button1;
	GtkWidget *frame2;
	GtkWidget *frame3;
	GtkWidget *hbox3;
	GtkWidget *scrolledwindow1;
	GtkWidget *scrolledwindow2;
	GtkWidget *scrolledwindow3;
	GtkWidget *scrolledwindow4;
	GtkWidget *mesg_clist1;
	GtkWidget *mesg_clist2;
	GtkWidget *mesg_clist3;
	GtkWidget *mesg_clist4;
	GtkWidget *button5;
	GtkWidget *button6;
	GtkWidget *button7;
	GtkWidget *button8;
	GtkWidget *pix_lab;

	gint i;

	mesg_gui = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_ref (mesg_gui);
	gtk_widget_set_usize (mesg_gui, 200, 70);
	gtk_window_set_title (GTK_WINDOW (mesg_gui), _("Messages"));
	gtk_window_set_wmclass (GTK_WINDOW (mesg_gui), "messages", "Anjuta");

	eventbox1 = gtk_event_box_new ();
	gtk_widget_ref (eventbox1);
	gtk_container_add (GTK_CONTAINER (mesg_gui), eventbox1);
	gtk_widget_show (eventbox1);

	dock1 = gnome_dock_new ();
	gtk_widget_ref (dock1);
	gtk_widget_show (dock1);

	dock_item1 = gnome_dock_item_new ("text_toolbar",
					  GNOME_DOCK_ITEM_BEH_EXCLUSIVE);
	gtk_widget_show (dock_item1);
	gnome_dock_add_item (GNOME_DOCK (dock1), GNOME_DOCK_ITEM (dock_item1),
			     GNOME_DOCK_LEFT, 1, 0, 0, 0);
	gnome_dock_item_set_shadow_type (GNOME_DOCK_ITEM (dock_item1),
					 GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (dock_item1), 2);

	toolbar1 =
		gtk_toolbar_new (GTK_ORIENTATION_VERTICAL, GTK_TOOLBAR_ICONS);
	gtk_widget_show (toolbar1);
	gtk_container_add (GTK_CONTAINER (dock_item1), toolbar1);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar1),
				       GTK_RELIEF_NONE);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar1),
				     GTK_TOOLBAR_SPACE_LINE);

	frame3 = gtk_frame_new (NULL);
	gtk_widget_show (frame3);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1),
				   frame3, _("Build messages"), NULL);
	gtk_widget_set_usize (frame3, 80, -1);

	button5 = gtk_toggle_button_new ();
	gtk_widget_show (button5);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button5), TRUE);
	gtk_button_set_relief (GTK_BUTTON (button5), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (frame3), button5);
	pix_lab =
		create_xpm_label_box (mesg_gui, ANJUTA_PIXMAP_MINI_BUILD, FALSE,
				      _("Build"));
	gtk_widget_show (pix_lab);
	gtk_container_add (GTK_CONTAINER (button5), pix_lab);

	frame3 = gtk_frame_new (NULL);
	gtk_widget_show (frame3);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1),
				   frame3, _("Debug messages"), NULL);
	gtk_widget_set_usize (frame3, 80, -1);

	button6 = gtk_toggle_button_new ();
	gtk_widget_show (button6);
	gtk_button_set_relief (GTK_BUTTON (button6), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (frame3), button6);
	pix_lab =
		create_xpm_label_box (mesg_gui, ANJUTA_PIXMAP_MINI_DEBUG, FALSE,
				      _("Debug"));
	gtk_widget_show (pix_lab);
	gtk_container_add (GTK_CONTAINER (button6), pix_lab);

	frame3 = gtk_frame_new (NULL);
	gtk_widget_show (frame3);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1),
				   frame3, _("Find messages"), NULL);
	gtk_widget_set_usize (frame3, 80, -1);

	button7 = gtk_toggle_button_new ();
	gtk_widget_show (button7);
	gtk_button_set_relief (GTK_BUTTON (button7), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (frame3), button7);
	pix_lab =
		create_xpm_label_box (mesg_gui, ANJUTA_PIXMAP_MINI_FIND, FALSE,
				      _("Find"));
	gtk_widget_show (pix_lab);
	gtk_container_add (GTK_CONTAINER (button7), pix_lab);

	frame3 = gtk_frame_new (NULL);
/* gtk_widget_show (frame3); *//* Not yet implemented */
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1),
				   frame3, _("CVS messages"), NULL);
	gtk_widget_set_usize (frame3, 80, -1);

	button8 = gtk_toggle_button_new ();
	gtk_widget_show (button8);
	gtk_button_set_relief (GTK_BUTTON (button8), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (frame3), button8);
	pix_lab =
		create_xpm_label_box (mesg_gui, ANJUTA_PIXMAP_MINI_CVS, FALSE,
				      _("CVS"));
	gtk_widget_show (pix_lab);
	gtk_container_add (GTK_CONTAINER (button8), pix_lab);

	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1),
				   frame2, _("Dock window"), NULL);
	gtk_widget_set_usize (frame2, 80, -1);

	button1 = gtk_button_new ();
	gtk_widget_show (button1);
	gtk_button_set_relief (GTK_BUTTON (button1), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (frame2), button1);
	pix_lab =
		create_xpm_label_box (mesg_gui, ANJUTA_PIXMAP_MINI_DOCK, FALSE,
				      _("Dock"));
	gtk_widget_show (pix_lab);
	gtk_container_add (GTK_CONTAINER (button1), pix_lab);

	hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox3);
	gnome_dock_set_client_area (GNOME_DOCK (dock1), hbox3);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (hbox3), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	mesg_clist1 = gtk_clist_new (1);
	gtk_widget_show (mesg_clist1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), mesg_clist1);
	gtk_clist_column_titles_hide (GTK_CLIST (mesg_clist1));
	gtk_clist_set_column_auto_resize (GTK_CLIST (mesg_clist1), 0, TRUE);

	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox3), scrolledwindow2, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	mesg_clist2 = gtk_clist_new (1);
	gtk_widget_show (mesg_clist2);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), mesg_clist2);
	gtk_clist_column_titles_hide (GTK_CLIST (mesg_clist2));
	gtk_clist_set_column_auto_resize (GTK_CLIST (mesg_clist2), 0, TRUE);

	scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox3), scrolledwindow3, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	mesg_clist3 = gtk_clist_new (1);
	gtk_widget_show (mesg_clist3);
	gtk_container_add (GTK_CONTAINER (scrolledwindow3), mesg_clist3);
	gtk_clist_column_titles_hide (GTK_CLIST (mesg_clist3));
	gtk_clist_set_column_auto_resize (GTK_CLIST (mesg_clist3), 0, TRUE);

	scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox3), scrolledwindow4, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	mesg_clist4 = gtk_clist_new (1);
	gtk_widget_show (mesg_clist4);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), mesg_clist4);
	gtk_clist_column_titles_hide (GTK_CLIST (mesg_clist4));
	gtk_clist_set_column_auto_resize (GTK_CLIST (mesg_clist4), 0, TRUE);

	gtk_signal_connect (GTK_OBJECT (mesg_clist1), "select_row",
			    GTK_SIGNAL_FUNC (on_mesg_clist_select_row), m);
	gtk_signal_connect (GTK_OBJECT (mesg_clist2), "select_row",
			    GTK_SIGNAL_FUNC (on_mesg_clist_select_row), m);
	gtk_signal_connect (GTK_OBJECT (mesg_clist3), "select_row",
			    GTK_SIGNAL_FUNC (on_mesg_clist_select_row), m);
	gtk_signal_connect (GTK_OBJECT (mesg_clist4), "select_row",
			    GTK_SIGNAL_FUNC (on_mesg_clist_select_row), m);

	gtk_signal_connect (GTK_OBJECT (mesg_gui), "delete_event",
			    GTK_SIGNAL_FUNC (on_mesg_win_delete_event), m);

	gtk_signal_connect (GTK_OBJECT (toolbar1), "orientation_changed",
			    GTK_SIGNAL_FUNC (on_mesg_win_orien_changed), m);

	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_dock_clicked), m);

	gtk_signal_connect (GTK_OBJECT (button5), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but1_clicked), m);
	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but2_clicked), m);
	gtk_signal_connect (GTK_OBJECT (button7), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but3_clicked), m);
	gtk_signal_connect (GTK_OBJECT (button8), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but4_clicked), m);

	gtk_signal_connect (GTK_OBJECT (button5), "event",
			    GTK_SIGNAL_FUNC (on_mesg_win_but_event), m);
	gtk_signal_connect (GTK_OBJECT (button6), "event",
			    GTK_SIGNAL_FUNC (on_mesg_win_but_event), m);
	gtk_signal_connect (GTK_OBJECT (button7), "event",
			    GTK_SIGNAL_FUNC (on_mesg_win_but_event), m);
	gtk_signal_connect (GTK_OBJECT (button8), "event",
			    GTK_SIGNAL_FUNC (on_mesg_win_but_event), m);


	m->GUI = mesg_gui;
	m->client_area = eventbox1;
	m->client = dock1;
	m->extra_toolbar = frame2;
	gtk_widget_ref (m->GUI);
	gtk_widget_ref (m->client_area);
	gtk_widget_ref (m->client);
	gtk_widget_ref (m->extra_toolbar);

	m->scrolledwindow[0] = scrolledwindow1;
	m->scrolledwindow[1] = scrolledwindow2;
	m->scrolledwindow[2] = scrolledwindow3;
	m->scrolledwindow[3] = scrolledwindow4;
	for (i = 0; i < MESSAGE_TYPE_END; i++)
		gtk_widget_ref (GTK_WIDGET (m->scrolledwindow[i]));

	m->clist[0] = GTK_CLIST (mesg_clist1);
	m->clist[1] = GTK_CLIST (mesg_clist2);
	m->clist[2] = GTK_CLIST (mesg_clist3);
	m->clist[3] = GTK_CLIST (mesg_clist4);
	for (i = 0; i < MESSAGE_TYPE_END; i++)
		gtk_widget_ref (GTK_WIDGET (m->clist[i]));

	m->but[0] = GTK_TOGGLE_BUTTON (button5);
	m->but[1] = GTK_TOGGLE_BUTTON (button6);
	m->but[2] = GTK_TOGGLE_BUTTON (button7);
	m->but[3] = GTK_TOGGLE_BUTTON (button8);
	for (i = 0; i < MESSAGE_TYPE_END; i++)
		gtk_widget_ref (GTK_WIDGET (m->but[i]));
}

void
messages_next_message (Messages*m)
{
	GList *node;
	gchar *cur_file;
	gint cur_line;
	gboolean cur_parsable, first;

	g_return_if_fail (m != NULL);

	first = FALSE;
	if (m->current_pos[m->cur_type] <0)
	{
		m->current_pos[m->cur_type] = 0;
		first = TRUE;
	}
	node = g_list_nth (m->data[m->cur_type], m->current_pos[m->cur_type]);

	if( !node)
	{
		gdk_beep();
		return;
	}
	cur_parsable = parse_error_line (node->data, &cur_file, &cur_line);
	/* Skip the current line */
	node = g_list_next (node);
	while (node)
	{
		if(node->data)
		{
			gchar* fn;
			gint ln;
			
			if (parse_error_line (node->data, &fn, &ln))
			{
				gboolean same_pos;
				
				same_pos = FALSE;
				if(cur_parsable)
				{
					if (cur_line == ln && strcmp (cur_file, fn)==0)
						same_pos = TRUE;
				}
				if (same_pos == FALSE || first == TRUE)
				{
					GtkAdjustment* adj;
					gfloat value;
					gint pos;
					
					pos = g_list_position (m->data[m->cur_type], node);
					gtk_clist_select_row (GTK_CLIST (m->clist[m->cur_type]), pos, 0);
					anjuta_goto_file_line (fn, ln);
					g_free (fn);
					if (cur_parsable) g_free (cur_file);
						
					adj = gtk_clist_get_vadjustment (m->clist[m->cur_type]);
					value = (adj->upper*pos)/g_list_length(m->data[m->cur_type]) - adj->page_increment/2;
					value = MAX (0, value);
					value = MIN (value, adj->upper - adj->page_increment);
					gtk_adjustment_set_value (adj, value);
					return;
				}
			}
		}
		node = g_list_next (node);
	}
	gdk_beep();
	if (cur_parsable) g_free (cur_file);
}

void
messages_previous_message (Messages*m)
{
	GList *node;
	gchar* cur_file;
	gint cur_line;
	gboolean cur_parsable, first;

	g_return_if_fail (m != NULL);

	first = FALSE;
	if (m->current_pos[m->cur_type] <0)
	{
		m->current_pos[m->cur_type] = 0;
		first = TRUE;
	}
	node = g_list_nth (m->data[m->cur_type], m->current_pos[m->cur_type]);

	if( !node)
	{
		gdk_beep();
		return;
	}
	cur_parsable = parse_error_line (node->data, &cur_file, &cur_line);
	/* Skip the current line */
	node = g_list_previous (node);
	while (node)
	{
		if (node->data)
		{
			gchar* fn;
			gint ln;
			
			if (parse_error_line (node->data, &fn, &ln))
			{
				gboolean same_pos;
				
				same_pos = FALSE;
				if(cur_parsable)
				{
					if (cur_line == ln && strcmp (cur_file, fn)==0)
						same_pos = TRUE;
				}
				if (same_pos == FALSE || first == TRUE)
				{
					GtkAdjustment* adj;
					gfloat value;
					gint pos;
					
					pos = g_list_position (m->data[m->cur_type], node);
					gtk_clist_select_row (GTK_CLIST (m->clist[m->cur_type]), pos, 0);
					anjuta_goto_file_line (fn, ln);
					g_free (fn);
					if (cur_parsable) g_free (cur_file);

					adj = gtk_clist_get_vadjustment (m->clist[m->cur_type]);
					value = (adj->upper*pos)/g_list_length(m->data[m->cur_type]) - adj->page_increment/2;
					value = (value > 0) ? value : 0;
					gtk_adjustment_set_value (adj, value);
					
					return;
				}
			}
		}
		node = g_list_previous(node);
	}
	gdk_beep();
	if (cur_parsable) g_free (cur_file);
}

void
on_mesg_clist_select_row (GtkCList * clist,
			  gint row,
			  gint column, GdkEvent * event, gpointer user_data)
{
	gchar *s, *fn;
	gint ln;
	Messages *m = user_data;


	m->current_pos[m->cur_type] = row;

	if (event == NULL)
		return;
	if (event->type != GDK_2BUTTON_PRESS)
		return;
	if (((GdkEventButton *) event)->button != 1)
		return;
	s = g_list_nth_data (m->data[m->cur_type], row);
	if (parse_error_line (s, &fn, &ln))
	{
		anjuta_goto_file_line (fn, ln);
		g_free (fn);
	}
}

gint
on_mesg_win_delete_event (GtkWidget * w, GdkEvent * event, gpointer data)
{
	Messages *m = data;
	messages_hide (m);
	return TRUE;
}

void
on_mesg_win_dock_clicked (GtkButton * button, gpointer data)
{
	messages_dock (app->messages);
}

void
messages_dock (Messages * m)
{
	if (m)
	{
		if (m->is_docked)
			return;
		if (m->is_showing)
		{
			messages_hide (m);
			m->is_docked = TRUE;
			messages_show (m, MESSAGE_TYPE_END);
			return;
		}
		else
		{
			m->is_docked = TRUE;
			return;
		}
	}
}
void
messages_undock (Messages * m)
{
	if (m)
	{
		if (!m->is_docked)
			return;
		if (m->is_showing)
		{
			messages_hide (m);
			m->is_docked = FALSE;
			messages_show (m, MESSAGE_TYPE_END);
			return;
		}
		else
		{
			m->is_docked = FALSE;
			return;
		}
	}
}

/*******************************
 * Private functions: Do not use  *
 *******************************/
void
messages_detach (Messages * m)
{
	gtk_container_remove (GTK_CONTAINER (app->widgets.client_area),
			      app->widgets.the_client);
	gtk_container_remove (GTK_CONTAINER (app->widgets.vpaned),
			      app->widgets.hpaned_client);
	gtk_container_add (GTK_CONTAINER (app->widgets.client_area),
			   app->widgets.hpaned_client);
	app->widgets.the_client = app->widgets.hpaned_client;
	gtk_container_remove (GTK_CONTAINER (app->widgets.mesg_win_container),
			      app->messages->client);
	gtk_container_add (GTK_CONTAINER (app->messages->client_area),
			   app->messages->client);
	gtk_widget_show (app->messages->client);
	gtk_widget_show (m->extra_toolbar);
}

void
messages_attach (Messages * m)
{
	messages_hide (app->messages);
	gtk_container_remove (GTK_CONTAINER (app->messages->client_area),
			      app->messages->client);
	gtk_container_add (GTK_CONTAINER (app->widgets.mesg_win_container),
			   app->messages->client);
	gtk_widget_show (app->messages->client);
	m->is_docked = TRUE;

	gtk_container_remove (GTK_CONTAINER (app->widgets.client_area),
			      app->widgets.hpaned_client);
	gtk_container_add (GTK_CONTAINER (app->widgets.vpaned),
			   app->widgets.hpaned_client);
	gtk_container_add (GTK_CONTAINER (app->widgets.client_area),
			   app->widgets.vpaned);
	app->widgets.the_client = app->widgets.vpaned;
	gtk_widget_hide (m->extra_toolbar);
}

static void
on_mesg_win_but1_clicked (GtkButton * but, gpointer data)
{
	Messages *m = data;
	gint type = MESSAGE_BUILD;
	gint i;

	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[1]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but2_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[2]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but3_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[3]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but4_clicked), m);

	for (i = 0; i < MESSAGE_TYPE_END; i++)
	{
		if (i != type)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (m->but[i]), FALSE);

		if (i == type)
			gtk_widget_show (m->scrolledwindow[i]);
		else
			gtk_widget_hide (m->scrolledwindow[i]);
		m->cur_type = MESSAGE_BUILD;
	}
	gtk_signal_connect (GTK_OBJECT (m->but[1]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but2_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[2]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but3_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[3]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but4_clicked), m);
}

static void
on_mesg_win_but2_clicked (GtkButton * but, gpointer data)
{
	Messages *m = data;
	gint type = MESSAGE_DEBUG;
	gint i;

	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[0]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but1_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[2]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but3_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[3]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but4_clicked), m);

	for (i = 0; i < MESSAGE_TYPE_END; i++)
	{
		if (i != type)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (m->but[i]), FALSE);

		if (i == type)
			gtk_widget_show (m->scrolledwindow[i]);
		else
			gtk_widget_hide (m->scrolledwindow[i]);
		m->cur_type = MESSAGE_DEBUG;
	}
	gtk_signal_connect (GTK_OBJECT (m->but[0]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but1_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[2]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but3_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[3]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but4_clicked), m);
}

static void
on_mesg_win_but3_clicked (GtkButton * but, gpointer data)
{
	Messages *m = data;
	gint type = MESSAGE_FIND;
	gint i;

	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[0]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but1_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[1]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but2_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[3]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but4_clicked), m);

	for (i = 0; i < MESSAGE_TYPE_END; i++)
	{
		if (i != type)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (m->but[i]), FALSE);

		if (i == type)
			gtk_widget_show (m->scrolledwindow[i]);
		else
			gtk_widget_hide (m->scrolledwindow[i]);
		m->cur_type = MESSAGE_FIND;
	}
	gtk_signal_connect (GTK_OBJECT (m->but[0]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but1_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[1]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but2_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[3]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but4_clicked), m);
}

static void
on_mesg_win_but4_clicked (GtkButton * but, gpointer data)
{
	Messages *m = data;
	gint type = MESSAGE_CVS;
	gint i;

	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[0]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but1_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[1]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but2_clicked), m);
	gtk_signal_disconnect_by_func (GTK_OBJECT (m->but[2]),
				       GTK_SIGNAL_FUNC
				       (on_mesg_win_but3_clicked), m);

	for (i = 0; i < MESSAGE_TYPE_END; i++)
	{
		if (i != type)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (m->but[i]), FALSE);

		if (i == type)
			gtk_widget_show (m->scrolledwindow[i]);
		else
			gtk_widget_hide (m->scrolledwindow[i]);
		m->cur_type = MESSAGE_CVS;
	}
	gtk_signal_connect (GTK_OBJECT (m->but[0]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but1_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[1]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but2_clicked), m);
	gtk_signal_connect (GTK_OBJECT (m->but[2]), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_but3_clicked), m);
}

gboolean
on_mesg_win_but_event (GtkWidget * widget,
		       GdkEvent * event, gpointer user_data)
{
	/* Following equalities are correct. Do not get confused by the
	 * other button event handlers */
	if (event->type == GDK_BUTTON_PRESS)
		return FALSE;
	if (((GdkEventButton *) event)->button == 1)
		return FALSE;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) == TRUE)
		return TRUE;
	else
		return FALSE;
}

static void
on_mesg_win_orien_changed (GtkToolbar * t, GtkOrientation or, gpointer data)
{
}
