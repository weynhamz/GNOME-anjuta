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

#include <vte/vte.h>

#include "message-manager.h"
#include "debug_tree.h"
#include "watch.h"

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
	GtkWidget *dock_item;
	
	// Only if undocked!
	GtkWidget* window;
	gint xpos;
	gint ypos;
	gint width;
	gint height;
		
	GdkColor color_error;
	GdkColor color_warning;
	GdkColor color_message1;
	GdkColor color_message2;
	
	bool is_docked;
	bool is_shown;
};

class MessageSubwindow
{
	public:
		MessageSubwindow(AnjutaMessageManager* p_amm, int p_type_id,
						 string p_type, string p_pixmap);
		virtual ~MessageSubwindow() { };
	
		AnjutaMessageManager* get_parent() const;
		
		virtual void show() = 0;
		virtual void hide() = 0;
		virtual void clear() = 0;
		
		void activate();
	
		bool is_shown() const;
	
		gint get_page_num() const;
		const string& get_type() const;
		int get_type_id() const;
		
		/* No call this from the on_show_hide_tab callback */
		void set_check_item(bool p_state);
	
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
		AnjutaMessageWindow(AnjutaMessageManager* p_amm, int p_type_id,
							string p_type, string p_pixmap); 
		virtual ~AnjutaMessageWindow() { };
		
		const vector<string>& get_messages() const;
	
		void add_to_buffer(char c);
		void append_buffer();
		
		void set_cur_line(int line);
		unsigned int get_cur_line() const;
		
		void clear();
		void freeze();
		void thaw();
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
		TerminalWindow (AnjutaMessageManager* p_amm, int p_type_id,
					   string p_type, string p_pixmap);
		virtual ~TerminalWindow() { };
	
		void show();
		void hide();
		void clear() { }
		
	private:
		GtkWidget* m_frame;
		GtkWidget* m_hbox;
		GtkWidget* m_terminal;
		GtkWidget* m_scrollbar;
		GtkWidget* m_profile_combo;
		char termenv[255];
		pid_t m_child_pid;
    	
		void preferences_update (void);

		/* Signal handlers */
		static void use_default_profile_cb (GtkToggleButton *button,
											TerminalWindow *tw);
		static gboolean term_focus_cb (GtkWidget *widget,
									   GdkEvent  *event,
									   TerminalWindow *tw);
		static void term_destroy_cb (GtkWidget *widget,
									 TerminalWindow *tw);
		static void term_init_cb (GtkWidget *widget,
								  TerminalWindow *tw);
};

class LocalsWindow : public MessageSubwindow
{
	public:
	
		LocalsWindow(AnjutaMessageManager* p_amm,
					 int p_type_id,
					 string p_type,
					 string p_pixmap);
	
		~LocalsWindow();
		
		void show();
		void hide();
		void update_view(GList* list);
		void clear();
	
	private:
	
		GtkWidget* m_frame;
		DebugTree* m_debug_tree;
		GtkWidget* m_scrollbar;
};

void connect_menuitem_signal(GtkWidget* item, MessageSubwindow* msg_win);
void disconnect_menuitem_signal(GtkWidget* item, MessageSubwindow* msg_win);

#endif
