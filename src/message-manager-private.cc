/*
    message-manager-private.cc
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

#include "message-manager-private.h"
#include "message-manager-dock.h"
#include "preferences.h"
#include "debug_tree.h"
#include "watch.h"

extern "C"
{
	#include "utilities.h"
};

#include <zvt/zvtterm.h>
#include <pwd.h>

#ifdef DEBUG
  #define DEBUG_PRINT g_message
#else
  #define DEBUG_PRINT(ARGS...)
#endif

// MessageSubwindow (base class for AnjutaMessageWindow and TerminalWindow:

MessageSubwindow::MessageSubwindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap)
{	
	g_return_if_fail(p_amm != NULL);
	
	// Create menuitem
	m_menuitem = gtk_check_menu_item_new_with_label(p_type.c_str());
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menuitem), true);
	connect_menuitem_signal(m_menuitem, this);
	gtk_widget_show(m_menuitem);
	gtk_menu_append(GTK_MENU(p_amm->intern->popupmenu), m_menuitem);
	
	m_parent = p_amm;
	
	m_type_name = p_type;
	m_type_id = p_type_id;
	m_pixmap = p_pixmap;
	m_is_shown = true;

	p_amm->intern->last_page++;
	m_page_num = p_amm->intern->last_page;
}

AnjutaMessageManager* 
MessageSubwindow::get_parent() const
{
	return m_parent;
}
			
bool 
MessageSubwindow::is_shown() const
{
	return m_is_shown;
}
	
gint 
MessageSubwindow::get_page_num() const
{
	return m_page_num;
}

const string&
MessageSubwindow::get_type() const
{
	return m_type_name;
}

int MessageSubwindow::get_type_id() const
{
	return m_type_id;
}

void MessageSubwindow::activate()
{
	if (!m_is_shown)
	{
		show();
		set_check_item(true);
		
		// reorder windows
		vector<bool> win_is_shown;
	
		for (uint i = 0; i < m_parent->intern->msg_windows.size(); i++)
		{
			win_is_shown.push_back(m_parent->intern->msg_windows[i]->is_shown());
			m_parent->intern->msg_windows[i]->hide();
		}
		for (uint i = 0; i < m_parent->intern->msg_windows.size(); i++)
		{
			if (win_is_shown[i])
				m_parent->intern->msg_windows[i]->show();
		}
	}	
	gtk_notebook_set_page(GTK_NOTEBOOK(m_parent->intern->notebook), m_page_num);
	m_parent->intern->cur_msg_win = this;
}

GtkWidget* MessageSubwindow::create_label() const
{
	GtkWidget* label = create_xpm_label_box(GTK_WIDGET(m_parent), (gchar*) m_pixmap.c_str(), FALSE, (gchar*) m_type_name.c_str());
	gtk_widget_ref(label);
	gtk_widget_show(label);
	return label;
}

void MessageSubwindow::set_check_item(bool p_state)
{
	disconnect_menuitem_signal(m_menuitem, this);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menuitem), p_state);	
	connect_menuitem_signal(m_menuitem, this);
}

// MessageWindow:

AnjutaMessageWindow::AnjutaMessageWindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap)
	: MessageSubwindow(p_amm, p_type_id, p_type, p_pixmap)
{
	g_return_if_fail(p_amm != NULL);

	m_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(m_scrolled_win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(m_scrolled_win),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	m_msg_list = gtk_clist_new(1);
	gtk_container_add(GTK_CONTAINER(m_scrolled_win), m_msg_list);
	gtk_widget_show(m_msg_list);
	gtk_clist_columns_autosize (GTK_CLIST(m_msg_list));
	gtk_clist_set_selection_mode(GTK_CLIST(m_msg_list), GTK_SELECTION_BROWSE);
	
	GtkWidget* label = create_label();
	
	gtk_notebook_append_page(GTK_NOTEBOOK(p_amm->intern->notebook), m_scrolled_win, label);
	
	set_cur_line(0);
}

const vector<string>& 
AnjutaMessageWindow::get_messages() const
{
	return m_messages;
}
	
void
AnjutaMessageWindow::add_to_buffer(char c)
{
	m_line_buffer += c;
}

void
AnjutaMessageWindow::append_buffer()
{
	gtk_clist_freeze (GTK_CLIST(m_msg_list));

	GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
						     (m_scrolled_win));

	/* If the scrollbar is almost at the end, */
	/* then only we scroll to the end */
	bool update_adj = false;
	if (adj->value > 0.95 * (adj->upper-adj->page_size) || adj->value == 0)
		update_adj = true;
	
	string message = m_line_buffer;
	m_line_buffer = string();
	m_messages.push_back(message);
	
	// Truncate Message:
	int truncat_mesg = preferences_get_int (get_preferences(), TRUNCAT_MESSAGES);
	int mesg_first = preferences_get_int (get_preferences(), TRUNCAT_MESG_FIRST);
	int mesg_last = preferences_get_int (get_preferences(), TRUNCAT_MESG_LAST);

	if (truncat_mesg == FALSE
	    || message.length() <= uint(mesg_first + mesg_last))
	{
		char * msg = new char[message.length() + 1];
		strcpy(msg, message.c_str());
		gtk_clist_append(GTK_CLIST(m_msg_list), &msg);
		delete []msg;
	}
	else
	{
		string part1(message.begin(), message.begin() + mesg_first);
		string part2(message.end() - mesg_last, message.end());
		string m1 = part1 + " ................... " + part2;
		char* msg = new char[m1.length() + 1];
		strcpy(msg, m1.c_str());
		gtk_clist_append(GTK_CLIST(m_msg_list), &msg);
		delete []msg;	
	}

	// Highlite messages:
	int dummy_int;
	char* dummy_fn;
	if (parse_error_line((char*) message.c_str(), &dummy_fn, &dummy_int))
	{
		if (message.find(" warning: ") != message.npos)
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_warning);
			anjuta_message_manager_indicate_warning (m_parent, m_type_id, dummy_fn, dummy_int);
		}
		else
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_error);
			anjuta_message_manager_indicate_error (m_parent, m_type_id, dummy_fn, dummy_int);
		}
	}
	else
	{
		if (message.find(':') != message.npos)
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_message1);
		}
		else
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_message2);
		}
	}
	g_free(dummy_fn);

	gtk_clist_thaw(GTK_CLIST(m_msg_list));
	gtk_clist_columns_autosize (GTK_CLIST(m_msg_list));
	if (update_adj) 
		gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

void
AnjutaMessageWindow::set_cur_line(int line)
{
	m_cur_line = line;
}

unsigned int
AnjutaMessageWindow::get_cur_line() const
{
	return m_cur_line;
}
			
void
AnjutaMessageWindow::clear()
{
	m_messages.clear();
	gtk_clist_clear(GTK_CLIST(m_msg_list));
	set_cur_line(0);
}

void
AnjutaMessageWindow::freeze()
{
	gtk_clist_freeze(GTK_CLIST(m_msg_list));
}

void
AnjutaMessageWindow::thaw()
{
	gtk_clist_thaw(GTK_CLIST(m_msg_list));
}

void 
AnjutaMessageWindow::show()
{
	if (!m_is_shown)
	{
		GtkWidget* label = create_label();
		
		gtk_notebook_append_page(GTK_NOTEBOOK(m_parent->intern->notebook), m_scrolled_win, label);
		gtk_widget_unref(m_scrolled_win);
		
		GList* children = gtk_container_children(GTK_CONTAINER(m_parent->intern->notebook));
		for (uint i = 0; i < g_list_length(children); i++)
		{
			if (g_list_nth(children, i)->data == m_scrolled_win)
			{
				m_page_num = i;
				break;
			}
		}
		m_is_shown = true;
	}
}

void 
AnjutaMessageWindow::hide()
{
	if (m_is_shown)
	{
		gtk_widget_ref(m_scrolled_win);
		gtk_container_remove(GTK_CONTAINER(m_parent->intern->notebook), m_scrolled_win);
		
		m_is_shown = false;
	}
}

GtkWidget* AnjutaMessageWindow::get_msg_list()
{
	return m_msg_list;
}

TerminalWindow::TerminalWindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap)
	: MessageSubwindow(p_amm, p_type_id, p_type, p_pixmap)
{	
	g_return_if_fail(p_amm != NULL);

	/* Set terminal preferences */
	gchar *font = preferences_get(get_preferences(), TERMINAL_FONT);
	if (!font) font = g_strdup(DEFAULT_ZVT_FONT);
	gint scrollsize = preferences_get_int(get_preferences(), TERMINAL_SCROLLSIZE);
	if (scrollsize < DEFAULT_ZVT_SCROLLSIZE)
		scrollsize = DEFAULT_ZVT_SCROLLSIZE;
	if (scrollsize > MAX_ZVT_SCROLLSIZE)
		scrollsize = MAX_ZVT_SCROLLSIZE;
	gchar *term = preferences_get(get_preferences(), TERMINAL_TERM);
    if (!term) term = g_strdup(DEFAULT_ZVT_TERM);
	guchar *wordclass = (guchar *) preferences_get(get_preferences(), TERMINAL_WORDCLASS);
	if (!wordclass) wordclass = (guchar *) g_strdup(DEFAULT_ZVT_WORDCLASS);
	g_snprintf(termenv, 255L, "TERM=%s", term);
	g_free(term);

	DEBUG_PRINT("Font: '%s'", font);
	DEBUG_PRINT("Scroll Buffer: '%d'", scrollsize);
	DEBUG_PRINT("TERM: '%s'", termenv);
	DEBUG_PRINT("Word characters: '%s'", wordclass);

	putenv(termenv);
	// putenv("TERM=xterm");
	m_terminal = zvt_term_new();
	zvt_term_set_font_name(ZVT_TERM(m_terminal), font);
	g_free(font);
	zvt_term_set_blink(ZVT_TERM(m_terminal), preferences_get_int(
	  get_preferences(), TERMINAL_BLINK) ? TRUE : FALSE);
	zvt_term_set_bell(ZVT_TERM(m_terminal), preferences_get_int(
	  get_preferences(), TERMINAL_BELL) ? TRUE : FALSE);
	zvt_term_set_scrollback(ZVT_TERM(m_terminal), scrollsize);
	zvt_term_set_scroll_on_keystroke(ZVT_TERM(m_terminal)
	  , preferences_get_int(get_preferences(), TERMINAL_SCROLL_KEY
	  ) ? TRUE : FALSE);
	zvt_term_set_scroll_on_output(ZVT_TERM(m_terminal)
	  , preferences_get_int(get_preferences(), TERMINAL_SCROLL_OUTPUT
	  ) ? TRUE : FALSE);
	zvt_term_set_background(ZVT_TERM(m_terminal), NULL, 0, 0);
	zvt_term_set_wordclass(ZVT_TERM(m_terminal), wordclass);
	g_free(wordclass);
#ifdef ZVT_TERM_MATCH_SUPPORT
	zvt_term_match_add	(ZVT_TERM(m_terminal),
		"^[-A-Za-z0-9_\\/.]+:[0-9]+:.*$",
		VTATTR_UNDERLINE, NULL);
#endif
	gtk_widget_show(m_terminal);

	m_scrollbar = gtk_vscrollbar_new(GTK_ADJUSTMENT(
	  ZVT_TERM(m_terminal)->adjustment));
	GTK_WIDGET_UNSET_FLAGS(m_scrollbar, GTK_CAN_FOCUS);
	
	m_frame = gtk_frame_new(NULL);
	gtk_widget_show (m_frame);
	gtk_frame_set_shadow_type(GTK_FRAME(m_frame), GTK_SHADOW_IN);
	gtk_notebook_append_page(GTK_NOTEBOOK(p_amm->intern->notebook), m_frame, create_label());

	m_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show (m_hbox);
	gtk_container_add (GTK_CONTAINER(m_frame), m_hbox);
	
	gtk_box_pack_start(GTK_BOX(m_hbox), m_terminal, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(m_hbox), m_scrollbar, FALSE, TRUE, 0);
	gtk_widget_show (m_scrollbar);

	//zvterm_reinit_child(ZVT_TERM(m_terminal));
	gtk_signal_connect(GTK_OBJECT(m_terminal), "button_press_event"
	  , GTK_SIGNAL_FUNC(TerminalWindow::zvterm_mouse_clicked), m_terminal);
	gtk_signal_connect (GTK_OBJECT(m_terminal), "child_died"
	  , GTK_SIGNAL_FUNC (TerminalWindow::zvterm_reinit_child), NULL);
	gtk_signal_connect (GTK_OBJECT (m_terminal),"destroy"
	  , GTK_SIGNAL_FUNC (TerminalWindow::zvterm_terminate), NULL);

	gtk_signal_connect (GTK_OBJECT (m_terminal), "focus_in_event",
		GTK_SIGNAL_FUNC (TerminalWindow::zvterm_focus_in), NULL);
}

void TerminalWindow::show()
{
	if (!m_is_shown)
	{		
		GtkWidget* label = create_label();
		
		gtk_notebook_append_page(GTK_NOTEBOOK(m_parent->intern->notebook), m_frame, label);
		gtk_widget_unref(m_frame);
		
		GList* children = gtk_container_children(GTK_CONTAINER(m_parent->intern->notebook));
		for (uint i = 0; i < g_list_length(children); i++)
		{
			if (g_list_nth(children, i)->data == m_frame)
			{
				m_page_num = i;
				break;
			}
		}
		m_is_shown = true;
	}
}
	
void TerminalWindow::hide()
{
	if (m_is_shown)
	{
		gtk_widget_ref(m_frame);
		gtk_container_remove(GTK_CONTAINER(m_parent->intern->notebook), m_frame);
		m_is_shown = false;
	}
}

void mouse_to_char(ZvtTerm *term, int mousex, int mousey, int *x, int *y)
{
	*x = mousex/term->charwidth;
	*y = mousey/term->charheight;
}

extern "C" void anjuta_goto_file_line (gchar * fname, glong lineno);

gboolean TerminalWindow::zvterm_mouse_clicked(GtkWidget* widget, GdkEvent* event, gpointer user_data)
{
  int x,y;
  gchar *line = NULL;
  gchar *filename;
  gchar *procpath;
  int lineno;
	GtkWidget* terminal = GTK_WIDGET(user_data);
  ZvtTerm *term = ZVT_TERM(terminal);
	gtk_widget_grab_focus(terminal);
  // double left button click
  if (event->type == GDK_2BUTTON_PRESS && event->button.button==1)
  {
    mouse_to_char(term, (int)event->button.x, (int)event->button.y, &x, &y);
    DEBUG_PRINT("coord: %d %d, scrollbackoffset: %d", x, y, term->vx->vt.scrollbackoffset);
    line = zvt_term_get_buffer(term, NULL, VT_SELTYPE_LINE, 0, y+term->vx->vt.scrollbackoffset, 0, y+term->vx->vt.scrollbackoffset);
    DEBUG_PRINT("got line: '%s'", line);
    filename = NULL;
    if (parse_error_line(line, &filename, &lineno))
    {
      DEBUG_PRINT("parse_error_line: '%s' %d", filename, lineno);
      // look for the file in the cwd
      if (filename[0]!='/')
        procpath = g_strdup_printf("/proc/%d/cwd/%s", term->vx->vt.childpid, filename);
      else
        procpath = g_strdup(filename);
      DEBUG_PRINT("full linked path: %s", procpath);
      anjuta_goto_file_line (procpath, lineno);
      g_free(procpath);
      g_free(filename);
    }
    g_free(line);
  }
	return TRUE;
}

extern char **environ;

void TerminalWindow::zvterm_reinit_child(ZvtTerm* term)
{
	struct passwd *pw;
	static GString *shell = NULL;
	static GString *name = NULL;
  int pid;

	if (!shell)
		shell = g_string_new(NULL);
	if (!name)
		name = g_string_new(NULL);
	zvt_term_reset(term, TRUE);
	pid = zvt_term_forkpty(term, ZVT_TERM_DO_UTMP_LOG |  ZVT_TERM_DO_WTMP_LOG);
  switch (pid)
	{
		case -1:
			break;
		case 0:
			pw = getpwuid(getuid());
			if (pw)
			{
				g_string_assign(shell, pw->pw_shell);
				g_string_assign(name, "-");
			}
			else
			{
				g_string_assign(shell, "/bin/sh");
				g_string_assign(name, "-sh");
			}
			execle (shell->str, name->str, NULL, environ);
		default:
      DEBUG_PRINT("zvt terminal shell pid: %d\n", pid);
			break;
	}
}

void TerminalWindow::zvterm_terminate(ZvtTerm* term)
{
	gtk_signal_disconnect_by_func(GTK_OBJECT(term), GTK_SIGNAL_FUNC(TerminalWindow::zvterm_reinit_child), NULL);
	zvt_term_closepty(term);
}

int TerminalWindow::zvterm_focus_in(ZvtTerm* term, GdkEventFocus* event)
{
	static bool need_init = true;
	if (need_init)
	{
		zvterm_reinit_child(term);
		need_init = false;
	}
	return true;
}

LocalsWindow::LocalsWindow (AnjutaMessageManager * p_amm, int p_type_id,
		  string p_type, string p_pixmap):
			MessageSubwindow (p_amm, p_type_id, p_type, p_pixmap)
{
	g_return_if_fail (p_amm != NULL);

	m_scrollbar = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_scrollbar),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (m_scrollbar);
	m_debug_tree = debug_tree_create (m_scrollbar);
	m_frame = gtk_event_box_new ();
	gtk_widget_show (m_frame);
	//gtk_frame_set_shadow_type (GTK_FRAME (m_frame), GTK_SHADOW_IN);
	gtk_notebook_append_page (GTK_NOTEBOOK (p_amm->intern->notebook),
				  m_frame, create_label ());

	gtk_container_add (GTK_CONTAINER (m_frame), m_scrollbar);
}

LocalsWindow::~LocalsWindow ()
{
	/* TODO: where is it called? */
	g_print("Destructor of localwindow called\n");
	debug_tree_destroy (m_debug_tree);
}

void
LocalsWindow::show ()
{
	if (!m_is_shown)
	{
		GtkWidget *label = create_label ();

		gtk_notebook_append_page (GTK_NOTEBOOK
					  (m_parent->intern->notebook),
					  m_frame, label);
		gtk_widget_unref (m_frame);

		GList *children =
			gtk_container_children (GTK_CONTAINER
						(m_parent->intern->notebook));
		for (uint i = 0; i < g_list_length (children); i++)
		{
			if (g_list_nth (children, i)->data == m_frame)
			{
				m_page_num = i;
				break;
			}
		}
		m_is_shown = true;
	}
}

void
LocalsWindow::hide ()
{
	if (m_is_shown)
	{
		gtk_widget_ref (m_frame);
		gtk_container_remove (GTK_CONTAINER
				      (m_parent->intern->notebook), m_frame);
		m_is_shown = false;
	}
}

void
LocalsWindow::clear ()
{
	debug_tree_clear (m_debug_tree);
}

void
LocalsWindow::update_view (GList * list)
{
	debug_tree_parse_variables (m_debug_tree, list);
}
