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
#include <libgnomeui/gnome-window-icon.h>

#include "resources.h"
#include "debugger.h"
#include "utilities.h"
#include "anjuta.h"

#define GLADE_FILE_SIGNALS PACKAGE_DATA_DIR"/glade/anjuta.glade"

enum {
	COLUMN_SIGNAL,
	COLUMN_STOP,
	COLUMN_PRINT,
	COLUMN_PASS,
	COLUMN_DESCRIPTION,
	COLUMNS_NB
};

static const gchar *column_names[COLUMNS_NB] = {
	N_("Signal"), N_("Stop"), N_("Print"), N_("Pass"), N_("Description")
};

static void create_signals_gui(Signals *cr);
static GtkWidget* create_signals_set_dialog (Signals *s);
static GtkWidget* create_signals_menu (void);

Signals *
signals_new ()
{
  Signals *ew;
  ew = g_malloc (sizeof (Signals));
  if (ew)
  {
    ew->is_showing = FALSE;
    ew->win_width = 460;
    ew->win_height = 320;
    ew->win_pos_x = 150;
    ew->win_pos_y = 130;
    create_signals_gui (ew);
  }
  return ew;
}

void
signals_clear (Signals * sg)
{
	gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (sg->widgets.view))));
}

void
signals_update (GList * lines, gpointer data)
{
  Signals *sg;
  gint j, count;
  gchar *str;
  gchar sig[32], stop[10], print[10], pass[10];
  GList *list, *node;
  GtkListStore *store;

  sg = (Signals *) data;
  signals_clear (sg);
  list = remove_blank_lines (lines);
  if (g_list_length (list) < 2)
  {
    g_list_free (list);
    return;
  }
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (sg->widgets.view)));
  node = list->next;
  while (node)
  {
	GtkTreeIter iter;
	  
	count =
	  sscanf ((char *) node->data, "%s %s %s %s", sig, stop, print, pass);
	str = node->data;
	node = g_list_next (node);
	if (count != 4)
	  continue;
	
	/* Do not worry. This is used to avoid the last line */
	if (node == NULL)
	  break;
	for (j = 0; j < 4; j++)
	{
	    while (isspace (*str))
	      str++;
	    while (!isspace (*str))
	      str++;
	}
	while (isspace (*str))
	  str++;
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, COLUMN_SIGNAL, sig,
			    COLUMN_STOP, stop, COLUMN_PRINT, print,
			    COLUMN_PASS, pass, COLUMN_DESCRIPTION, str, -1);
  }
  g_list_free (list);
}

void
signals_show (Signals * ew)
{
  if (ew)
  {
    if (ew->is_showing)
    {
		gdk_window_raise (ew->widgets.window->window);
		return;
    }
    gtk_window_move (GTK_WINDOW (ew->widgets.window), ew->win_pos_x,
					 ew->win_pos_y);
    gtk_window_set_default_size (GTK_WINDOW (ew->widgets.window),
				   ew->win_width, ew->win_height);
    gtk_widget_show (ew->widgets.window);
    ew->is_showing = TRUE;
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
    gdk_drawable_get_size (GDK_DRAWABLE (ew->widgets.window->window), &ew->win_width,
	   &ew->win_height);
    gtk_widget_hide (ew->widgets.window);
    ew->is_showing = FALSE;
  }
}

gboolean
signals_save_yourself (Signals * ew, FILE * stream)
{
	if (!ew) return FALSE;

	if (ew->is_showing)
	{
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
		      &ew->win_pos_y);
		gdk_drawable_get_size (GDK_DRAWABLE (ew->widgets.window->window),
							   &ew->win_width, &ew->win_height);
	}
	fprintf(stream, "signals.win.pos.x=%d\n", ew->win_pos_x);
	fprintf(stream, "signals.win.pos.y=%d\n", ew->win_pos_y);
	fprintf(stream, "signals.win.width=%d\n", ew->win_width);
	fprintf(stream, "signals.win.height=%d\n", ew->win_height);
	return TRUE;
}

gboolean
signals_load_yourself (Signals * ew, PropsID props)
{
	if (!ew) return FALSE;
	
	ew->win_pos_x = prop_get_int (props, "signals.win.pos.x", 150);
	ew->win_pos_y = prop_get_int (props, "signals.win.pos.y", 130);
	ew->win_width = prop_get_int (props, "signals.win.width", 460);
	ew->win_height = prop_get_int (props, "signals.win.height", 320);
	return TRUE;
}

void
signals_destroy (Signals * sg)
{
  if (sg)
  {
    signals_clear (sg);
    g_object_unref (G_OBJECT (sg->widgets.window));
    g_object_unref (G_OBJECT (sg->widgets.view));
    g_object_unref (G_OBJECT (sg->widgets.menu));
    g_object_unref (G_OBJECT (sg->widgets.menu_modify));
    g_object_unref (G_OBJECT (sg->widgets.menu_signal));
    g_object_unref (G_OBJECT (sg->widgets.menu_update));
    if (GTK_IS_WIDGET (sg->widgets.window))
      gtk_widget_destroy (sg->widgets.window);
    g_free (sg);
  }
}

void
signals_update_controls (Signals * ew)
{
  gboolean A, R, Pr;

  A = debugger_is_active ();
  R = debugger_is_ready ();
  Pr = debugger.prog_is_running;
  gtk_widget_set_sensitive (ew->widgets.menu_signal, A && Pr);
  gtk_widget_set_sensitive (ew->widgets.menu_modify, A && R);
  gtk_widget_set_sensitive (ew->widgets.menu_update, A && R);
}


static gint
on_signals_delete_event(GtkWidget* w, GdkEvent *event, gpointer data)
{
  Signals* cr = data;
  signals_hide(cr);
  return TRUE;
}

static void
on_signals_close (GtkWidget* w, gpointer data)
{
  Signals* cr = data;
  signals_hide(cr);
}

static void
on_signals_response (GtkWidget* w, gint res, gpointer data)
{
  Signals* cr = data;
  signals_hide(cr);
}

static void
on_signals_modify_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget* dialog;
  Signals *s = debugger.signals;
  dialog = create_signals_set_dialog(s);
  if(dialog) gtk_widget_show(dialog);
}

static void
on_signals_send_activate            (GtkMenuItem     *menuitem,
                                     gpointer         user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  gchar* sig;
  Signals *s = debugger.signals;

  if (debugger_is_active() == FALSE) return;
  if (debugger.prog_is_running == FALSE) return;
  if (debugger.child_pid < 1) return;

  signals_show(s);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (s->widgets.view));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
  	gtk_tree_model_get (model, &iter, COLUMN_SIGNAL, &sig, -1);
  	debugger_signal(sig, TRUE);
  }
}

static void
on_signals_update_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
 debugger_put_cmd_in_queqe("info signals", DB_CMD_NONE, signals_update, debugger.signals);
 debugger_execute_cmd_in_queqe();
}

static void
on_signals_help_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

static gboolean
on_signals_event                (GtkWidget       *widget,
                                 GdkEvent  *event,
                                 gpointer         user_data)
{
  GdkEventButton *bevent;
  Signals* ew = debugger.signals;
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
on_signals_togglebutton1_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
   Signals *sig=user_data;
   sig->stop = gtk_toggle_button_get_active(togglebutton);
   if(sig->stop)
       gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton)->child), _("Yes"));
    else
       gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton)->child), _("No"));
}


static void
on_signals_togglebutton2_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
   Signals *sig=user_data;
   sig->print = gtk_toggle_button_get_active(togglebutton);
   if(sig->print)
       gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton)->child), _("Yes"));
    else
       gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton)->child), _("No"));
}

static void
on_signals_togglebutton3_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
   Signals *sig=user_data;
   sig->pass = gtk_toggle_button_get_active(togglebutton);
   if(sig->pass)
       gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton)->child), _("Yes"));
    else
       gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton)->child), _("No"));
}

static void
on_signals_set_response              (GtkDialog *dlg, gint res,
                                      gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gchar* cmd, *tmp;
	const gchar *stop, *print, *pass;
	
	Signals *s = debugger.signals;
	if (res == GTK_RESPONSE_OK)
	{
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (s->widgets.view));
		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			cmd = g_strconcat("handle ", s->signal, " ",  NULL);
			tmp = cmd;
			if(s->stop)
			{
				stop = "Yes";
				cmd = g_strconcat(tmp, "stop ", NULL);
			}
			else
			{
				stop = "No";
				cmd = g_strconcat(tmp, "nostop ", NULL);
			}
			g_free(tmp);
			
			tmp = cmd;
			if(s->print)
			{
				print = "Yes";
				cmd = g_strconcat(tmp, "print ", NULL);
			}
			else
			{
				print = "No";
				cmd = g_strconcat(tmp, "noprint ", NULL);
			}
			g_free(tmp);
			
			tmp = cmd;
			if(s->pass)
			{
				pass = "Yes";
				cmd = g_strconcat(tmp, "pass ", NULL);
			}
			else
			{
				pass = "No";
				cmd = g_strconcat(tmp, "nopass ", NULL);
			}
			g_free(tmp);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_STOP, stop,
								COLUMN_PRINT, print, COLUMN_PASS, pass, -1);
			debugger_put_cmd_in_queqe(cmd, DB_CMD_NONE, NULL, NULL);
			debugger_execute_cmd_in_queqe();
			g_free(cmd);
		}
	}
	gtk_widget_destroy (GTK_WIDGET (dlg));
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
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("Help"),
    NULL,
    on_signals_help_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

static GtkWidget*
create_signals_menu ()
{
  GtkWidget *signals_menu;

  signals_menu = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (signals_menu), signals_menu_uiinfo,
                       NULL, FALSE, 0);
  return signals_menu;
}

static void
create_signals_gui(Signals *cr)
{
	GladeXML *gxml;
	GtkWidget *topwindow;
	GtkTreeView *view;
	GtkListStore *store;
	guint i;

	gxml = glade_xml_new (GLADE_FILE_SIGNALS, "window.debugger.signals", NULL);
	topwindow = glade_xml_get_widget (gxml, "window.debugger.signals");
	view = GTK_TREE_VIEW (glade_xml_get_widget (gxml, "debugger.signals.tv"));
	g_object_unref (gxml);

	// top level window
	gtk_window_set_transient_for(GTK_WINDOW (topwindow),
				GTK_WINDOW (app->widgets.window));
	gtk_window_set_title (GTK_WINDOW (topwindow), _("Kernel signals"));
	gtk_window_set_role (GTK_WINDOW (topwindow), "KernelSignals");
	gnome_window_icon_set_from_default (GTK_WINDOW (topwindow));

	// treeview
	store = gtk_list_store_new (COLUMNS_NB,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
								 GTK_SELECTION_BROWSE);
	gtk_tree_view_set_search_column (view, COLUMN_SIGNAL);
	gtk_tree_view_set_headers_clickable (view, FALSE);

	for (i = 0; i < COLUMNS_NB; i++)
	{
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (column_names[i],
					renderer, "text", i, NULL);
		// gtk_tree_view_column_set_sort_column_id (column, i);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);
	}
	g_object_unref (G_OBJECT (store));

	g_signal_connect (G_OBJECT (topwindow), "delete_event",
					  G_CALLBACK (on_signals_delete_event), cr);
	g_signal_connect (G_OBJECT (topwindow), "close",
					  G_CALLBACK (on_signals_close), cr);
	g_signal_connect (G_OBJECT (topwindow), "response",
					  G_CALLBACK (on_signals_response), cr);
	g_signal_connect (G_OBJECT (view), "event",
					  G_CALLBACK (on_signals_event), cr);

	cr->widgets.window = topwindow;
	cr->widgets.view = GTK_WIDGET (view);
	cr->widgets.menu = create_signals_menu();
	cr->widgets.menu_modify = signals_menu_uiinfo[0].widget;
	cr->widgets.menu_signal = signals_menu_uiinfo[1].widget;
	cr->widgets.menu_update = signals_menu_uiinfo[2].widget;
	
	gtk_widget_ref(cr->widgets.window);
	gtk_widget_ref(cr->widgets.view);
	gtk_widget_ref(cr->widgets.menu);
	gtk_widget_ref(cr->widgets.menu_modify);
	gtk_widget_ref(cr->widgets.menu_signal);
	gtk_widget_ref(cr->widgets.menu_update);
}

static GtkWidget*
create_signals_set_dialog (Signals *s)
{
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *togglebutton1;
	GtkWidget *togglebutton2;
	GtkWidget *togglebutton3;
	gchar *sig, *desc, *stop, *print, *pass;

	GladeXML *gxml;
	GtkWidget *topwindow;

	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	/* Get current selection */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (s->widgets.view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return NULL;
	gtk_tree_model_get (model, &iter, COLUMN_SIGNAL, &sig,
						COLUMN_STOP, &stop,
						COLUMN_PRINT, &print,
						COLUMN_PASS, &pass,
						COLUMN_DESCRIPTION, &desc, -1);
	
	gxml = glade_xml_new (GLADE_FILE_SIGNALS, "window.debugger.signals.set", NULL);
	topwindow = glade_xml_get_widget (gxml, "window.debugger.signals.set");
	label1 = glade_xml_get_widget (gxml, "label.signal");
	label2 = glade_xml_get_widget (gxml, "label.description");
	togglebutton1 = glade_xml_get_widget (gxml, "toggle.stop");
	togglebutton2 = glade_xml_get_widget (gxml, "toggle.print");
	togglebutton3 = glade_xml_get_widget (gxml, "toggle.pass");
	g_object_unref (gxml);

	gtk_dialog_set_default_response (GTK_DIALOG (topwindow), GTK_RESPONSE_OK);
	s->signal = sig;

	if(strcasecmp(stop, "Yes")==0)
		s->stop = TRUE;
	else
		s->stop = FALSE;

	if(strcasecmp(print, "Yes")==0)
		s->print = TRUE;
	else
		s->print = FALSE;

	if(strcasecmp(pass, "Yes")==0)
		s->pass = TRUE;
	else
		s->pass = FALSE;

	gtk_label_set_text (GTK_LABEL (label1), sig);
	gtk_label_set_text (GTK_LABEL (label2), desc);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton1), s->stop);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton2), s->print);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton3), s->pass);
	gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton1)->child), stop);
	gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton2)->child), print);
	gtk_label_set_text(GTK_LABEL(GTK_BIN(togglebutton3)->child), pass);

	g_signal_connect (G_OBJECT (togglebutton1), "toggled",
                      G_CALLBACK (on_signals_togglebutton1_toggled),
                      s);
	g_signal_connect (G_OBJECT (togglebutton2), "toggled",
                      G_CALLBACK (on_signals_togglebutton2_toggled),
                      s);
	g_signal_connect (G_OBJECT (togglebutton3), "toggled",
                      G_CALLBACK (on_signals_togglebutton3_toggled),
                      s);
	g_signal_connect (G_OBJECT (topwindow), "response",
                      G_CALLBACK (on_signals_set_response),
                      s);
	return topwindow;
}
