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
#include "anjuta.h"

using std::vector;
using std::string;

class MessageSubwindow;

struct _AnMessageManagerPrivate
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
		MessageSubwindow(AnMessageManager* p_amm, int p_type_id,
						 string p_type, string p_pixmap);
		virtual ~MessageSubwindow() { };
	
		AnMessageManager* get_parent() const;
		
		GtkWidget* create_scrolled_window(GtkWidget* widget_in_window);
		
		virtual void show() = 0;
		virtual void hide() = 0;
		virtual void clear() {};
		
		void activate();
	
		bool is_shown() const;
	
		gint get_page_num() const;
		const string& get_type() const;
		int get_type_id() const;
		
		/* No call this from the on_show_hide_tab callback */
		void set_check_item(bool p_state);
	
	protected:
		AnMessageManager* m_parent;
		
		GtkWidget* m_menuitem;
		
		int m_page_num;
		string m_pixmap;
		string m_type_name;
		int m_type_id;
		
		bool m_is_shown;

		GtkWidget* create_label() const;
		void hideWidget(GtkWidget* widget);
		void showWidget(GtkWidget* widget);
};
	

class AnMessageWindow : public MessageSubwindow
{
	public:
		AnMessageWindow(AnMessageManager* p_amm, int p_type_id,
							string p_type, string p_pixmap); 
		virtual ~AnMessageWindow() { };
		
		const vector<string>& get_messages() const;
	
		void add_to_buffer(char c);
		void append_buffer();
		
		int get_cur_line();
		string get_cur_msg();
		
		bool select_next();
		bool select_prev();
		
		void clear();
		void show();
		void hide();
		
		GtkWidget* get_msg_list();
		GtkWidget* get_scrolled_win();
	private:
		GtkWidget* m_tree;
		GtkWidget* m_scrolled_win;
		
		vector<string> m_messages;
		string m_line_buffer;

		gulong adj_chgd_hdlr;

		static gboolean on_mesg_event (GtkTreeView* list, GdkEvent * event, gpointer data);
		static void on_adjustment_changed(GtkAdjustment* adj, gpointer data);
		static void on_adjustment_value_changed(GtkAdjustment* adj, gpointer data);
};

class TerminalWindow : public MessageSubwindow
{
	public:
		TerminalWindow (AnMessageManager* p_amm, int p_type_id,
					   string p_type, string p_pixmap);
		virtual ~TerminalWindow() { };
	
		void show();
		void hide();
		
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
		static gboolean term_keypress_cb (GtkWidget *widget,
										  GdkEventKey  *event,
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
	
		LocalsWindow(AnMessageManager* p_amm,
					 int p_type_id,
					 string p_type,
					 string p_pixmap);
	
		~LocalsWindow();
		
		void show();
		void hide();
		void update_view(GList* list);
		void clear();
	
	private:
	
		DebugTree* m_debug_tree;
		GtkWidget* m_scrollbar;
};


/* this class provides a basic facility for an externally
   defined widget to be displayed within a message window */
class WidgetWindow : public MessageSubwindow
{
	public:
	
		WidgetWindow(AnMessageManager* p_amm,
					 int p_type_id,
					 string p_type,
					 string p_pixmap);
	
		~WidgetWindow();
		
		void show();
		void hide();
		void set_widget(GtkWidget* w);
	
	private:

		GtkWidget* m_widget;	
		GtkWidget* m_scrollbar;
};

void connect_menuitem_signal(GtkWidget* item, MessageSubwindow* msg_win);
void disconnect_menuitem_signal(GtkWidget* item, MessageSubwindow* msg_win);

#endif
