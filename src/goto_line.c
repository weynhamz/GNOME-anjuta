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
static void on_go_to_line_response (GtkDialog* dialog, gint response,
                                    gpointer user_data);

guint
gotoline_get_type ()
{
	static guint obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GotoLineClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gotoline_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GotoLineClass),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gotoline_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (GTK_TYPE_DIALOG,
		                                   "GotoLine", &obj_info, 0);
	}
	return obj_type;
}


static void
gotoline_class_init (GotoLineClass * klass)
{
  GObjectClass *object_class;
  assert (klass != NULL);
  object_class = (GObjectClass *) klass;
  //g_object_class_add_signals (object_class, gotoline_signals, LAST_SIGNAL);
}


static void
gotoline_init (GotoLine * obj)
{
  GtkWidget *dialog_vbox;
  GtkWidget *frame;
  GtkWidget *numberentry;
  GtkWidget *combo_entry;

  assert (obj != NULL);

  gtk_window_set_position (GTK_WINDOW (obj), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (obj), FALSE, FALSE, FALSE);
  gtk_window_set_title (GTK_WINDOW (obj), "Goto Line ...");
  //gtk_dialog_set_close (GNOME_DIALOG (obj), TRUE);

  dialog_vbox = GTK_DIALOG (obj)->vbox;
  gtk_widget_show (dialog_vbox);

  frame = gtk_frame_new (_(" Goto Line number "));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, FALSE, 0);

  numberentry = gtk_entry_new ();
  gtk_widget_show (numberentry);
  gtk_container_add (GTK_CONTAINER (frame), numberentry);
  gtk_container_set_border_width (GTK_CONTAINER (numberentry), 10);

  gtk_dialog_add_button (GTK_DIALOG (obj), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (obj), GTK_STOCK_OK, GTK_RESPONSE_OK);
  
  gtk_dialog_set_default_response (GTK_DIALOG (obj), GTK_RESPONSE_OK);
  gtk_entry_set_activates_default (GTK_ENTRY (numberentry), TRUE);
  gtk_widget_grab_focus (numberentry);
  g_signal_connect (G_OBJECT (obj), "response",
		      G_CALLBACK (on_go_to_line_response),
		      numberentry);
}

GtkWidget *
gotoline_new ()
{
  return GTK_WIDGET (gtk_type_new (gotoline_get_type ()));
}


static void
on_go_to_line_response (GtkDialog* dialog, gint response, gpointer user_data)
{
  GtkEntry *ne;
  TextEditor *te;
  guint num;

  if (response == GTK_RESPONSE_OK) 
  {
	ne = (GtkEntry *) user_data;
	te = anjuta_get_current_text_editor ();
	num = atoi (gtk_entry_get_text (ne));
	if (te)
		text_editor_goto_line (te, num, TRUE);
  }
}
