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

#include <config.h>
#include "message-manager-private.h"
#include "message-manager-dock.h"
#include "preferences.h"
#include "debug_tree.h"
#include "watch.h"

/* Some desktop/gnome-terminal gconf keys. */
#define GCONF_MONOSPACE_FONT      "/desktop/gnome/interface/monospace_font_name"
#define GCONF_DEFAULT_PROFILE     "/apps/gnome-terminal/global/default_profile"
#define GCONF_PROFILE_LIST        "/apps/gnome-terminal/global/profile_list"

#define GCONF_PROFILE_PREFIX      "/apps/gnome-terminal/profiles"
#define GCONF_BACKGROUND_COLOR    "background_color"
#define GCONF_BACKSPACE_BINDING   "backspace_binding"
#define GCONF_CURSOR_BLINK        "cursor_blink"
#define GCONF_DELETE_BINDING      "delete_binding"
#define GCONF_EXIT_ACTION         "exit_action"
#define GCONF_VTE_TERMINAL_FONT   "font"
#define GCONF_FOREGROUND_COLOR    "foreground_color"
#define GCONF_SCROLLBACK_LINES    "scrollback_lines"
#define GCONF_SCROLL_ON_KEYSTROKE "scroll_on_keystroke"
#define GCONF_SCROLL_ON_OUTPUT    "scroll_on_output"
#define GCONF_SILENT_BELL         "silent_bell"
#define GCONF_USE_SYSTEM_FONT     "use_system_font"
#define GCONF_WORD_CHARS          "word_chars"

#define PREFS_TERMINAL_PROFILE_USE_DEFAULT    "terminal.default.profile"
#define PREFS_TERMINAL_PROFILE                "terminal.profile"

extern "C"
{
	#include "utilities.h"
};

#include <gconf/gconf-client.h>
#include <vte/vte.h>
#include <pwd.h>
#include <gtk/gtk.h>

#ifdef DEBUG
  #define DEBUG_PRINT g_message
#else
  #define DEBUG_PRINT(ARGS...)
#endif

enum
{
	COLUMN_LINE,
	COLUMN_COLOR,
	COLUMN_MESSAGES,
	N_COLUMNS
};

// MessageSubwindow (base class for AnMessageWindow and TerminalWindow:

MessageSubwindow::MessageSubwindow(AnMessageManager* p_amm,
								   int p_type_id, string p_type,
								   string p_pixmap)
{	
	g_return_if_fail(p_amm != NULL);
	
	// Create menuitem
	m_menuitem = gtk_check_menu_item_new_with_label(p_type.c_str());
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menuitem), true);
	connect_menuitem_signal(m_menuitem, this);
	gtk_widget_show(m_menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(p_amm->intern->popupmenu), m_menuitem);
	
	m_parent = p_amm;
	
	m_type_name = p_type;
	m_type_id = p_type_id;
	m_pixmap = p_pixmap;
	m_is_shown = true;

	p_amm->intern->last_page++;
	m_page_num = p_amm->intern->last_page;
}

GtkWidget* 
MessageSubwindow::create_scrolled_window(GtkWidget* widget_in_window)
{
	GtkWidget* scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_win),
									GTK_POLICY_AUTOMATIC, 
									GTK_POLICY_AUTOMATIC);
	
	gtk_container_add(GTK_CONTAINER(scrolled_win), widget_in_window);
	gtk_widget_show(widget_in_window);
	
	GtkWidget* label = create_label();
	
	gtk_notebook_append_page(GTK_NOTEBOOK(m_parent->intern->notebook),
							 scrolled_win, label);
	
	return scrolled_win;
}

void
MessageSubwindow::hideWidget(GtkWidget* widget)
{
	if (m_is_shown)
	{
		gtk_widget_ref(widget);
		gtk_container_remove(GTK_CONTAINER(m_parent->intern->notebook), widget);		
		m_is_shown = false;
	}
}

void 
MessageSubwindow::showWidget(GtkWidget* widget)
{
	if (!m_is_shown)
	{
		GtkWidget* label = create_label();
		
		gtk_notebook_append_page (GTK_NOTEBOOK(m_parent->intern->notebook),
								  widget, label);
		gtk_widget_unref(widget);
		
		GList* children = gtk_container_get_children
								(GTK_CONTAINER (m_parent->intern->notebook));
		for (uint i = 0; i < g_list_length(children); i++)
		{
			if (g_list_nth(children, i)->data == widget)
			{
				m_page_num = i;
				break;
			}
		}
		m_is_shown = true;
	}
}

AnMessageManager* 
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
	gtk_notebook_set_current_page(GTK_NOTEBOOK(m_parent->intern->notebook), m_page_num);
	m_parent->intern->cur_msg_win = this;
}

GtkWidget* MessageSubwindow::create_label() const
{
	GtkWidget* label = create_xpm_label_box (GTK_WIDGET(m_parent),
											 (gchar*) m_pixmap.c_str(),
											 FALSE,
											 (gchar*) m_type_name.c_str());
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

AnMessageWindow::AnMessageWindow(AnMessageManager* p_amm,
										 int p_type_id, string p_type,
										 string p_pixmap)
	: MessageSubwindow(p_amm, p_type_id, p_type, p_pixmap)
{
	g_return_if_fail(p_amm != NULL);

	// m_line = 0;		
	
	m_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(m_scrolled_win),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	// Create Tree
	m_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
		gtk_list_store_new(N_COLUMNS, G_TYPE_INT, GDK_TYPE_COLOR, G_TYPE_STRING)));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(m_tree), FALSE);
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_title(column, p_type.c_str());
	gtk_tree_view_column_add_attribute
		(column, renderer, "foreground-gdk", COLUMN_COLOR);
	gtk_tree_view_column_add_attribute
		(column, renderer, "text", COLUMN_MESSAGES);
	gtk_tree_view_append_column(GTK_TREE_VIEW(m_tree), column);
	GtkTreeSelection* select = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_tree));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);	

	gtk_container_add(GTK_CONTAINER(m_scrolled_win), m_tree);
	gtk_notebook_append_page (GTK_NOTEBOOK(p_amm->intern->notebook),
							  m_scrolled_win, create_label());
	gtk_widget_show_all(m_scrolled_win);
	
	g_signal_connect(G_OBJECT(m_tree), "event", 
		G_CALLBACK(AnMessageWindow::on_mesg_event), this);
	GtkAdjustment* adj =
			gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
												     (m_scrolled_win));
	adj_chgd_hdlr = g_signal_connect(G_OBJECT(adj), "changed",
		G_CALLBACK(AnMessageWindow::on_adjustment_changed), this);
	g_signal_connect(G_OBJECT(adj), "value_changed",
		G_CALLBACK(AnMessageWindow::on_adjustment_value_changed), this);
}

const vector<string>& 
AnMessageWindow::get_messages() const
{
	return m_messages;
}
	
void
AnMessageWindow::add_to_buffer(char c)
{
	m_line_buffer += c;
}

void
AnMessageWindow::append_buffer()
{
/*	GtkAdjustment* adj =
			gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
												     (m_scrolled_win)); */
	string message = m_line_buffer;
	m_line_buffer = string();
	m_messages.push_back(message);

	// Truncate Message:
	int truncat_mesg = anjuta_preferences_get_int (get_preferences(), TRUNCAT_MESSAGES);
	int mesg_first = anjuta_preferences_get_int (get_preferences(), TRUNCAT_MESG_FIRST);
	int mesg_last = anjuta_preferences_get_int (get_preferences(), TRUNCAT_MESG_LAST);

	string c_message;
	if (truncat_mesg == FALSE
	    || message.length() <= uint(mesg_first + mesg_last))
	{
		c_message = message;
	}
	else
	{
		string part1(message.begin(), message.begin() + mesg_first);
		string part2(message.end() - mesg_last, message.end());
		string m1 = part1 + " ................... " + part2;
		c_message = m1;
	}
	
	// Highlite messages:
	int dummy_int;
	char* dummy_fn;
	GdkColor color; 
	if (parse_error_line(message.c_str(), &dummy_fn, &dummy_int))
	{
		if (message.find(_(" warning: ")) != message.npos)
		{
			color =	m_parent->intern->color_warning;
			an_message_manager_indicate_warning (m_parent, m_type_id, dummy_fn, dummy_int);
		}
		else
		{
			color =	m_parent->intern->color_error;
			an_message_manager_indicate_error (m_parent, m_type_id, dummy_fn, dummy_int);
		}
	}
	else
	{
		if (message.find(':') != message.npos)
		{
			color =	m_parent->intern->color_message1;
		}
		else
		{
			color =	m_parent->intern->color_message2;
		}
		an_message_manager_indicate_others (m_parent, m_type_id, dummy_fn, dummy_int);
	}
	g_free(dummy_fn);
	
	GtkTreeIter iter;	
	GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(m_tree)));
	gtk_list_store_append(store, &iter);
	
	/*
	Must be normalized to compose representation to be
	displayed correctly (Bug in gtk_list???)
	*/
	gchar* utf8_msg = g_utf8_normalize(c_message.c_str(), -1, 
									   G_NORMALIZE_DEFAULT_COMPOSE);
	gtk_list_store_set (store, &iter,
						COLUMN_MESSAGES, utf8_msg,
						COLUMN_COLOR, &color,
						COLUMN_LINE, m_messages.size() - 1, -1);
	g_free(utf8_msg);
}

int
AnMessageWindow::get_cur_line()
{
	GtkTreeIter iter;
	GtkTreeSelection *select;
	GtkTreeModel *model;
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_tree));
	if (!gtk_tree_selection_get_selected (select, &model, &iter))
		return m_messages.size();
	int line;
	gtk_tree_model_get (model, &iter, COLUMN_LINE, &line, -1);
	return line;
}

string
AnMessageWindow::get_cur_msg()
{
	int line = get_cur_line();
	if (line < 0 || ((unsigned int)line) >= m_messages.size())
		return "";
	return m_messages[line];
}

void
AnMessageWindow::clear()
{
	m_messages.clear();
	GtkListStore* store = GTK_LIST_STORE(
		gtk_tree_view_get_model(GTK_TREE_VIEW(m_tree)));
	gtk_list_store_clear(store);
}

void 
AnMessageWindow::show()
{
	showWidget(m_scrolled_win);
}

void 
AnMessageWindow::hide()
{
	hideWidget(m_scrolled_win);
}

GtkWidget* AnMessageWindow::get_msg_list()
{
	return m_tree;
}

GtkWidget* AnMessageWindow::get_scrolled_win()
{
	return m_scrolled_win;	
}

bool AnMessageWindow::select_next()
{
	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(m_tree));
	GtkTreeSelection* select = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_tree));	
	if (!gtk_tree_selection_get_selected(select, &model, &iter))
	{
		if (!gtk_tree_model_get_iter_first(model, &iter))
			return false;
		gtk_tree_selection_select_iter(select, &iter);
		return true;
	}
	if (gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_selection_select_iter(select, &iter);
		return true;
	}
	return false;
}

bool AnMessageWindow::select_prev()
{
	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(m_tree));
	GtkTreeSelection* select = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_tree));	
	if (!gtk_tree_selection_get_selected(select, &model, &iter))
	{
		if (!gtk_tree_model_get_iter_first(model, &iter))
			return false;
		gtk_tree_selection_select_iter(select, &iter);
		return true;
	}
	GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
	if (gtk_tree_path_prev(path))
	{
		gtk_tree_selection_select_path(select, path);
		gtk_tree_path_free(path);
		return true;
	}
	else
	{
		gtk_tree_path_free(path);
		return false;
	}
}

gboolean AnMessageWindow::on_mesg_event (GtkTreeView* list, GdkEvent * event, gpointer data)
{
	g_return_val_if_fail(data != NULL, FALSE);
	AnMessageWindow *win =
			reinterpret_cast < AnMessageWindow * >(data);
	
	if (event == NULL)
		return FALSE;
		
	if (event->type == GDK_KEY_PRESS)
	{
		switch(((GdkEventKey *)event)->keyval)
		{
			case GDK_space:
			case GDK_Return:
			{
				gchar* message = NULL;
				if (!win->get_messages().empty())
				{
					message = new gchar[win->get_cur_msg().size() + 1];	
					strcpy(message, win->get_cur_msg().c_str());
					on_message_clicked(win->get_parent(), message);
					g_free(message);	
					return TRUE;
				}
				break;
			}
			default:
				return FALSE;
		}
	}
	else if (event->type == GDK_2BUTTON_PRESS) 
	{
		if (((GdkEventButton *) event)->button == 1)
		{
			gchar* message = NULL;
			if (!win->get_messages().empty())
			{
				message = new gchar[win->get_cur_msg().size() + 1];	
				strcpy(message, win->get_cur_msg().c_str());
				on_message_clicked(win->get_parent(), message);
				g_free(message);	
				return TRUE;
			}	
			return FALSE;
		}
	}		
	return FALSE;
}

void 
AnMessageWindow::on_adjustment_changed (GtkAdjustment* adj, gpointer data)
{
	gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

void 
AnMessageWindow::on_adjustment_value_changed (GtkAdjustment* adj, gpointer data)
{
	AnMessageWindow *amw = (AnMessageWindow *) data;
	if (adj->value > (adj->upper - adj->page_size) - 0.1)
	{
		if (!amw->adj_chgd_hdlr)
		{
			amw->adj_chgd_hdlr = g_signal_connect (G_OBJECT (adj), "changed",
				G_CALLBACK (AnMessageWindow::on_adjustment_changed), NULL);
		}
	}
	else
	{
		if (amw->adj_chgd_hdlr)
		{
			g_signal_handler_disconnect (G_OBJECT (adj), amw->adj_chgd_hdlr);
			amw->adj_chgd_hdlr = 0;
		}
	}
}

static const gchar * get_profile_key (const gchar *profile, const gchar *key)
{
	/* A resonably safe buffer */
	static gchar buffer[1024];
	
	g_return_val_if_fail (profile != NULL && key != NULL, NULL);
	
	snprintf (buffer, 1024, "%s/%s/%s", GCONF_PROFILE_PREFIX, profile, key);
	return buffer;
}

#define GET_PROFILE_BOOL(key) \
			gconf_client_get_bool (client, \
								   get_profile_key (profile, key), \
								   NULL);
#define GET_PROFILE_INT(key) \
			gconf_client_get_int (client, \
								  get_profile_key (profile, key), \
								  NULL);
#define GET_PROFILE_STRING(key) \
			gconf_client_get_string (client, \
									 get_profile_key (profile, key), \
									 NULL);

void TerminalWindow::preferences_update ()
{
	GConfClient *client;
	char *text;
	int value;
	gboolean setting;
	GdkColor color;
	GtkWidget *vte = m_terminal;
	gchar *profile;
	
	client = gconf_client_get_default ();
	g_return_if_fail (client != NULL);
	
	/* Update the currently available list of terminal profiles */
	GSList *profiles = gconf_client_get_list (client, GCONF_PROFILE_LIST,
											  GCONF_VALUE_STRING, NULL);
	if (profiles)
	{
		GList *list = NULL;
		GSList *node = profiles;
		while (node)
		{
			if (node->data)
				list = g_list_append (list, node->data);
			node = g_slist_next (node);
		}
		gtk_combo_set_popdown_strings (GTK_COMBO (m_profile_combo), list);
		g_list_free (list);
	}
	AnjutaPreferences *pref = get_preferences();
	setting = anjuta_preferences_get_int (pref, PREFS_TERMINAL_PROFILE_USE_DEFAULT);
	if (setting)
	{
		/* Use the currently selected profile in gnome-terminal */
		text = gconf_client_get_string (client, GCONF_DEFAULT_PROFILE, NULL);
		if (!text)
			text = "Default";
	}
	else
	{
		/* Otherwise use the user selected profile */
		text = anjuta_preferences_get (pref, PREFS_TERMINAL_PROFILE);
		if (!text)
			text = "Default";
	}
	profile = g_strdup (text);
	
	vte_terminal_set_mouse_autohide (VTE_TERMINAL (vte), TRUE);

	/* Set terminal font either using the desktop wide font or g-t one. */
	setting = GET_PROFILE_BOOL (GCONF_USE_SYSTEM_FONT);
	if (setting) {
		text = gconf_client_get_string (client, GCONF_MONOSPACE_FONT, NULL);
		if (!text)
			text = GET_PROFILE_STRING (GCONF_VTE_TERMINAL_FONT);
	} else {
		text = GET_PROFILE_STRING (GCONF_VTE_TERMINAL_FONT);
	}
	if (text)
		vte_terminal_set_font_from_string (VTE_TERMINAL (vte), text);

	setting = GET_PROFILE_BOOL (GCONF_CURSOR_BLINK);
	vte_terminal_set_cursor_blinks (VTE_TERMINAL (vte), setting);
	setting = GET_PROFILE_BOOL (GCONF_SILENT_BELL);
	vte_terminal_set_audible_bell (VTE_TERMINAL (vte), !setting);
	value = GET_PROFILE_INT (GCONF_SCROLLBACK_LINES);
	vte_terminal_set_scrollback_lines (VTE_TERMINAL (vte), value);
	setting = GET_PROFILE_BOOL (GCONF_SCROLL_ON_KEYSTROKE);
	vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (vte), setting);
	setting = GET_PROFILE_BOOL (GCONF_SCROLL_ON_OUTPUT);
	vte_terminal_set_scroll_on_output (VTE_TERMINAL (vte), TRUE);
	text = GET_PROFILE_STRING (GCONF_WORD_CHARS);
	if (text)
		vte_terminal_set_word_chars (VTE_TERMINAL (vte), text);

	text = GET_PROFILE_STRING (GCONF_BACKSPACE_BINDING);
	if (text)
	{
		if (!strcmp (text, "ascii-del"))
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_ASCII_DELETE);
		else if (!strcmp (text, "escape-sequence"))
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_DELETE_SEQUENCE);
		else if (!strcmp (text, "control-h"))
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_ASCII_BACKSPACE);
		else
			vte_terminal_set_backspace_binding (VTE_TERMINAL (vte),
								VTE_ERASE_AUTO);
	}
	text = GET_PROFILE_STRING (GCONF_DELETE_BINDING);
	if (text)
	{
		if (!strcmp (text, "ascii-del"))
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
							 VTE_ERASE_ASCII_DELETE);
		else if (!strcmp (text, "escape-sequence"))
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
							 VTE_ERASE_DELETE_SEQUENCE);
		else if (!strcmp (text, "control-h"))
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
							 VTE_ERASE_ASCII_BACKSPACE);
		else
			vte_terminal_set_delete_binding (VTE_TERMINAL (vte),
							 VTE_ERASE_AUTO);
	}
	/* Set fore- and background colors. */
	text = GET_PROFILE_STRING (GCONF_BACKGROUND_COLOR);
	if (text)
	{
		gdk_color_parse (text, &color);
		vte_terminal_set_color_background (VTE_TERMINAL (vte), &color);
	}
	text = GET_PROFILE_STRING (GCONF_FOREGROUND_COLOR);
	if (text)
	{
		gdk_color_parse (text, &color);
		vte_terminal_set_color_foreground (VTE_TERMINAL (vte), &color);
	}
	g_free (profile);
	g_object_unref (client);
}

void TerminalWindow::use_default_profile_cb (GtkToggleButton *button,
											 TerminalWindow *tw)
{
	if (gtk_toggle_button_get_active (button))
		gtk_widget_set_sensitive (tw->m_profile_combo, FALSE);
	else
		gtk_widget_set_sensitive (tw->m_profile_combo, TRUE);
}

TerminalWindow::TerminalWindow(AnMessageManager* p_amm, int p_type_id,
							   string p_type, string p_pixmap)
	: MessageSubwindow(p_amm, p_type_id, p_type, p_pixmap)
{
	GtkWidget *sb, *frame, *hbox, *button;
	GladeXML *gxml;
	
	g_return_if_fail(p_amm != NULL);

	m_child_pid = 0;
	
	/* Create the terminal preferences page */
	gxml = glade_xml_new (PACKAGE_DATA_DIR"/glade/anjuta.glade",
						  "preferences_dialog_terminal",
						  NULL);
	anjuta_preferences_add_page (get_preferences(), gxml,
								"Terminal",
								"preferences-terminal.png");
	m_profile_combo = glade_xml_get_widget (gxml, "profile_list_combo");
	button =
		glade_xml_get_widget (gxml,
					"preferences_toggle:bool:1:0:terminal.default.profile");
	g_signal_connect (G_OBJECT(button), "toggled",
					  G_CALLBACK (TerminalWindow::use_default_profile_cb),
					  this);
	g_object_unref (gxml);
	
	/* Create new terminal. */
	m_terminal = vte_terminal_new ();
	vte_terminal_set_size (VTE_TERMINAL (m_terminal), 50, 1);
	
	preferences_update ();
	
	g_signal_connect (G_OBJECT (m_terminal), "focus_in_event",
					  G_CALLBACK (TerminalWindow::term_focus_cb), this);
	g_signal_connect (G_OBJECT (m_terminal), "child-exited",
					  G_CALLBACK (TerminalWindow::term_init_cb), this);
	g_signal_connect (G_OBJECT (m_terminal), "destroy",
					  G_CALLBACK (TerminalWindow::term_destroy_cb), this);

	sb = gtk_vscrollbar_new (GTK_ADJUSTMENT (VTE_TERMINAL (m_terminal)->adjustment));
	GTK_WIDGET_UNSET_FLAGS (sb, GTK_CAN_FOCUS);

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start (GTK_BOX (hbox), m_terminal, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), sb, FALSE, TRUE, 0);
	gtk_widget_show_all (frame);
	gtk_notebook_append_page (GTK_NOTEBOOK(p_amm->intern->notebook),
							  frame, create_label());
	
	m_scrollbar = sb;
	m_frame = frame;
	m_hbox = hbox;
}

void TerminalWindow::show()
{
	showWidget(m_frame);
}

void TerminalWindow::hide()
{
	hideWidget(m_frame);
}

extern "C" void an_goto_file_line (gchar * fname, glong lineno);

extern char **environ;

static char **
get_child_environment (GtkWidget *term)
{
	/* code from gnome-terminal, sort of. */
	char **p;
	int i;
	char **retval;
#define EXTRA_ENV_VARS 6

	/* count env vars that are set */
	for (p = environ; *p; p++);

	i = p - environ;
	retval = g_new (char *, i + 1 + EXTRA_ENV_VARS);

	for (i = 0, p = environ; *p; p++) {
		/* Strip all these out, we'll replace some of them */
		if ((strncmp (*p, "COLUMNS=", 8) == 0) ||
		    (strncmp (*p, "LINES=", 6) == 0)   ||
		    (strncmp (*p, "TERM=", 5) == 0)    ||
		    (strncmp (*p, "GNOME_DESKTOP_ICON=", 19) == 0)) {
			/* nothing: do not copy */
		} else {
			retval[i] = g_strdup (*p);
			++i;
		}
	}

	retval[i] = g_strdup ("TERM=xterm"); /* FIXME configurable later? */
	++i;

	retval[i] = NULL;

	return retval;
}

void TerminalWindow::term_init_cb (GtkWidget *widget,
								   TerminalWindow *tw)
{
	VteTerminal *term = VTE_TERMINAL (widget);
	struct passwd *pw;
	const char *shell;
	const char *dir;
	char **env;
	
	vte_terminal_reset (term, TRUE, TRUE);

	pw = getpwuid (getuid ());
	if (pw) {
		shell = pw->pw_shell;
		dir = pw->pw_dir;
	} else {
		shell = "/bin/sh";
		dir = "/";
	}
	
	env = get_child_environment (widget);
#if OLD_VTE == 1
	tw->m_child_pid = vte_terminal_fork_command (term, shell, NULL, env);
#else
	tw->m_child_pid = vte_terminal_fork_command (term, shell, NULL, env, dir, 0, 0, 0);
#endif
	g_strfreev (env);
	tw->preferences_update ();
}

gboolean TerminalWindow::term_focus_cb (GtkWidget *widget,
										GdkEvent  *event,
										TerminalWindow *tw) 
{
	static bool need_init = true;
	if (need_init)
	{
		term_init_cb (widget, tw);
		need_init = false;
	}
	gtk_widget_grab_focus (widget);
	return FALSE;
}

void TerminalWindow::term_destroy_cb (GtkWidget *widget,
									  TerminalWindow *tw)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
					(void*) G_CALLBACK (TerminalWindow::term_init_cb),
					tw);
}


// locals window


LocalsWindow::LocalsWindow (AnMessageManager * p_amm, int p_type_id,
							string p_type, string p_pixmap):
							MessageSubwindow (p_amm, p_type_id,
							p_type, p_pixmap)
{
	g_return_if_fail (p_amm != NULL);

	m_debug_tree = debug_tree_create ();

	m_scrollbar = create_scrolled_window(m_debug_tree->tree);
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
	showWidget(m_scrollbar);
}

void
LocalsWindow::hide ()
{
	hideWidget(m_scrollbar);
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


// widget window

WidgetWindow::WidgetWindow (AnMessageManager * p_amm, int p_type_id,
							string p_type, string p_pixmap):
							MessageSubwindow (p_amm, p_type_id,
							p_type, p_pixmap)
{
}

WidgetWindow::~WidgetWindow ()
{
}


void
WidgetWindow::set_widget(GtkWidget* widget)
{
	m_scrollbar = create_scrolled_window(widget);
}

void
WidgetWindow::show ()
{
	showWidget(m_scrollbar);
}

void
WidgetWindow::hide ()
{
	hideWidget(m_scrollbar);
}
