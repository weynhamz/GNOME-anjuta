/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <assert.h>

#include <gnome.h>

#include "text_editor.h"
#include "resources.h"
#include "anjuta.h"
#include "goto_line.h"


/* Widget implementation headers */

enum
{
  LAST_SIGNAL
};

static void gotoline_class_init (GotoLineClass * klass);
static void gotoline_init (GotoLine * obj);

static gint gotoline_signals[LAST_SIGNAL];


/* Widget function headers */

/** Jumps Current text buffer to entered line number */
static void on_go_to_line_ok_clicked (GtkButton * button, gpointer user_data);


guint
gotoline_get_type ()
{
  static guint obj_type = 0;

  if (!obj_type)
    {
      GtkTypeInfo obj_info;

      obj_info.type_name = "GotoLine";
      obj_info.object_size = sizeof (GotoLine);
      obj_info.class_size = sizeof (GotoLineClass);
      obj_info.class_init_func = (GtkClassInitFunc) gotoline_class_init;
      obj_info.object_init_func = (GtkObjectInitFunc) gotoline_init;
      obj_info.reserved_1 = (GtkArgSetFunc) NULL;
      obj_info.reserved_2 = (GtkArgGetFunc) NULL;
      obj_info.base_class_init_func = (GtkClassInitFunc) NULL;

      obj_type = gtk_type_unique (gnome_dialog_get_type (), &obj_info);
    }

  return obj_type;
}


static void
gotoline_class_init (GotoLineClass * klass)
{
  GtkObjectClass *object_class;

  assert (klass != NULL);

  object_class = (GtkObjectClass *) klass;

  gtk_object_class_add_signals (object_class, gotoline_signals, LAST_SIGNAL);
}


static void
gotoline_init (GotoLine * obj)
{
  GtkWidget *dialog_vbox;
  GtkWidget *frame;
  GtkWidget *numberentry;
  GtkWidget *combo_entry;
  GtkWidget *button1;
  GtkWidget *button2;

  assert (obj != NULL);

  gtk_window_set_position (GTK_WINDOW (obj), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (obj), FALSE, FALSE, FALSE);
  gtk_window_set_title (GTK_WINDOW (obj), "Goto Line ...");
  gnome_dialog_set_close (GNOME_DIALOG (obj), TRUE);

  dialog_vbox = GNOME_DIALOG (obj)->vbox;
  gtk_widget_show (dialog_vbox);

  frame = gtk_frame_new (_(" Goto Line number "));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, FALSE, 0);

  numberentry = gnome_number_entry_new (NULL, NULL);
  gtk_widget_show (numberentry);
  gtk_container_add (GTK_CONTAINER (frame), numberentry);
  gtk_container_set_border_width (GTK_CONTAINER (numberentry), 10);

  combo_entry =
    gnome_number_entry_gtk_entry (GNOME_NUMBER_ENTRY (numberentry));
  gtk_widget_show (combo_entry);

  gnome_dialog_append_button (GNOME_DIALOG (obj), GNOME_STOCK_BUTTON_OK);
  button1 = g_list_last (GNOME_DIALOG (obj)->buttons)->data;
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (obj), GNOME_STOCK_BUTTON_CANCEL);
  button2 = g_list_last (GNOME_DIALOG (obj)->buttons)->data;
  gtk_widget_show (button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

  /** Default to the OK button */
  gnome_dialog_set_default (GNOME_DIALOG (obj), 0);
  gtk_widget_grab_focus (GTK_WIDGET(button1));

  /** If press enter in line number enter, act as if default button was pressed */
  gnome_dialog_editable_enters (GNOME_DIALOG (obj),
				GTK_EDITABLE (combo_entry));

  gtk_widget_grab_focus (combo_entry);

  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
		      GTK_SIGNAL_FUNC (on_go_to_line_ok_clicked),
		      numberentry);
}


GtkWidget *
gotoline_new ()
{
  return GTK_WIDGET (gtk_type_new (gotoline_get_type ()));
}


static void
on_go_to_line_ok_clicked (GtkButton * button, gpointer user_data)
{
  GnomeNumberEntry *ne;
  TextEditor *te;
  guint num;

  ne = (GnomeNumberEntry *) user_data;
  te = anjuta_get_current_text_editor ();
  num = (guint) gnome_number_entry_get_number (ne);

  if (te)
    text_editor_goto_line (te, num, TRUE);
}
