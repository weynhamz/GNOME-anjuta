/*
    message-manager.cc
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

#include "message-manager.h"
#include "message-manager-private.h"
#include "message-manager-dock.h"
#include "pixmaps.h"
#include "preferences.h"

#include <libgnomeui/gnome-window-icon.h>

extern "C"
{
#include "utilities.h"
};

// SIGNALS

enum
{
	MESSAGE_CLICKED,
	SIGNALS_END
};

static guint anjuta_message_manager_signals[SIGNALS_END] = { 0 };

// Data:
static char *labels[] =
	{ N_("Build"), N_("Debug"), N_("Find"), N_("CVS"), N_("Locals"),
N_("Terminal") };

// Intern functions
static void anjuta_message_manager_destroy (GtkObject * object);
static void anjuta_message_manager_class_init (AnjutaMessageManagerClass *
					       klass);
static void anjuta_message_manager_init (GtkObject * obj);
static void anjuta_message_manager_show_impl (GtkWidget * widget);
static void anjuta_message_manager_hide_impl (GtkWidget * widget);

// Callbacks
static gboolean on_dock_activate (GtkWidget * menuitem,
				  AnjutaMessageManager * amm);
static gboolean on_show_hide_tab (GtkWidget * menuitem,
				  MessageSubwindow * msg_win);
static gboolean on_popup_clicked (AnjutaMessageManager * amm,
				  GdkEvent * event);
static void on_msg_double_clicked (GtkCList * list, gint row, gint column,
				   GdkEvent * event, gpointer msg_win);
static gboolean on_mesg_event (GtkCList *list, GdkEvent * event, gpointer user_data);

// Intern functions:

GtkFrameClass *parent_class;

GtkWidget *
anjuta_message_manager_new ()
{
	GtkWidget *amm;
	amm = GTK_WIDGET (gtk_type_new (anjuta_message_manager_get_type ()));
	return amm;
}

GtkType
anjuta_message_manager_get_type (void)
{
	static GtkType type = 0;

	if (!type)
	{
		static const GtkTypeInfo info = {
			"AnjutaMessageManager",
			sizeof (AnjutaMessageManager),
			sizeof (AnjutaMessageManagerClass),
			(GtkClassInitFunc) anjuta_message_manager_class_init,
			(GtkObjectInitFunc) anjuta_message_manager_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};
		type = gtk_type_unique (gtk_frame_get_type (), &info);
	}
	return type;
}

static void
anjuta_message_manager_class_init (AnjutaMessageManagerClass * klass)
{
	GtkObjectClass *object_class =
		reinterpret_cast < GtkObjectClass * >(klass);

	parent_class = reinterpret_cast < GtkFrameClass * >(klass);
	// Signals
	anjuta_message_manager_signals[MESSAGE_CLICKED] =
		gtk_signal_new ("message_clicked", GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (AnjutaMessageManagerClass,
						   message_clicked),
				gtk_marshal_NONE__POINTER, GTK_TYPE_NONE,
				1, GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class,
				      anjuta_message_manager_signals,
				      SIGNALS_END);

	object_class->destroy = anjuta_message_manager_destroy;
}

static void
anjuta_message_manager_init (GtkObject * obj)
{
	// Init Class
	AnjutaMessageManager *amm = ANJUTA_MESSAGE_MANAGER (obj);
	gtk_widget_set_usize (GTK_WIDGET (amm), 200, 70);

	amm->intern = new AnjutaMessageManagerPrivate;
	amm->intern->last_page = -1;
	amm->intern->cur_msg_win = 0;

	// Create Widgets
	amm->intern->notebook = gtk_notebook_new ();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (amm->intern->notebook),
				  GTK_POS_LEFT);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (amm->intern->notebook),
				     true);
	gtk_widget_show (amm->intern->notebook);

	amm->intern->window = NULL;

	// Create Menu
	static GtkItemFactoryEntry menu_items[] = {
		{"/separator", NULL, NULL, 0, "<Separator>"}
	};
	GtkWidget* dock_item = gtk_check_menu_item_new_with_label(_("Docked"));
	gtk_signal_connect(GTK_OBJECT(dock_item), "activate", GTK_SIGNAL_FUNC(on_dock_activate), amm);
	gtk_widget_show(dock_item);
	
	GtkAccelGroup *agroup = gtk_accel_group_new ();
	GtkItemFactory *factory =
		gtk_item_factory_new (GTK_TYPE_MENU, "<none>", agroup);
	gtk_item_factory_create_items (factory, 1, menu_items, NULL);
	amm->intern->popupmenu =
		gtk_item_factory_get_widget (factory, "<none>");
	gtk_menu_prepend(GTK_MENU(amm->intern->popupmenu), dock_item);
	amm->intern->dock_item = dock_item;
	gtk_widget_ref(amm->intern->dock_item);
	gtk_signal_connect (GTK_OBJECT (amm), "button_press_event",
			    GTK_SIGNAL_FUNC (on_popup_clicked), amm);

	// Add Widgets to UI
	gtk_container_add (GTK_CONTAINER (amm), amm->intern->notebook);

	gtk_signal_connect (GTK_OBJECT (amm), "show",
			    GTK_SIGNAL_FUNC
			    (anjuta_message_manager_show_impl),
			    GTK_WIDGET (amm));
	gtk_signal_connect (GTK_OBJECT (amm), "hide",
			    GTK_SIGNAL_FUNC
			    (anjuta_message_manager_hide_impl),
			    GTK_WIDGET (amm));

	amm->intern->is_docked = false;
	amm->intern->is_shown = false;
}

static void
anjuta_message_manager_destroy (GtkObject * obj)
{
	AnjutaMessageManager *amm = ANJUTA_MESSAGE_MANAGER (obj);
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		delete *cur_win;
	}
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}

// Public:

// Creates a new Notebook page for message of a new type if it not already exists
gboolean
anjuta_message_manager_add_type (AnjutaMessageManager * amm, gint type_name,
				 const gchar * pixmap)
{
	if (!amm || !pixmap)
		return false;
	
	string type = labels[type_name];
	typedef vector < MessageSubwindow * >::const_iterator CI;
	for (CI cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		if ((*cur_win)->get_type () == type)
		{
			return false;
		}
	}

	MessageSubwindow *sub_win;
	if (type_name == MESSAGE_TERMINAL)
	{
		TerminalWindow *window = new TerminalWindow (amm, type_name, type, pixmap);
		sub_win = window;
	}
	else if (type_name == MESSAGE_LOCALS)
	{
	    LocalsWindow* window = new LocalsWindow(amm, type_name, type, pixmap);
	    sub_win = window;
	}
	else 
	{
		AnjutaMessageWindow *window =
			new AnjutaMessageWindow (amm, type_name, type, pixmap);
		gtk_signal_connect (GTK_OBJECT (window->get_msg_list ()),
				    "select_row",
				    GTK_SIGNAL_FUNC (on_msg_double_clicked),
				    window);
		gtk_signal_connect (GTK_OBJECT (window->get_msg_list ()),
				    "event",
				    GTK_SIGNAL_FUNC (on_mesg_event),
				    window);
		sub_win = window;
	}
	amm->intern->msg_windows.push_back (sub_win);
	if (amm->intern->last_page == 0)
		amm->intern->msg_windows.back ()->activate ();
	return true;
}

// Adds a string to a message_window and shows it when a newline appears
gboolean
anjuta_message_manager_append (AnjutaMessageManager * amm,
			       const gchar * msg_string, gint type_name)
{
	if (!amm)
		return false;
	
	string type = labels[type_name];
	string msg = msg_string;
	vector < MessageSubwindow * >::iterator cur_win;

	if (!amm->intern->msg_windows.empty ())
	{
		bool found = false;
		for (cur_win = amm->intern->msg_windows.begin ();
		     cur_win != amm->intern->msg_windows.end (); cur_win++)
		{
			if ((*cur_win)->get_type () == type)
			{
				found = true;
				if (!dynamic_cast < AnjutaMessageWindow * >(*cur_win))
					return false;
				if (!(*cur_win)->is_shown ())
				{
					(*cur_win)->activate ();
				}
				break;
			}
		}
		if (!found)
		{
			g_warning (_("Could not find message type %s!\n"),
				   type.c_str ());
			return false;
		}
	}
	else
	{
		return false;
	}
	AnjutaMessageWindow *window =
		dynamic_cast < AnjutaMessageWindow * >(*cur_win);
	for (string::iterator c = msg.begin (); c != msg.end (); c++)
	{
		if (*c == '\n')
		{
			window->append_buffer ();
		}
		else
		{
			window->add_to_buffer (*c);
		}
	}
	return true;
}

// Returns the subwindow according to the requested type
MessageSubwindow *
anjuta_message_manager_get_window(AnjutaMessageManager * amm,gint type_name)
{
	if (!amm)
	  return NULL;
	
	string type = labels[type_name];
	vector < MessageSubwindow * >::iterator cur_win;

	if (!amm->intern->msg_windows.empty ())
	{
		bool found = false;
		for (cur_win = amm->intern->msg_windows.begin ();
		     cur_win != amm->intern->msg_windows.end (); cur_win++)
		{
			if ((*cur_win)->get_type () == type)
			{
				found = true;
				//				if (!dynamic_cast < AnjutaMessageWindow * >(*cur_win))
				//return NULL;
				if (!(*cur_win)->is_shown ())
				{
					(*cur_win)->activate ();
				}
				break;
			}
		}
		if (!found)
		{
			g_warning (_("Could not find message type %s!\n"),
				   type.c_str ());
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
	//	AnjutaMessageWindow *window =
	//dynamic_cast < AnjutaMessageWindow * >(*cur_win);
	//return window;
	return *cur_win;
}

// Sent a "message_click" event as if the next message was clicked
void
anjuta_message_manager_next (AnjutaMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	AnjutaMessageWindow *win =
		dynamic_cast <
		AnjutaMessageWindow * >(amm->intern->cur_msg_win);
	g_return_if_fail (win != 0);
	// Fix for bug #509192 (Crash on next message)
	if (win->get_messages().empty())
		return;
	if (win->get_cur_line () < (win->get_messages ().size () - 1))
	{
		gchar* file;
		int line;
		if (!parse_error_line(win->get_messages()[win->get_cur_line() + 1].c_str(), &file, &line))
		{
			gtk_clist_select_row (GTK_CLIST (win->get_msg_list ()),
				      win->get_cur_line () + 1, 0);
			anjuta_message_manager_next(amm);
			return;
		}
		else
			delete[] file;		
		
		gtk_signal_emit (GTK_OBJECT (amm),
				 anjuta_message_manager_signals
				 [MESSAGE_CLICKED],
				 win->get_messages ()[win->get_cur_line() + 1].c_str ());
		// this increments win->get_cur_line, so + 1 is not needed after it
		gtk_clist_select_row (GTK_CLIST (win->get_msg_list ()),
				      win->get_cur_line () + 1, 0);
		
		GtkAdjustment* adj = gtk_clist_get_vadjustment (GTK_CLIST(win->get_msg_list ()));
		float value = (adj->upper*win->get_cur_line())/win->get_messages().size() - adj->page_increment/2;
		value = MAX (0, value);
		value = MIN (value, adj->upper - adj->page_increment);
		gtk_adjustment_set_value (adj, value);
	}
}

// Sent a "message_click" event as if the previous message was clicked
void
anjuta_message_manager_previous (AnjutaMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	AnjutaMessageWindow *win =
		dynamic_cast < AnjutaMessageWindow * >(amm->intern->cur_msg_win);
	g_return_if_fail (win != 0);
	// Fix for bug #509192 (Crash on next message)
	if (win->get_messages().empty())
		return;
	if (win->get_cur_line () > 0)
	{
		gchar* file;
		int line;
		if (!parse_error_line(win->get_messages()[win->get_cur_line() - 1].c_str(), &file, &line))
		{
			gtk_clist_select_row (GTK_CLIST (win->get_msg_list ()),
				      win->get_cur_line () - 1, 0);
			anjuta_message_manager_previous(amm);
			return;
		}
		else
			delete[] file;
		
		gtk_signal_emit (GTK_OBJECT (amm),
				 anjuta_message_manager_signals
				 [MESSAGE_CLICKED],
				 win->get_messages ()[win->get_cur_line () - 1].c_str ());
		// see above...
		gtk_clist_select_row (GTK_CLIST (win->get_msg_list ()),
				      win->get_cur_line () - 1, 0);
		
		GtkAdjustment* adj = gtk_clist_get_vadjustment (GTK_CLIST(win->get_msg_list ()));
		float value = (adj->upper*win->get_cur_line())/win->get_messages().size() - adj->page_increment/2;
		value = MAX (0, value);
		value = MIN (value, adj->upper - adj->page_increment);
		gtk_adjustment_set_value (adj, value);
	}
}

void
anjuta_message_manager_show (AnjutaMessageManager * amm, gint type_name)
{
	g_return_if_fail(amm != NULL);
	if (type_name != MESSAGE_NONE)
	{
		for (vector < MessageSubwindow * >::iterator cur_win =
		     amm->intern->msg_windows.begin ();
		     cur_win != amm->intern->msg_windows.end (); cur_win++)
		{
			if ((*cur_win)->get_type () ==
			    string (labels[type_name]))
			{
				(*cur_win)->activate ();
				break;
			}
		}
	}
	if (anjuta_message_manager_is_shown (amm))
	{
		if (!amm->intern->is_docked && amm->intern->window)
			gdk_window_raise(amm->intern->window->window);
		return;
	}
	if (amm->intern->is_docked)
	{
		amm_show_docked ();
		gtk_widget_show (GTK_WIDGET (amm));
	}
	else
	{
		if (amm->intern->window == 0)
		{
			amm->intern->is_docked = true;
			anjuta_message_manager_undock (amm);
		}
		gtk_widget_show_all(amm->intern->window);
	}
}

void
anjuta_message_manager_info_locals (AnjutaMessageManager * amm, GList * lines,
				    gpointer data)
{
	g_return_if_fail (amm != NULL);
	if (g_list_length (lines) < 1)
		return;
	MessageSubwindow* window = anjuta_message_manager_get_window(amm,MESSAGE_LOCALS);
	if (!window) 
	{
		// g_print ("anjuta_message_manager_get_window error\n");
		return;
	}
	LocalsWindow* locals_window = dynamic_cast<LocalsWindow*>(window);
	if (!locals_window)
	{
		// g_print ("dynamic cast to locals window error\n");
		return;
	}
	locals_window->update_view(lines);
}

void
anjuta_message_manager_clear (AnjutaMessageManager * amm, gint type_name)
{
	g_return_if_fail(amm != NULL);
	
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		if ((*cur_win)->get_type () == string (labels[type_name]))
		{
			(*cur_win)->clear();
			/*AnjutaMessageWindow *win = dynamic_cast < AnjutaMessageWindow * >(*cur_win);
			if (win != 0)
				win->clear ();
				win->clear ();*/
			break;
		}
	}
}

void
anjuta_message_manager_dock (AnjutaMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	
	if (!amm->intern->is_docked)
	{
		bool init = (amm->intern->window == NULL);
		amm_dock (GTK_WIDGET (amm), &amm->intern->window);
		amm->intern->is_docked = true;
		if (!init) ;
		{
			amm_show_docked ();
			gtk_widget_show (GTK_WIDGET (amm));
		}
	}
}

void
anjuta_message_manager_undock (AnjutaMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	
	if (amm->intern->is_docked)
	{
		amm->intern->is_docked = false;
		
		amm->intern->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gnome_window_icon_set_from_default((GtkWindow *) amm->intern->window);
		gtk_window_set_wmclass(GTK_WINDOW(amm->intern->window), "message-manager", "anjuta");
		gtk_window_set_title(GTK_WINDOW(amm->intern->window), _("Messages"));
		gtk_window_set_default_size(GTK_WINDOW(amm->intern->window), amm->intern->width, amm->intern->height);
		gtk_widget_set_uposition(amm->intern->window, amm->intern->xpos, amm->intern->ypos);
		
		amm_undock (GTK_WIDGET (amm), &amm->intern->window);
		if (amm->intern->is_shown)
			gtk_widget_show (GTK_WIDGET (amm->intern->window));
	}
}

gboolean
anjuta_message_manager_is_shown (AnjutaMessageManager * amm)
{
	if (!amm)
		return false;
	return amm->intern->is_shown;
}

gboolean
anjuta_message_manager_save_yourself (AnjutaMessageManager * amm,
				      FILE * stream)
{
	if (!amm)
		return false;
	
	fprintf (stream, "messages.is.docked=%d\n", amm->intern->is_docked);
	if (amm->intern->is_shown && !amm->intern->is_docked)
	{
		gdk_window_get_root_origin (amm->intern->window->window,
					    &amm->intern->xpos,
					    &amm->intern->ypos);
		gdk_window_get_size (amm->intern->window->window,
				     &amm->intern->width,
				     &amm->intern->height);
	}
	fprintf (stream, "messages.win.pos.x=%d\n", amm->intern->xpos);
	fprintf (stream, "messages.win.pos.y=%d\n", amm->intern->ypos);
	fprintf (stream, "messages.win.width=%d\n", amm->intern->width);
	fprintf (stream, "messages.win.height=%d\n", amm->intern->height);
	
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		fprintf(stream, "messages.show.%d=%d\n", (*cur_win)->get_type_id(),
			(*cur_win)->is_shown());
	}

	
	return true;
}

gboolean
anjuta_message_manager_load_yourself (AnjutaMessageManager * amm,
				      PropsID props)
{
	if (!amm)
		return false;
	bool dock_flag;
	if (!amm)
		return false;

	dock_flag = prop_get_int (props, "messages.is.docked", 0);
	amm->intern->xpos = prop_get_int (props, "messages.win.pos.x", 50);
	amm->intern->ypos = prop_get_int (props, "messages.win.pos.y", 50);
	amm->intern->width = prop_get_int (props, "messages.win.width", 600);
	amm->intern->height =
		prop_get_int (props, "messages.win.height", 300);
	
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		char* str = new char[sizeof("messages.tab.") + 3];
		sprintf(str, "messages.show.%d", (*cur_win)->get_type_id());
		int show = prop_get_int (props, str, 1);
		if (!show)
		{
			(*cur_win)->hide();
			(*cur_win)->set_check_item(false);
		}
		delete[] str;
	}
	
	anjuta_message_manager_update(amm);
	
	amm_hide_docked ();
	if (!dock_flag)
		anjuta_message_manager_undock (amm);
	else
		amm->intern->is_docked = true;
	return true;
}

void
anjuta_message_manager_update(AnjutaMessageManager* amm)
{
	g_return_if_fail(amm != NULL);
	
	Preferences* p = get_preferences();
	
	char* tag_pos = preferences_get(p, MESSAGES_TAG_POS);
	if (tag_pos != NULL)
	{
		if (strcmp(tag_pos, "top")==0)
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_TOP);
		else if (strcmp(tag_pos, "left")==0)
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_LEFT);
		if (strcmp(tag_pos, "bottom")==0)
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_BOTTOM);
		if (strcmp(tag_pos, "right")==0)
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_RIGHT);
		g_free(tag_pos);
	}
	
	guint8 r, g, b;
	guint factor = ((guint16) -1) / ((guint8) -1);
	char* color;
	color = preferences_get(p, MESSAGES_COLOR_ERROR);
	if (color)
	{
		ColorFromString(color, &r, &g, &b);
		amm->intern->color_error.red = r * factor;
		amm->intern->color_error.green = g * factor;
		amm->intern->color_error.blue = b * factor;
		g_free(color);
	}
	
	color = preferences_get(p, MESSAGES_COLOR_WARNING);
	if (color)
	{
		ColorFromString(color, &r, &g, &b);
		amm->intern->color_warning.red = r * factor;
		amm->intern->color_warning.green = g * factor;
		amm->intern->color_warning.blue = b * factor;
		g_free(color);
	}
	color = preferences_get(p, MESSAGES_COLOR_MESSAGES1);
	if (color)
	{
		ColorFromString(color, &r, &g, &b);
		amm->intern->color_message1.red = r * factor;
		amm->intern->color_message1.green = g * factor;
		amm->intern->color_message1.blue = b * factor;
		g_free(color);
	}
	color = preferences_get(p, MESSAGES_COLOR_MESSAGES2);
	if (color)
	{
		ColorFromString(color, &r, &g, &b);
		amm->intern->color_message2.red = r * factor;
		amm->intern->color_message2.green = g * factor;
		amm->intern->color_message2.blue = b * factor;
		g_free(color);
	}
	
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		AnjutaMessageWindow* win = dynamic_cast<AnjutaMessageWindow*>(*cur_win);
		if (!win)
			break;
		
		vector<string> messages = win->get_messages();
		win->freeze();
		win->clear();
		int type = win->get_type_id();
		
		typedef vector<string>::const_iterator CI;
		for (CI cur_msg = messages.begin(); cur_msg != messages.end(); cur_msg++)
		{
			string msg = *cur_msg + "\n";
			anjuta_message_manager_append(amm, msg.c_str(), type);
		}
		win->thaw();
	}
}

gboolean
anjuta_message_manager_save_build (AnjutaMessageManager* amm, FILE * stream)
{
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		AnjutaMessageWindow* win = dynamic_cast<AnjutaMessageWindow*>(*cur_win);
		if (!win)
			break;
		if ((*cur_win)->get_type_id() == MESSAGE_BUILD)
		{
			vector<string> messages = win->get_messages();
			typedef vector<string>::const_iterator CI;
			for (CI cur_msg = messages.begin(); cur_msg != messages.end(); cur_msg++)
			{
				string msg = *cur_msg + "\n";
				fprintf(stream, msg.c_str());
			}
		}
	}
	return true;
}

gboolean
anjuta_message_manager_build_is_empty(AnjutaMessageManager* amm)
{
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		AnjutaMessageWindow* win = dynamic_cast<AnjutaMessageWindow*>(*cur_win);
		if (!win)
			break;
		if ((*cur_win)->get_type_id() == MESSAGE_BUILD)
		{
			return win->get_messages().empty();
		}
	}
	return true;
}
		
// Private:

// Callbacks:

static void
anjuta_message_manager_show_impl (GtkWidget * widget)
{
	AnjutaMessageManager *amm = ANJUTA_MESSAGE_MANAGER (widget);
	g_return_if_fail  (ANJUTA_IS_MESSAGE_MANAGER(amm));
	
	if (!amm->intern->is_docked)
	{
		gtk_widget_set_uposition (amm->intern->window,
					  amm->intern->xpos,
					  amm->intern->ypos);
		gtk_window_set_default_size (GTK_WINDOW (amm->intern->window),
					     amm->intern->width,
					     amm->intern->height);
		gtk_widget_show(amm->intern->window);
	}
	amm->intern->is_shown = true;
}

static void
anjuta_message_manager_hide_impl (GtkWidget * widget)
{
	AnjutaMessageManager *amm = ANJUTA_MESSAGE_MANAGER (widget);
	if (amm->intern->is_docked)
	{
		amm_hide_docked ();
	}
	else
	{
		gdk_window_get_root_origin (amm->intern->window->window,
					    &amm->intern->xpos,
					    &amm->intern->ypos);
		gdk_window_get_size (amm->intern->window->window,
				     &amm->intern->width,
				     &amm->intern->height);
		gtk_widget_hide (amm->intern->window);
	}
	amm->intern->is_shown = false;
}

static gboolean
on_dock_activate (GtkWidget * menuitem, AnjutaMessageManager * amm)
{
	if (!amm)
		return false;
	
	if (amm->intern->is_docked)
	{
		anjuta_message_manager_undock (amm);
	}
	else
	{
		anjuta_message_manager_dock (amm);
	}
	return true;
}

static gboolean
on_popup_clicked (AnjutaMessageManager * amm, GdkEvent * event)
{
	if (!amm || !event)
		return FALSE;
	
	if (event->type == GDK_BUTTON_PRESS
	    && ((GdkEventButton *) event)->button == 3)
	{
		GTK_CHECK_MENU_ITEM(amm->intern->dock_item)->active = amm->intern->is_docked;
		gtk_menu_popup (GTK_MENU (amm->intern->popupmenu), NULL, NULL,
				NULL, NULL,
				((GdkEventButton *) event)->button,
				((GdkEventButton *) event)->time);
		return TRUE;
	}
	return FALSE;
}

static gboolean
on_show_hide_tab (GtkWidget * menuitem, MessageSubwindow * msg_win)
{
	if (menuitem == NULL)
		return false;
	AnjutaMessageManager *amm = msg_win->get_parent ();
	if (msg_win->is_shown ())
	{
		msg_win->hide ();
	}
	else
	{
		msg_win->show ();
	}

	// reorder windows
	vector < bool > win_is_shown;

	for (uint i = 0; i < amm->intern->msg_windows.size (); i++)
	{
		win_is_shown.push_back (amm->intern->msg_windows[i]->
					is_shown ());
		amm->intern->msg_windows[i]->hide ();
	}
	for (uint i = 0; i < amm->intern->msg_windows.size (); i++)
	{
		if (win_is_shown[i])
			amm->intern->msg_windows[i]->show ();
	}
	return true;
}

// Called when a message in the list is double clicked. Emits a the "message_clicked" signal
void
on_msg_double_clicked (GtkCList * list, gint row, gint column,
		       GdkEvent * event, gpointer data)
{
	g_return_if_fail(data != NULL);
	AnjutaMessageWindow *win =
			reinterpret_cast < AnjutaMessageWindow * >(data);
	win->set_cur_line (row);
	
	if (event == NULL)
		return;
	
	if (event->type == GDK_2BUTTON_PRESS) {
		if (((GdkEventButton *) event)->button == 1) {
			gtk_signal_emit (GTK_OBJECT (win->get_parent ()),
				 anjuta_message_manager_signals[MESSAGE_CLICKED],
					win->get_messages()[row].c_str());
		}
	}
}

gboolean
on_mesg_event (GtkCList *list, GdkEvent * event, gpointer data)
{
	g_return_val_if_fail(data != NULL, FALSE);
	AnjutaMessageWindow *win =
			reinterpret_cast < AnjutaMessageWindow * >(data);
	
	if (event == NULL)
		return FALSE;
	
	if (event->type == GDK_KEY_PRESS){
		switch(((GdkEventKey *)event)->keyval) {
			case GDK_space:
			case GDK_Return:
				gtk_signal_emit (GTK_OBJECT (win->get_parent ()),
				 anjuta_message_manager_signals[MESSAGE_CLICKED],
					win->get_messages()[win->get_cur_line()].c_str());
				return TRUE;
				break;
			default:
				return FALSE;
		}
	}
	return FALSE;
}

// Utilities:

void
create_default_types (AnjutaMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	anjuta_message_manager_add_type (amm, MESSAGE_BUILD,
					 ANJUTA_PIXMAP_MINI_BUILD);
	anjuta_message_manager_add_type (amm, MESSAGE_DEBUG,
					 ANJUTA_PIXMAP_MINI_DEBUG);
	anjuta_message_manager_add_type (amm, MESSAGE_FIND,
					 ANJUTA_PIXMAP_MINI_FIND);
	anjuta_message_manager_add_type (amm, MESSAGE_CVS,
					 ANJUTA_PIXMAP_MINI_CVS);
	anjuta_message_manager_add_type (amm, MESSAGE_LOCALS,
					 ANJUTA_PIXMAP_MINI_LOCALS);
	anjuta_message_manager_add_type (amm, MESSAGE_TERMINAL,
					 ANJUTA_PIXMAP_MINI_TERMINAL);
	
	// Fix for bug #509192 (Crash on next message)
	amm->intern->cur_msg_win = 
			dynamic_cast<AnjutaMessageWindow*>(*(amm->intern->msg_windows.begin()));
}

void
connect_menuitem_signal (GtkWidget * item, MessageSubwindow * msg_win)
{
	gtk_signal_connect (GTK_OBJECT (item), "toggled",
			    GTK_SIGNAL_FUNC (on_show_hide_tab), msg_win);
}

void
disconnect_menuitem_signal (GtkWidget * item, MessageSubwindow * msg_win)
{
	gtk_signal_disconnect_by_func (GTK_OBJECT (item),
				       GTK_SIGNAL_FUNC (on_show_hide_tab),
				       msg_win);
}
