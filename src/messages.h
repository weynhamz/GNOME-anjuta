/* 
    messages.h
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
#ifndef _MESSAGES_H_
#define _MESSAGES_H_

typedef struct _Messages Messages;
typedef enum _MessageType MessageType;

enum _MessageType
{
	MESSAGE_BUILD = 0,
	MESSAGE_DEBUG = 1,
	MESSAGE_FIND = 2,
	MESSAGE_CVS = 3,
	MESSAGE_LOCALS = 4,
	MESSAGE_TYPE_END = 5,
	MESSAGE_TERMINAL = 5,
	MESSAGE_BUTTONS_END = 6
};

struct _Messages
{
  GtkWidget *GUI;
  GtkWidget *client_area;
  GtkWidget *client;
  GtkWidget *extra_toolbar;
  GtkWidget *scrolledwindow[MESSAGE_TYPE_END];
  GtkWidget *terminal;
  GtkCList *clist[MESSAGE_TYPE_END];
  gint current_pos[MESSAGE_TYPE_END];

  GtkToggleButton *but[MESSAGE_BUTTONS_END];

  GList *data[MESSAGE_TYPE_END];
  gchar *line_buffer[MESSAGE_TYPE_END];
  gint cur_char_pos[MESSAGE_TYPE_END];
  MessageType cur_type;

  GdkColor color_red, color_blue, color_green, color_black, color_white;

  gboolean is_showing;
  gboolean is_docked;
  gint win_pos_x, win_pos_y, win_width, win_height;
  gint toolbar_pos;
};

Messages *messages_new (void);

void create_mesg_gui (Messages * m);

void messages_show (Messages * m, MessageType t);

void messages_hide (Messages * m);

void messages_add_line (Messages * m, MessageType t);

void messages_append (Messages * m, gchar * string, MessageType t);

void messages_previous_message (Messages* m);
void messages_next_message (Messages* m);

void messages_clear (Messages * m, MessageType t);

void messages_destroy (Messages * m);

gboolean messages_save_yourself (Messages * m, FILE * stream);

gboolean messages_load_yourself (Messages * m, PropsID props);

void
on_mesg_clist_select_row (GtkCList * clist,
			  gint row,
			  gint column, GdkEvent * event, gpointer user_data);

void on_mesg_win_dock_clicked (GtkButton * button, gpointer data);

gint
on_mesg_win_delete_event (GtkWidget * w, GdkEvent * event, gpointer data);

void messages_attach (Messages * m);

void messages_detach (Messages * m);

void messages_dock (Messages * m);

void messages_undock (Messages * m);

void messages_update (Messages * m);

void
message_info_locals (GList * lines, gpointer data);
void
message_clear_locals(void);

#endif
