/*
    anjuta_info.h
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include "anjuta.h"
#include "resources.h"
#include "utilities.h"
#include "anjuta_info.h"

static GtkWidget *
create_anjuta_info_dialog_with_less (gint height, gint width)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *less1;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;

  dialog1 = gnome_dialog_new (_("Information"), NULL);
  gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window));
  gtk_widget_set_usize (dialog1, 400, 250);
  gnome_dialog_set_close (GNOME_DIALOG (dialog1), TRUE);
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, TRUE, FALSE);
  if (height < 250)
    height = 250;
  if (width < 400)
    width = 400;
  gtk_window_set_default_size (GTK_WINDOW (dialog1), width, height);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "infoless", "Anjuta");
  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  less1 = gnome_less_new ();
  gtk_widget_show (less1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), less1, TRUE, TRUE, 0);
  gnome_less_set_fixed_font  (GNOME_LESS(less1), TRUE);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
  button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
  gnome_dialog_set_default (GNOME_DIALOG(dialog1), 0);
  gnome_dialog_grab_focus (GNOME_DIALOG(dialog1), 0);

  gtk_widget_ref (less1);
  gtk_widget_show (dialog1);

  return less1;
}

static GtkWidget *
create_anjuta_info_dialog_with_clist (gint height, gint width)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *clist1;
  GtkWidget *dialog_action_area1;
  GtkWidget *scrolledwindow1;
  GtkWidget *button1;
  GdkFont   *font;

  dialog1 = gnome_dialog_new (_("Information"), NULL);
  gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window));
  gtk_widget_set_usize (dialog1, 400, 250);
  gnome_dialog_set_close (GNOME_DIALOG (dialog1), TRUE);
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, TRUE, FALSE);
  if (height < 250)
    height = 250;
  if (width < 400)
    width = 400;
  gtk_window_set_default_size (GTK_WINDOW (dialog1), width, height);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "infoclist", "Anjuta");
  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist1 = gtk_clist_new (1);
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
  gtk_clist_set_column_auto_resize (GTK_CLIST (clist1), 0, TRUE);
  font = get_fixed_font ();
  if (font)
  {
    GtkStyle* style;
    style  = gtk_style_copy(gtk_widget_get_style(clist1));
    if(style->font) gdk_font_unref(style->font);
    style->font = font;
    gtk_widget_set_style(clist1, style);
  }
  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
  button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gnome_dialog_set_default (GNOME_DIALOG(dialog1), 0);
  gnome_dialog_grab_focus (GNOME_DIALOG(dialog1), 0);

  gtk_widget_ref (clist1);
  gtk_widget_show (dialog1);

  return clist1;
}

gboolean
anjuta_info_show_file (const gchar * path, gint height, gint width)
{
  gboolean ret;
  GtkWidget *less = create_anjuta_info_dialog_with_less (height, width);
  ret = gnome_less_show_file (GNOME_LESS (less), path);
  gtk_widget_unref (less);
  return ret;
}

gboolean
anjuta_info_show_command (const gchar * command_line, gint height, gint width)
{
  gboolean ret;
  GtkWidget *less = create_anjuta_info_dialog_with_less (height, width);
  ret = gnome_less_show_command (GNOME_LESS (less), command_line);
  gtk_widget_unref (less);
  return ret;
}

gboolean
anjuta_info_show_string (const gchar * s, gint height, gint width)
{
  GtkWidget *less = create_anjuta_info_dialog_with_less (height, width);
  gnome_less_show_string (GNOME_LESS (less), s);
  gtk_widget_unref (less);
  return TRUE;
}

gboolean
anjuta_info_show_filestream (FILE * f, gint height, gint width)
{
  gboolean ret;
  GtkWidget *less = create_anjuta_info_dialog_with_less (height, width);
  ret = gnome_less_show_filestream (GNOME_LESS (less), f);
  gtk_widget_unref (less);
  return ret;
}

gboolean
anjuta_info_show_fd (int file_descriptor, gint height, gint width)
{
  gboolean ret;
  GtkWidget *less = create_anjuta_info_dialog_with_less (height, width);
  ret = gnome_less_show_fd (GNOME_LESS (less), file_descriptor);
  gtk_widget_unref (less);
  return ret;
}

void
anjuta_info_show_list (GList* list, gint height, gint width)
{
  GtkWidget *clist = create_anjuta_info_dialog_with_clist (height, width);
  gtk_clist_freeze(GTK_CLIST(clist));
  while(list)
  {
     gchar *temp;
     temp = remove_white_spaces(list->data);
     gtk_clist_append(GTK_CLIST(clist), &temp);
     g_free(temp);
     list = g_list_next(list);
  }
  gtk_clist_thaw(GTK_CLIST(clist));
  gtk_widget_unref (clist);
}
