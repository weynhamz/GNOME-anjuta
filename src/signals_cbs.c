/*
    signals_cbs.c
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

#include <gnome.h>
#include "signals.h"
#include "signals_cbs.h"
#include "debugger.h"

void
on_signals_clist_select_row          (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    Signals *s;
    s = debugger.signals;
    s->index = row;
}

gint
on_signals_delete_event(GtkWidget* w, GdkEvent *event, gpointer data)
{
  Signals* cr = data;
  signals_hide(cr);
  return TRUE;
}

void
on_signals_modify_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget* dialog;
  Signals *s = debugger.signals;
  dialog = create_signals_set_dialog(s);
  if(dialog) gtk_widget_show(dialog);
}

void
on_signals_send_activate            (GtkMenuItem     *menuitem,
                                     gpointer         user_data)
{
  Signals *s = debugger.signals;
  gchar* msg;
  gchar* sig;

  if (debugger_is_active() == FALSE) return;
  if (debugger.prog_is_running == FALSE) return;
  if (debugger.child_pid < 1) return;

  signals_show(s);
  gtk_clist_get_text(GTK_CLIST (s->widgets.clist), s->index, 0, &sig);
  debugger_signal(sig, TRUE);
}

void
on_signals_update_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
 debugger_put_cmd_in_queqe("info signals", DB_CMD_NONE, signals_update, debugger.signals);
 debugger_execute_cmd_in_queqe();
}

void
on_signals_help_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

gboolean
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

void
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


void
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


void
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


void
on_signals_set_help_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  on_signals_help_activate(NULL , user_data);
}


void
on_signals_set_ok_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  gchar* cmd, *tmp;
  Signals *s = debugger.signals;

  cmd = g_strconcat("handle ", s->signal, " ",  NULL);
  tmp = cmd;
  if(s->stop)
  {
      gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->index, 1, "Yes");
      cmd = g_strconcat(tmp, "stop ", NULL);
  }
  else
  {
      gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->index, 1, "No");
      cmd = g_strconcat(tmp, "nostop ", NULL);
  }
  g_free(tmp);

  tmp = cmd;
  if(s->print)
  {
      gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->index, 2, "Yes");
      cmd = g_strconcat(tmp, "print ", NULL);
  }
  else
  {
      gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->index, 2, "No");
      cmd = g_strconcat(tmp, "noprint ", NULL);
   }
  g_free(tmp);

  tmp = cmd;
  if(s->pass)
  {
      gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->index, 3, "Yes");
      cmd = g_strconcat(tmp, "pass ", NULL);
  }
  else
  {
      gtk_clist_set_text(GTK_CLIST(s->widgets.clist), s->index, 3, "No");
      cmd = g_strconcat(tmp, "nopass ", NULL);
  }
  g_free(tmp);
  debugger_put_cmd_in_queqe(cmd, DB_CMD_NONE, NULL, NULL);
  debugger_execute_cmd_in_queqe();
  g_free(cmd);
}
