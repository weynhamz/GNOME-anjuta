/*
    message-manager-private.h
    Copyright (C) 2000, 2001  Kh. Naba Kumar Singh, Johannes Schmid

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

#ifndef _MESSAGE_MANAGER_PRIVATE
#define _MESSAGE_MANAGER_PRIVATE

#include <string>
#include <vector>

#include <zvt/zvtterm.h>

#include "message-manager.h"

using std::vector;
using std::string;

class MessageSubwindow;

struct _AnjutaMessageManagerPrivate
{
	vector<MessageSubwindow*> msg_windows;
	MessageSubwindow* cur_msg_win;
	
	gint last_page;
	
	GtkWidget* box;
	GtkWidget* notebook;
	GtkWidget* popupmenu;
	
	// Only if undocked!
	GtkWidget* window;
	gint xpos;
	gint ypos;
	gint width;
	gint height;
		
	GdkColor color_red;
	GdkColor color_green;
	GdkColor color_blue;
	GdkColor color_white;
	GdkColor color_black;
	
	bool is_docked;
	bool is_shown;
};

class MessageSubwindow
{
	public:
		MessageSubwindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap);
		virtual ~MessageSubwindow() { };
	
		AnjutaMessageManager* get_parent() const;
		
		virtual void show() = 0;
		virtual void hide() = 0;
		
		void activate();
	
		bool is_shown() const;
	
		gint get_page_num() const;
		const string& get_type() const;
		int get_type_id() const;
	
	protected:
		AnjutaMessageManager* m_parent;
		
		GtkWidget* m_menuitem;
		
		int m_page_num;
		string m_pixmap;
		string m_type_name;
		int m_type_id;
		
		bool m_is_shown;

		GtkWidget* create_label() const;
};
	

class AnjutaMessageWindow : public MessageSubwindow
{
	public:
		AnjutaMessageWindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap); 
		virtual ~AnjutaMessageWindow() { };
		
		const vector<string>& get_messages() const;
	
		void add_to_buffer(char c);
		void append_buffer();
		
		void set_cur_line(int line);
		unsigned int get_cur_line() const;
		
		void clear();
	
		void show();
		void hide();
		
		GtkWidget* get_msg_list();
	private:
		GtkWidget* m_msg_list;
		GtkWidget* m_scrolled_win;
		
		vector<string> m_messages;
		string m_line_buffer;
		unsigned int m_cur_line;
};

class TerminalWindow : public MessageSubwindow
{
	public:
		TerminalWindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap);
		virtual ~TerminalWindow() { };
	
		void show();
		void hide();
	private:
		GtkWidget* m_frame;
		GtkWidget* m_hbox;
		GtkWidget* m_terminal;
		GtkWidget* m_scrollbar;
	
		static gboolean zvterm_mouse_clicked(GtkWidget* widget, GdkEvent* event, gpointer user_data);
		static void zvterm_reinit_child(ZvtTerm* term);
		static void zvterm_terminate(ZvtTerm* term);
};

void connect_menuitem_signal(GtkWidget* item, MessageSubwindow* msg_win);
void disconnect_menuitem_signal(GtkWidget* item, MessageSubwindow* msg_win);

#endif
