/*
    messagebox.c
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

#include "text_editor.h"
#include "mainmenu_callbacks.h"
#include "messagebox.h"
#include "resources.h"

GtkWidget*
create_messagebox_gui (MesgBoxData mbd)
{
  GtkWidget *messagebox_gui;
  GtkWidget *dialog_vbox;
  GtkWidget *button;
  GtkWidget *dialog_action_area;
  gint i;
  if( mbd.no_of_buttons > 10)return NULL;
  messagebox_gui = gnome_message_box_new (mbd.mesg, mbd.type, NULL);
  gtk_window_set_position (GTK_WINDOW (messagebox_gui), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (messagebox_gui), FALSE, FALSE, FALSE);
  gtk_window_set_modal (GTK_WINDOW (messagebox_gui), TRUE);
  gnome_dialog_set_close (GNOME_DIALOG (messagebox_gui), TRUE);

  dialog_vbox = GNOME_DIALOG (messagebox_gui)->vbox;
  gtk_widget_show (dialog_vbox);

  for(i=0;i<mbd.no_of_buttons;i++)
  {
     gnome_dialog_append_button (GNOME_DIALOG (messagebox_gui), mbd.buttons[i]);
     button = g_list_last (GNOME_DIALOG (messagebox_gui)->buttons)->data;
     gtk_widget_show (button);
     GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

     gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (mbd.callbacks[i]),
                      mbd.data);
  }


  dialog_action_area = GNOME_DIALOG (messagebox_gui)->action_area;

  return messagebox_gui;
}

/* Just displays the message */
void
messagebox(char *type, char* mesg) 
{
  messagebox1(type,
              mesg,
              GNOME_STOCK_BUTTON_OK,
              GTK_SIGNAL_FUNC(NULL),
              NULL);
}

void
messagebox1           (char *type,    //Message box with 1 button
                       char *mesg,
                       char *butt_type,
                       GtkSignalFunc callback,
                       gpointer data )
{
  MesgBoxData mbd;
  GtkWidget *mb;
  
  mbd.type = type;
  mbd.mesg = mesg;
  mbd.no_of_buttons = 1;
  mbd.buttons[0] = butt_type;
  mbd.callbacks[0] = callback;
  mbd.data = data; 
  
  mb = create_messagebox_gui(mbd);
  gtk_widget_show(mb);
}

void
messagebox2           (char *type,    //Message box with 2 buttons
                       char *mesg,
                       char *butt1_type, char *butt2_type,
                       GtkSignalFunc callback1,
                       GtkSignalFunc callback2,
                       gpointer data )
{
  MesgBoxData mbd;
  GtkWidget *mb;
  
  mbd.type = type;
  mbd.mesg = mesg;
  mbd.no_of_buttons = 2;
  mbd.buttons[0] = butt1_type;
  mbd.buttons[1] = butt2_type;
  mbd.callbacks[0] = callback1;
  mbd.callbacks[1] = callback2;
  mbd.data = data; 
  
  mb = create_messagebox_gui(mbd);
  gtk_widget_show(mb); 
}

void
messagebox3           (char *type,    //Message box with 3 buttons
                       char *mesg,
                       char *butt1_type,char *butt2_type,char *butt3_type,
                       GtkSignalFunc callback1,
                       GtkSignalFunc callback2,
                       GtkSignalFunc callback3,
                       gpointer data )
{
  MesgBoxData mbd;
  GtkWidget *mb;
  
  mbd.type = type;
  mbd.mesg = mesg;
  mbd.no_of_buttons = 3;
  mbd.buttons[0] = butt1_type;
  mbd.buttons[1] = butt2_type;
  mbd.buttons[2] = butt3_type;
  mbd.callbacks[0] = callback1;
  mbd.callbacks[1] = callback2;
  mbd.callbacks[2] = callback3;
  mbd.data = data; 
  
  mb = create_messagebox_gui(mbd);
  gtk_widget_show(mb); 
}