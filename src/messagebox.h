/* 
    messagebox.h
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
#ifndef  _MESSAGE_BOX_H_
#define  _MESSAGE_BOX_H_

#include "global.h"

/************************************************************************
**  These are the available stock buttons to be used in button[][] field
**  in MesgBoxData
*************************************************************************
GNOME_STOCK_BUTTON_OK
GNOME_STOCK_BUTTON_CANCEL 
GNOME_STOCK_BUTTON_YES    
GNOME_STOCK_BUTTON_NO     
GNOME_STOCK_BUTTON_CLOSE  
GNOME_STOCK_BUTTON_APPLY  
GNOME_STOCK_BUTTON_HELP  
GNOME_STOCK_BUTTON_NEXT  
GNOME_STOCK_BUTTON_PREV  
GNOME_STOCK_BUTTON_UP    
GNOME_STOCK_BUTTON_DOWN  
GNOME_STOCK_BUTTON_FONT  

*************************************************************************
*** These are the stock message type to be used in type field
*** in MesgBoxData
*************************************************************************

 GNOME_MESSAGE_BOX_INFO
 GNOME_MESSAGE_BOX_WARNING
 GNOME_MESSAGE_BOX_ERROR
 GNOME_MESSAGE_BOX_QUESTION
 GNOME_MESSAGE_BOX_GENERIC  

*************************************************************************/

typedef struct _MesgBoxData MesgBoxData;

struct _MesgBoxData
{
  gchar         *type;
  gchar         *mesg;
  guint          no_of_buttons;
  gchar         *buttons[MAX_NO_OF_BUTTONS];
  GtkSignalFunc  callbacks[MAX_NO_OF_BUTTONS];
  gpointer     data;
};

GtkWidget* create_messagebox_gui(MesgBoxData);

/* Some handfull  message boxs with 1/2/3 buttons */

void messagebox(char *type, char* mesg); //Just displays the message


void messagebox1(char *type,    //Message box with 1 button
                       char *mesg,
                       char *butt_type,
                       GtkSignalFunc callback,
                       gpointer data );

void messagebox2(char *type,    //Message box with 2 buttons
                       char *mesg,
                       char *butt1_type, char *butt2_type,
                       GtkSignalFunc callback1,
                       GtkSignalFunc callback2,
                       gpointer data );

void messagebox3(char *type,    //Message box with 3 buttons
                       char *mesg,
                       char *butt1_type,char *butt2_type,char *butt3_type,
                       GtkSignalFunc callback1,
                       GtkSignalFunc callback2,
                       GtkSignalFunc callback3,
                       gpointer data );                        

#endif

