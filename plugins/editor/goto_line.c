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

#include <libanjuta/resources.h>

#include "text_editor.h"
// #include "anjuta.h"
#include "goto_line.h"

struct _GotoLinePriv
{
	TextEditor *te;
};

/* Widget implementation headers */
/*
enum
{
  LAST_SIGNAL
};
*/

static GtkDialogClass *parent_class;

static void gotoline_class_init (GotoLineClass * klass);
static void gotoline_init (GotoLine * obj);

/* static gint gotoline_signals[LAST_SIGNAL + 1]; */

/* Widget function headers */

/** Jumps Current text buffer to entered line number */
static void on_go_to_line_response (GtkDialog* dialog, gint response,
                                    gpointer user_data);

GType
gotoline_get_type ()
{
	static GType obj_type = 0;
	
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
gotoline_finalize (GObject *obj)
{
	g_free (GOTOLINE (obj)->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
gotoline_class_init (GotoLineClass * klass)
{
  GObjectClass *object_class;
  assert (klass != NULL);
  parent_class = g_type_class_peek_parent (klass);
  object_class = (GObjectClass *) klass;
  object_class->finalize = gotoline_finalize;

  //g_object_class_add_signals (object_class, gotoline_signals, LAST_SIGNAL);
}


static void
gotoline_init (GotoLine * obj)
{
  GtkWidget *dialog_vbox;
  GtkWidget *frame;
  GtkWidget *numberentry;
  GtkWidget *vbox;
  GtkWidget *label;
	
  assert (obj != NULL);

  obj->priv = g_new0 (GotoLinePriv, 1);
  obj->priv->te = NULL;

  gtk_window_set_position (GTK_WINDOW (obj), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (obj), FALSE);
  gtk_window_set_title (GTK_WINDOW (obj), "Go to Line ...");

  dialog_vbox = GTK_DIALOG (obj)->vbox;
  gtk_widget_show (dialog_vbox);

  label = gtk_label_new(NULL);
  gtk_label_set_markup (GTK_LABEL (label), _("<b>Go to Line number:</b>"));
  gtk_widget_show (label);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  
  numberentry = gtk_entry_new ();
  gtk_widget_show (numberentry);
  gtk_container_add (GTK_CONTAINER (vbox), numberentry);

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
gotoline_new (TextEditor *te)
{
  GotoLine *gotoline;
  g_return_val_if_fail (IS_TEXT_EDITOR (te), NULL);
  gotoline = GOTOLINE (g_type_create_instance (gotoline_get_type ()));
  gotoline->priv->te = te;
  return GTK_WIDGET (gotoline);
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
	te = GOTOLINE (dialog)->priv->te;
	g_return_if_fail (IS_TEXT_EDITOR (te));
	num = atoi (gtk_entry_get_text (ne));
	if (te)
		text_editor_goto_line (te, num, TRUE, TRUE);
  }
  gtk_widget_hide (GTK_WIDGET(dialog));
}
