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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <assert.h>
#include <glib-object.h>
#include <gnome.h>

#include "message-manager.h"
#include "message-manager-private.h"
#include "message-manager-dock.h"
#include "pixmaps.h"
#include "preferences.h"

extern "C"
{
#include "anjuta.h"
#include "utilities.h"
};

// Data:
static char *labels[] =
	{ N_("Build"), N_("Debug"), N_("Find"), N_("CVS"), N_("Locals"), N_("Watches"), N_("Stack"),
N_("Terminal"), N_("Stdout"), N_("Stderr") };

// Intern functions
static void an_message_manager_destroy (GtkObject * object);
static void an_message_manager_class_init (AnMessageManagerClass *
					       klass);
static void an_message_manager_init (GtkObject * obj);
static void an_message_manager_show_impl (GtkWidget * widget);
static void an_message_manager_hide_impl (GtkWidget * widget);

// Callbacks
static gboolean on_dock_activate (GtkWidget * menuitem,
				  AnMessageManager * amm);
static gboolean on_show_hide_tab (GtkWidget * menuitem,
				  MessageSubwindow * msg_win);
static gboolean
on_popup_clicked (GtkWidget* widget, GdkEvent * event, 
	AnMessageManagerPrivate* intern);
// Intern functions:

GtkFrameClass *parent_class;

GtkWidget *
an_message_manager_new ()
{
	GtkWidget *amm;
	amm = GTK_WIDGET (g_object_new (AN_MESSAGE_MANAGER_TYPE, NULL));
	return amm;
}

guint
an_message_manager_get_type (void)
{
	static guint type = 0;

	if (!type)
	{
		static const GTypeInfo info = 
		{
			sizeof (AnMessageManagerClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) an_message_manager_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (AnMessageManager),
			0,              /* n_preallocs */
			(GInstanceInitFunc) an_message_manager_init,
			NULL            /* value_table */
		};
		type = g_type_register_static (GTK_TYPE_FRAME,
									   "AnMessageManager",
									   &info, (GTypeFlags)0);
	}
	return type;
}

static void
an_message_manager_class_init (AnMessageManagerClass * klass)
{
	GtkObjectClass *object_class =
		reinterpret_cast < GtkObjectClass * >(klass);

	parent_class = reinterpret_cast < GtkFrameClass * >(klass);
	object_class->destroy = an_message_manager_destroy;
}

static void
an_message_manager_init (GtkObject * obj)
{
	// Init Class
	AnMessageManager *amm = AN_MESSAGE_MANAGER (obj);

	amm->intern = new AnMessageManagerPrivate;
	amm->intern->last_page = -1;
	amm->intern->cur_msg_win = 0;

	// Create Widgets
	amm->intern->notebook = gtk_notebook_new ();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (amm->intern->notebook),
				  			  GTK_POS_BOTTOM);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (amm->intern->notebook),
				     			 true);
	gtk_widget_show (amm->intern->notebook);

	amm->intern->window = NULL;

	// Create Menu
	static GtkItemFactoryEntry menu_items[] = {
		{"/separator", NULL, NULL, 0, "<Separator>"}
	};
	GtkWidget* dock_item = gtk_check_menu_item_new_with_label(_("Docked"));
	g_signal_connect(GTK_OBJECT(dock_item), "activate",
					   GTK_SIGNAL_FUNC(on_dock_activate), amm);
	gtk_widget_show(dock_item);
	
	GtkAccelGroup *agroup = gtk_accel_group_new ();
	GtkItemFactory *factory =
		gtk_item_factory_new (GTK_TYPE_MENU, "<none>", agroup);
	gtk_item_factory_create_items (factory, 1, menu_items, NULL);
	amm->intern->popupmenu =
		gtk_item_factory_get_widget (factory, "<none>");
	gtk_menu_shell_prepend(GTK_MENU_SHELL(amm->intern->popupmenu), dock_item);
	amm->intern->dock_item = dock_item;
	g_object_ref(G_OBJECT(amm->intern->dock_item));
	g_signal_connect (GTK_WIDGET (amm), "button_press_event",
			    GTK_SIGNAL_FUNC (on_popup_clicked), amm->intern);

	// Add Widgets to UI
	gtk_container_add (GTK_CONTAINER (amm), amm->intern->notebook);

	g_signal_connect (GTK_OBJECT (amm), "show",
			    GTK_SIGNAL_FUNC
			    (an_message_manager_show_impl),
			    GTK_WIDGET (amm));
	g_signal_connect (GTK_OBJECT (amm), "hide",
			    GTK_SIGNAL_FUNC
			    (an_message_manager_hide_impl),
			    GTK_WIDGET (amm));

	amm->intern->is_docked = false;
	amm->intern->is_shown = false;
}

static void
an_message_manager_destroy (GtkObject * obj)
{
	AnMessageManager *amm = AN_MESSAGE_MANAGER (obj);
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
an_message_manager_add_type (AnMessageManager * amm, gint type_name,
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
	switch(type_name) {
		case MESSAGE_TERMINAL:
			sub_win = new TerminalWindow (amm, type_name, type, pixmap);
			break;
		case MESSAGE_LOCALS:
		    sub_win = new LocalsWindow(amm, type_name, type, pixmap);
			break;
		case MESSAGE_WATCHES:
		case MESSAGE_STACK:
			sub_win = new WidgetWindow(amm, type_name, type, pixmap);
			break;
		default:
			sub_win = new AnMessageWindow (amm, type_name, type, pixmap);
			g_signal_connect (GTK_OBJECT (
				dynamic_cast<AnMessageWindow*>(sub_win)->get_msg_list()),
				"button_press_event", GTK_SIGNAL_FUNC (on_popup_clicked), 
				sub_win->get_parent()->intern);
		}
	amm->intern->msg_windows.push_back (sub_win);
	if (amm->intern->last_page == 0)
		amm->intern->msg_windows.back ()->activate ();
	return true;
}

// Adds a string to a message_window and shows it when a newline appears
gboolean
an_message_manager_append (AnMessageManager * amm,
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
				if (!dynamic_cast < AnMessageWindow * >(*cur_win))
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
	AnMessageWindow *window =
		dynamic_cast < AnMessageWindow * >(*cur_win);
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
an_message_manager_get_window(AnMessageManager * amm,gint type_name)
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
				//				if (!dynamic_cast < AnMessageWindow * >(*cur_win))
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
	return *cur_win;
}

// Sent a "message_click" event as if the next message was clicked
void
an_message_manager_next (AnMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	AnMessageWindow *win =
		dynamic_cast <
		AnMessageWindow * >(amm->intern->cur_msg_win);
	g_return_if_fail (win != 0);
	// Fix for bug #509192 (Crash on next message)
	if (win->get_messages().empty())
		return;
	
	gchar* file;
	int line;
	win->select_next();
	while (!parse_error_line(win->get_cur_msg().c_str(), &file, &line))
	{
		if (!win->select_next())			
			return;
		delete[] file;		
	}
	on_message_clicked (amm,
			 win->get_cur_msg().c_str());
	
	GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment (
		GTK_SCROLLED_WINDOW(win->get_scrolled_win ()));
	float value = (adj->upper*win->get_cur_line())/win->get_messages().size() - adj->page_increment/2;
	value = MAX (0, value);
	value = MIN (value, adj->upper - adj->page_increment);
	gtk_adjustment_set_value (adj, value);
}

// Sent a "message_click" event as if the previous message was clicked
void
an_message_manager_previous (AnMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	AnMessageWindow *win =
		dynamic_cast < AnMessageWindow * >(amm->intern->cur_msg_win);
	g_return_if_fail (win != 0);
	// Fix for bug #509192 (Crash on next message)
	if (win->get_messages().empty())
		return;
	gchar* file;
	int line;
	win->select_prev();
	while (!parse_error_line(win->get_cur_msg().c_str(), &file, &line))
	{
		if (!win->select_prev())
			return;		
		delete[] file;
	}
		
	on_message_clicked (amm,
			 win->get_cur_msg().c_str());
	GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment (
		GTK_SCROLLED_WINDOW(win->get_scrolled_win ()));
	float value = (adj->upper*win->get_cur_line())/win->get_messages().size() - adj->page_increment/2;
	value = MAX (0, value);
	value = MIN (value, adj->upper - adj->page_increment);
	gtk_adjustment_set_value (adj, value);
}

void
an_message_manager_indicate_error (AnMessageManager * amm, gint type_name,
		gchar* file, glong line)
{
	static MessageIndicatorInfo info;
	if (type_name) return; // Only for Build messages.
	info.type = type_name;
	info.filename = file;
	info.line = line;
	info.message_type = MESSAGE_INDICATOR_ERROR;
	on_message_indicate(amm, &info);
}

void
an_message_manager_indicate_warning (AnMessageManager * amm, gint type_name,
		gchar* file, glong line)
{
	static MessageIndicatorInfo info;
	if (type_name) return; // Only for Build messages.
	info.type = type_name;
	info.filename = file;
	info.line = line;
	info.message_type = MESSAGE_INDICATOR_WARNING;
	on_message_indicate(amm, &info);
}

void
an_message_manager_indicate_others (AnMessageManager * amm, gint type_name,
		gchar* file, glong line)
{
	static MessageIndicatorInfo info;
	info.type = type_name;
	info.filename = file;
	info.line = line;
	info.message_type = MESSAGE_INDICATOR_OTHERS;
	on_message_indicate(amm, &info);
}

void
an_message_manager_show (AnMessageManager * amm, gint type_name)
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
	if (an_message_manager_is_shown (amm))
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
			an_message_manager_undock (amm);
		}
		gtk_widget_show_all(amm->intern->window);
	}
}

void
an_message_manager_info_locals (AnMessageManager * amm, GList * lines,
				    gpointer data)
{
	g_return_if_fail (amm != NULL);
	if (g_list_length (lines) < 1)
		return;
	MessageSubwindow* window = an_message_manager_get_window(amm,MESSAGE_LOCALS);
	if (!window) 
	{
		// g_print ("an_message_manager_get_window error\n");
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
an_message_manager_clear (AnMessageManager * amm, gint type_name)
{
	g_return_if_fail(amm != NULL);
	
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		if ((*cur_win)->get_type () == string (labels[type_name]))
		{
			(*cur_win)->clear();
			if (type_name == 0) {
				anjuta_delete_all_indicators();
			}
			break;
		}
	}
}

void
an_message_manager_dock (AnMessageManager * amm)
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
an_message_manager_undock (AnMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	
	if (amm->intern->is_docked)
	{
		amm->intern->is_docked = false;
		
		amm->intern->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_transient_for(GTK_WINDOW(amm->intern->window), GTK_WINDOW(app->widgets.window));
		gnome_window_icon_set_from_default((GtkWindow *) amm->intern->window);
		gtk_window_set_wmclass(GTK_WINDOW(amm->intern->window), "message-manager", "an");
		gtk_window_set_title(GTK_WINDOW(amm->intern->window), _("Messages"));
		gtk_window_set_default_size(GTK_WINDOW(amm->intern->window), amm->intern->width, amm->intern->height);
		#warning G2 Port: uposition is deprecated but how to replace it?
		//gtk_widget_set_uposition(amm->intern->window, amm->intern->xpos, amm->intern->ypos);
		
		amm_undock (GTK_WIDGET (amm), &amm->intern->window);
		if (amm->intern->is_shown)
			gtk_widget_show (GTK_WIDGET (amm->intern->window));
	}
}

gboolean
an_message_manager_is_shown (AnMessageManager * amm)
{
	if (!amm)
		return false;
	return amm->intern->is_shown;
}

gboolean
an_message_manager_save_yourself (AnMessageManager * amm,
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
		gtk_window_get_size (GTK_WINDOW(amm->intern->window),
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
an_message_manager_load_yourself (AnMessageManager * amm,
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
	
	an_message_manager_update(amm);
	
	amm_hide_docked ();
	if (!dock_flag)
		an_message_manager_undock (amm);
	else
		amm->intern->is_docked = true;
	return true;
}

void
an_message_manager_update(AnMessageManager* amm)
{
	g_return_if_fail(amm != NULL);
	
	AnjutaPreferences* p = get_preferences();
	
	char* tag_pos = anjuta_preferences_get(p, MESSAGES_TAG_POS);
	if (tag_pos != NULL)
	{
		if (strcasecmp(tag_pos, "top")==0)
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_TOP);
		else if (strcasecmp(tag_pos, "left")==0)
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_LEFT);
		else if (strcasecmp(tag_pos, "right")==0)
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_RIGHT);
		else
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(amm->intern->notebook), GTK_POS_BOTTOM);
		
		g_free(tag_pos);
	}
	
	guint8 r, g, b;
	guint factor = ((guint16) -1) / ((guint8) -1);
	char* color;
	color = anjuta_preferences_get(p, MESSAGES_COLOR_ERROR);
	if (color)
	{
		anjuta_util_color_from_string (color, &r, &g, &b);
		amm->intern->color_error.pixel = 0;
		amm->intern->color_error.red = r * factor;
		amm->intern->color_error.green = g * factor;
		amm->intern->color_error.blue = b * factor;
		g_free(color);
	}
	
	color = anjuta_preferences_get (p, MESSAGES_COLOR_WARNING);
	if (color)
	{
		anjuta_util_color_from_string (color, &r, &g, &b);
		amm->intern->color_warning.pixel = 0;
		amm->intern->color_warning.red = r * factor;
		amm->intern->color_warning.green = g * factor;
		amm->intern->color_warning.blue = b * factor;
		g_free(color);
	}
	color = anjuta_preferences_get(p, MESSAGES_COLOR_MESSAGES1);
	if (color)
	{
		anjuta_util_color_from_string (color, &r, &g, &b);
		amm->intern->color_message1.pixel = 0;
		amm->intern->color_message1.red = r * factor;
		amm->intern->color_message1.green = g * factor;
		amm->intern->color_message1.blue = b * factor;
		g_free(color);
	}
	color = anjuta_preferences_get(p, MESSAGES_COLOR_MESSAGES2);
	if (color)
	{
		anjuta_util_color_from_string (color, &r, &g, &b);
		amm->intern->color_message2.pixel = 0;
		amm->intern->color_message2.red = r * factor;
		amm->intern->color_message2.green = g * factor;
		amm->intern->color_message2.blue = b * factor;
		g_free(color);
	}

	anjuta_delete_all_indicators();
	
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		AnMessageWindow* win = dynamic_cast<AnMessageWindow*>(*cur_win);
		if (!win)
			break;
		
		vector<string> messages = win->get_messages();
		win->clear();
		int type = win->get_type_id();
		
		typedef vector<string>::const_iterator CI;
		for (CI cur_msg = messages.begin(); cur_msg != messages.end(); cur_msg++)
		{
			string msg = *cur_msg + "\n";
			an_message_manager_append(amm, msg.c_str(), type);
		}
	}
}

gboolean
an_message_manager_save_build (AnMessageManager* amm, FILE * stream)
{
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		AnMessageWindow* win = dynamic_cast<AnMessageWindow*>(*cur_win);
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
an_message_manager_build_is_empty(AnMessageManager* amm)
{
	typedef vector < MessageSubwindow * >::iterator I;
	for (I cur_win = amm->intern->msg_windows.begin ();
	     cur_win != amm->intern->msg_windows.end (); cur_win++)
	{
		AnMessageWindow* win = dynamic_cast<AnMessageWindow*>(*cur_win);
		if (!win)
			break;
		if ((*cur_win)->get_type_id() == MESSAGE_BUILD)
		{
			return win->get_messages().empty();
		}
	}
	return true;
}
		

void
an_message_manager_set_widget(AnMessageManager* amm, gint type_name, GtkWidget* widget)
{
	MessageSubwindow* msb = NULL;
	WidgetWindow* ww = NULL;

	msb = an_message_manager_get_window(amm, type_name);
	
	if (!msb) 
	{
		g_warning("No window type: %d\n",type_name);
		return;
	}
	
	ww = dynamic_cast<WidgetWindow*>(msb);
	
	if (!ww)
	{
		g_warning("Cannot case to widget window");
		return;
	}
	
	ww->set_widget(widget);
}

// Private:

// Callbacks:

static void
an_message_manager_show_impl (GtkWidget * widget)
{
	AnMessageManager *amm = AN_MESSAGE_MANAGER (widget);
	g_return_if_fail  (AN_IS_MESSAGE_MANAGER(amm));
	
	if (!amm->intern->is_docked)
	{
		#warning G2 second time upostion
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
an_message_manager_hide_impl (GtkWidget * widget)
{
	AnMessageManager *amm = AN_MESSAGE_MANAGER (widget);
	if (amm->intern->is_docked)
	{
		amm_hide_docked ();
	}
	else
	{
		gdk_window_get_root_origin (amm->intern->window->window,
					    &amm->intern->xpos,
					    &amm->intern->ypos);
		gtk_window_get_size (GTK_WINDOW(amm->intern->window),
				     &amm->intern->width,
				     &amm->intern->height);
		gtk_widget_hide (amm->intern->window);
	}
	amm->intern->is_shown = false;
}

static gboolean
on_dock_activate (GtkWidget * menuitem, AnMessageManager * amm)
{
	if (!amm)
		return false;
	
	if (amm->intern->is_docked)
	{
		an_message_manager_undock (amm);
	}
	else
	{
		an_message_manager_dock (amm);
	}
	return true;
}

static gboolean
on_popup_clicked (GtkWidget* widget, GdkEvent * event, 
	AnMessageManagerPrivate* intern)
{
	if (!intern || !event)
		return FALSE;
	
	if (event->type == GDK_BUTTON_PRESS
	    && ((GdkEventButton *) event)->button == 3)
	{
		GTK_CHECK_MENU_ITEM(intern->dock_item)->active = intern->is_docked;
		gtk_menu_popup (GTK_MENU (intern->popupmenu), NULL, NULL,
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
	AnMessageManager *amm = msg_win->get_parent ();
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

// Utilities:

void
create_default_types (AnMessageManager * amm)
{
	g_return_if_fail(amm != NULL);
	an_message_manager_add_type (amm, MESSAGE_BUILD,
					 ANJUTA_PIXMAP_MINI_BUILD);
	an_message_manager_add_type (amm, MESSAGE_DEBUG,
					 ANJUTA_PIXMAP_MINI_DEBUG);
	an_message_manager_add_type (amm, MESSAGE_FIND,
					 ANJUTA_PIXMAP_MINI_FIND);
	an_message_manager_add_type (amm, MESSAGE_CVS,
					 ANJUTA_PIXMAP_MINI_CVS);
	an_message_manager_add_type (amm, MESSAGE_LOCALS,
					 ANJUTA_PIXMAP_MINI_LOCALS);
	an_message_manager_add_type (amm, MESSAGE_WATCHES,
					 ANJUTA_PIXMAP_MINI_LOCALS);	
	an_message_manager_add_type (amm, MESSAGE_STACK,
					 ANJUTA_PIXMAP_MINI_LOCALS);	
	an_message_manager_add_type (amm, MESSAGE_TERMINAL,
					 ANJUTA_PIXMAP_MINI_TERMINAL);
	an_message_manager_add_type (amm, MESSAGE_STDOUT,
					 ANJUTA_PIXMAP_MINI_TERMINAL);
	an_message_manager_add_type (amm, MESSAGE_STDERR,
					 ANJUTA_PIXMAP_MINI_TERMINAL);
	
	// Fix for bug #509192 (Crash on next message)
	amm->intern->cur_msg_win = 
			dynamic_cast<AnMessageWindow*>(*(amm->intern->msg_windows.begin()));
}

void
connect_menuitem_signal (GtkWidget * item, MessageSubwindow * msg_win)
{
	g_signal_connect (G_OBJECT (item), "toggled",
			    G_CALLBACK (on_show_hide_tab), msg_win);
}

void
disconnect_menuitem_signal (GtkWidget * item, MessageSubwindow * msg_win)
{
	g_signal_stop_emission_by_name (G_OBJECT (item), "toggled");
}

void
on_message_clicked(AnMessageManager* amm, const char* message)
{
	char* fn;
	int ln;
	if (parse_error_line (message, &fn, &ln))
	{
		anjuta_goto_file_line (fn, ln);
		g_free (fn);
	}
}

void
on_message_indicate (AnMessageManager* amm, MessageIndicatorInfo *info)
{
	gchar *fn;
	GList *node;

	TextEditor *te;
	if (!anjuta_preferences_get_int_with_default(ANJUTA_PREFERENCES
												 (app->preferences),
											MESSAGES_INDICATORS_AUTOMATIC, 1)) {
		return;
	}
	g_return_if_fail (info);
	/* Is always NULL if no error or warning is indicated!
	g_return_if_fail (info->filename); */
	if (info->filename == NULL)
		return;
	fn = anjuta_get_full_filename (info->filename);
	g_return_if_fail (fn);
	
	node = app->text_editor_list;
	while (node)
	{
		te = (TextEditor *) node->data;
		if (te->full_filename == NULL)
		{
			node = g_list_next (node);
			continue;
		}
		if (strcmp (fn, te->full_filename) == 0)
		{
			if (info->line >= 0)
				text_editor_set_indicator (te, info->line, info->message_type);
			g_free (fn);
			return;
		}
		node = g_list_next (node);
	}
	g_free (fn);
}
