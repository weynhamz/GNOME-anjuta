/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  (c) Johannes Schmid 2003
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>

#include "message-view.h"

struct _MessageViewPrivate
{
	guint num_messages;
	gchar *line_buffer;

	GtkWidget *tree_view;

	AnjutaPreferences* prefs;
	
	gint adj_chgd_hdlr;
	
	/* Properties */
	gchar *label;
	gboolean highlite;
};

enum
{
	COLUMN_COLOR = 0,
	COLUMN_LINE,
	COLUMN_MESSAGES,
	N_COLUMNS
};

enum
{
	MV_PROP_ID = 0,
	MV_PROP_LABEL,
	MV_PROP_HIGHLITE
};

typedef enum
{
	WARNING,
	ERROR,
	MESSAGE,
}
LineType;

typedef enum
{
	SIGNAL_CLICKED,
	SIGNAL_END,
}
Signals;

static guint message_view_signals[SIGNAL_END] = { 0 };

static void
message_view_set_property (GObject * object,
			   guint property_id,
			   const GValue * value, GParamSpec * pspec);

static void
message_view_get_property (GObject * object,
			   guint property_id,
			   GValue * value, GParamSpec * pspec);

static gboolean
on_message_event (GObject* object, GdkEvent* event, gpointer data);

/* Tools */
void add_char(gchar** str, gchar c);
GdkColor* convert_color(AnjutaPreferences* prefs, const gchar* pref_name);

/* Function to register the message-view object */

// GNOME_CLASS_BOILERPLATE (MessageView, message_view, GtkHBox, GTK_TYPE_HBOX);

/* Init message-view */

static void 
on_adjustment_changed (GtkAdjustment* adj, gpointer data)
{
	gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

static void
on_adjustment_value_changed (GtkAdjustment* adj, gpointer data)
{
	MessageView *self = (MessageView *) data;
	if (adj->value > (adj->upper - adj->page_size) - 0.1)
	{
		if (!self->privat->adj_chgd_hdlr)
		{
			self->privat->adj_chgd_hdlr =
				g_signal_connect (G_OBJECT (adj), "changed",
								  G_CALLBACK (on_adjustment_changed), NULL);
		}
	}
	else
	{
		if (self->privat->adj_chgd_hdlr)
		{
			g_signal_handler_disconnect (G_OBJECT (adj), self->privat->adj_chgd_hdlr);
			self->privat->adj_chgd_hdlr = 0;
		}
	}
}

static void
message_view_instance_init (MessageView * self)
{
	GtkWidget *scrolled_win;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkAdjustment* adj;

	g_return_if_fail(self != NULL);
	self->privat = g_new0 (MessageViewPrivate, 1);

	/* Init private data */
	self->privat->num_messages = 0;
	self->privat->line_buffer = g_strdup("");

	/* Create the tree widget */
	self->privat->tree_view =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (gtk_list_store_new
					       (N_COLUMNS, GDK_TYPE_COLOR,
						G_TYPE_UINT, G_TYPE_STRING)));
	gtk_widget_show (self->privat->tree_view);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW
					   (self->privat->tree_view), FALSE);

	/* Creat columns to hold text and color of a line, this
	 * columns are invisible of course. */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_title (column, "invisible");
	gtk_tree_view_column_add_attribute
		(column, renderer, "foreground-gdk", COLUMN_COLOR);
	gtk_tree_view_column_add_attribute
		(column, renderer, "text", COLUMN_MESSAGES);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->privat->tree_view),
				     column);

	/* Adjust selection */
	select = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (self->privat->tree_view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);

	/* Add tree view to a scrolled window */
	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_win),
			   self->privat->tree_view);
	gtk_widget_show (scrolled_win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
											   (scrolled_win));
	self->privat->adj_chgd_hdlr = g_signal_connect(G_OBJECT(adj), "changed",
									 G_CALLBACK (on_adjustment_changed), self);
	g_signal_connect(G_OBJECT(adj), "value_changed",
					 G_CALLBACK(on_adjustment_value_changed), self);

	/* Add it to the dockitem */
	gtk_container_add (GTK_CONTAINER (self), scrolled_win);
	
	/* Connect signals */
	g_signal_connect(G_OBJECT(self->privat->tree_view), "event", 
		G_CALLBACK(on_message_event), self);
}

/* Register properties */

static void
message_view_class_init (MessageViewClass * klass)
{
	GParamSpec *message_view_spec_label;
	GParamSpec *message_view_spec_highlite;
	
	GType paramter[1] = { G_TYPE_STRING };
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->set_property = message_view_set_property;
	gobject_class->get_property = message_view_get_property;

	message_view_signals[SIGNAL_CLICKED] =	g_signal_newv ("message_clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  NULL, NULL, NULL,
                  g_cclosure_marshal_VOID__STRING, 
				  G_TYPE_NONE, 1, 
				  paramter);
	
	message_view_spec_label = g_param_spec_string ("label",
						       "Label of the view",
						       "Used to decorate the view,"
						       "translateable",
						       "no label",
						       G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 MV_PROP_LABEL,
					 message_view_spec_label);

	message_view_spec_highlite = g_param_spec_boolean ("highlite",
							   "Highlite build messages",
							   "If TRUE, specify colors",
							   FALSE,
							   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 MV_PROP_HIGHLITE,
					 message_view_spec_highlite);

}

/* Returns a new message-view instance */

GtkWidget *
message_view_new (AnjutaPreferences* prefs)
{
	MessageView * mv = MESSAGE_VIEW (g_object_new (message_view_get_type (), NULL));
	mv->privat->prefs = prefs;
	return GTK_WIDGET(mv);
}

/* Adds a message to the message-view. 
The string is expected to be utf8 */

void
message_view_append (MessageView * view, const gchar * message)
{
	gint cur_char;
	gboolean highlite;

	gchar *utf8_msg;

	g_return_if_fail (view != NULL);
	g_return_if_fail (message != NULL);	
	
	
	/* Check if message contains newlines */
	for (cur_char = 0; cur_char < strlen(message); cur_char++)
	{		
		/* Is newline => print line */
		if (message[cur_char] != '\n')
		{
			add_char(&view->privat->line_buffer, message[cur_char]);
		}
		else
		{
			GdkColor* color;
			GtkListStore *store;
			GtkTreeIter iter;
			
			gboolean truncat_mesg;
			guint mesg_first;
			guint mesg_last;

			gchar* line;
			
			/* Truncate Message */			
			truncat_mesg = anjuta_preferences_get_int(view->privat->prefs, "truncat.messages");
			if (truncat_mesg)
			{ 
				mesg_first = anjuta_preferences_get_int(view->privat->prefs, "truncat.mesg.first");
				mesg_last = anjuta_preferences_get_int(view->privat->prefs, "truncat.mesg.last");
			}

			if (truncat_mesg == TRUE
			    && strlen(view->privat->line_buffer) >=
			    (mesg_first + mesg_last))
			{
				gint cur_char;
				gint len;
				gchar* first_part;
				gchar* last_part = strdup("");
				
				first_part = 
					g_strndup(view->privat->line_buffer, mesg_first);
				len = strlen(view->privat->line_buffer) - mesg_last;
				for (cur_char = len; cur_char >= 0; cur_char++)
				{
					add_char(&last_part, 
						view->privat->line_buffer[cur_char]);
				}
				line = g_strconcat(first_part, "........", last_part, NULL);
				g_free(first_part);
				g_free(last_part);
			}
			else
			{
				line = g_strdup(view->privat->line_buffer);
			}
			
			g_object_get (G_OBJECT (view), "highlite", &highlite, NULL);
			if (highlite)
			{
				if (strstr (line, _("error:")))
					color = convert_color(view->privat->prefs, "messages.color.error");
				else if (strstr (line, _("warning:")))
				{
					color = convert_color(view->privat->prefs, "messages.color.warning");
				}
			}
			else
				color = convert_color(view->privat->prefs, "messages.color.message1");

			/* Add the message to the tree */
			store = GTK_LIST_STORE (gtk_tree_view_get_model
						(GTK_TREE_VIEW
						 (view->privat->tree_view)));
			gtk_list_store_append (store, &iter);

			/*
			 * Must be normalized to compose representation to be
			 * displayed correctly (Bug in gtk_list???)
			 */
			utf8_msg = g_utf8_normalize (line, -1,
						     G_NORMALIZE_DEFAULT_COMPOSE);
			gtk_list_store_set (store, &iter,
					    COLUMN_MESSAGES, utf8_msg,
					    COLUMN_COLOR, color,
					    COLUMN_LINE,
					    view->privat->num_messages, -1);
			/* 
				Can we free the color when it's in the tree_view?
				g_free (color)
			*/
			g_free (utf8_msg);
			view->privat->num_messages++;
			
			g_free(view->privat->line_buffer);
			view->privat->line_buffer = g_strdup("");
		}
	}
}

static void
message_view_set_property (GObject * object,
			   guint property_id,
			   const GValue * value, GParamSpec * pspec)
{
	MessageView *self = MESSAGE_VIEW (object);
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
	case MV_PROP_LABEL:
	{
		g_free (self->privat->label);
		self->privat->label = g_value_dup_string (value);
		break;
	}
	case MV_PROP_HIGHLITE:
	{
		self->privat->highlite = g_value_get_boolean (value);
		break;
	}
	default:
	{
		g_assert ("Unknown property");
		break;
	}
	}
}

/* Read message view's properties */

static void
message_view_get_property (GObject * object,
			   guint property_id,
			   GValue * value, GParamSpec * pspec)
{
	MessageView *self = MESSAGE_VIEW (object);
	
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		case MV_PROP_LABEL:
		{
			g_value_set_string (value, self->privat->label);
			break;
		}
		case MV_PROP_HIGHLITE:
		{
			g_value_set_boolean (value, self->privat->highlite);
			break;
		}
		default:
		{
			g_assert ("Unknown property");
			break;
		}
	}
}

/* Move the selection to the next line. Return TRUE is next line 
is availible or if the first line was selected, otherwise false */

gboolean
message_view_select_next (MessageView * view)
{
	g_return_val_if_fail(view != NULL, FALSE);
	
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *select;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW
					 (view->privat->tree_view));
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW
					      (view->privat->tree_view));

	if (!gtk_tree_selection_get_selected (select, &model, &iter))
	{
		if (!gtk_tree_model_get_iter_first (model, &iter))
			return FALSE;
		else
		{
			gtk_tree_selection_select_iter (select, &iter);
			return TRUE;
		}
	}
	if (gtk_tree_model_iter_next (model, &iter))
	{
		gtk_tree_selection_select_iter (select, &iter);
		return TRUE;
	}
	return FALSE;
}

/* Move the selection to the previous line. Return TRUE is 
previous line is availible or if the first line was selected,
otherwise false */

gboolean
message_view_select_previous (MessageView * view)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *select;
	GtkTreePath *path;

	g_return_val_if_fail(view != NULL, FALSE);
	
	if (!gtk_tree_selection_get_selected (select, &model, &iter))
	{
		if (!gtk_tree_model_get_iter_first (model, &iter))
			return FALSE;
		else
		{
			gtk_tree_selection_select_iter (select, &iter);
			return TRUE;
		}
	}

	path = gtk_tree_model_get_path (model, &iter);

	if (gtk_tree_path_prev (path))
	{
		gtk_tree_selection_select_path (select, path);
		gtk_tree_path_free (path);
		return TRUE;
	}
	else
	{
		gtk_tree_path_free (path);
		return FALSE;
	}
}

/* Returns the currently selected line or the last line */

guint
message_view_get_line (MessageView * view)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *select;
	guint line;

	g_return_val_if_fail(view != NULL, 0);
	
	if (!gtk_tree_selection_get_selected (select, &model, &iter))
		return view->privat->num_messages;
	else
	{
		gtk_tree_model_get (model, &iter, COLUMN_LINE, &line, -1);
		return line;
	}
}

/* Return the currently selected messages or the first message if no
message is selected or NULL if no messages are availible. The
returned message must not be freed */

gchar *
message_view_get_message (MessageView * view)
{
	GtkTreeIter iter;
	GtkTreeSelection *select;
	GtkTreeModel *model;
	gchar *message;

	g_return_val_if_fail(view != NULL, NULL);
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW
					      (view->privat->tree_view));

	if (!gtk_tree_selection_get_selected (select, &model, &iter))
	{
		if (gtk_tree_model_get_iter_first
		    (GTK_TREE_MODEL (model), &iter))
		{
			gtk_tree_model_get (GTK_TREE_MODEL (model),
					    &iter, COLUMN_MESSAGES, &message,
					    -1);
			return message;
		}
		else
			return NULL;
	}
	else
	{
		gtk_tree_model_get (GTK_TREE_MODEL (model),
				    &iter, COLUMN_MESSAGES, &message, -1);
		return message;
	}
}

/* Returns a GList which contains all messages, the GList itself
must be freed, the messages are managed by the message view and
must not be freed. NULL is return if no messages are availible */

GList *
message_view_get_messages (MessageView * view)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *line;
	GList *messages = NULL;
	
	g_return_val_if_fail(view != NULL, NULL);

	store = GTK_LIST_STORE (gtk_tree_view_get_model
				(GTK_TREE_VIEW (view->privat->tree_view)));
	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
		return NULL;
	else
	{
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
					    COLUMN_MESSAGES, &line);
			g_list_append (messages, &line);
		}
		while (gtk_tree_model_iter_next
		       (GTK_TREE_MODEL (store), &iter));
		return messages;
	}
}

/* Clear all messages from the message view */

void
message_view_clear (MessageView * view)
{
	GtkListStore *store;

	store = GTK_LIST_STORE (gtk_tree_view_get_model
				(GTK_TREE_VIEW (view->privat->tree_view)));
	gtk_list_store_clear (store);
	view->privat->num_messages = 0;
}

/* Send a signal if a column was double-clicked or ENTER od SPACE 
was pressed */

static gboolean
on_message_event (GObject* object, GdkEvent* event, gpointer data)
{
	g_return_val_if_fail(object != NULL, FALSE);
	g_return_val_if_fail(event != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	
	MessageView* view = MESSAGE_VIEW(data);
		
	if (event == NULL)
		return FALSE;
		
	if (event->type == GDK_KEY_PRESS)
	{
		switch(((GdkEventKey *)event)->keyval)
		{
			case GDK_space:
			case GDK_Return:
			{
				if (view->privat->num_messages != 0)
				{
					gchar* message = message_view_get_message(view);
					g_signal_emit(view, message_view_signals[SIGNAL_CLICKED], 
						0, message);
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
			gchar* message = message_view_get_message(view);
			g_signal_emit(view, message_view_signals[SIGNAL_CLICKED], 
					0, message);
			return TRUE;
		}	
			return FALSE;
	}		
	return FALSE;
}

/* Adds the char c to the string str */

void add_char(gchar** str, gchar c)
{
	gchar* buffer;	
	
	g_return_if_fail(str != NULL);
	
	buffer = g_strdup_printf("%s%c", *str, c);
	g_free(*str);
	*str = buffer;
}

/* Get a GdkColor from preferences. Free the color with gfree() */

GdkColor* convert_color(AnjutaPreferences* prefs, const gchar* pref_name)
{
	guint8 r, g, b;
	guint factor = ((guint16) -1) / ((guint8) -1);
	char* color;
	GdkColor* gdkcolor = g_new0(GdkColor, 1);
	color = anjuta_preferences_get(prefs, pref_name);
	if (color)
	{
		anjuta_util_color_from_string (color, &r, &g, &b);
		gdkcolor->pixel = 0;
		gdkcolor->red = r * factor;
		gdkcolor->green = g * factor;
		gdkcolor->blue = b * factor;
		g_free(color);
	}
	return gdkcolor;
}

/*
 * IAnjutaMessageView interface implementation
 */
static void
ianjuta_msgview_append (IAnjutaMessageView * message_view,
						const gchar * message, GError ** e)
{
	message_view_append (MESSAGE_VIEW (message_view), message);
}

static void
ianjuta_msgview_clear (IAnjutaMessageView * message_view, GError ** e)
{
	message_view_clear (MESSAGE_VIEW (message_view));
}

static void
ianjuta_msgview_select_next (IAnjutaMessageView * message_view,
							 GError ** e)
{
	message_view_select_next (MESSAGE_VIEW (message_view));
}

static void
ianjuta_msgview_select_previous (IAnjutaMessageView * message_view,
								 GError ** e)
{
	message_view_select_previous (MESSAGE_VIEW (message_view));
}

static gint
ianjuta_msgview_get_line (IAnjutaMessageView * message_view, GError ** e)
{
	return message_view_get_line (MESSAGE_VIEW (message_view));
}

static gchar *
ianjuta_msgview_get_message (IAnjutaMessageView * message_view,
							 GError ** e)
{
	return message_view_get_message (MESSAGE_VIEW (message_view));
}

static GList *
ianjuta_msgview_get_all_messages (IAnjutaMessageView * message_view,
								  GError ** e)
{
	return message_view_get_messages (MESSAGE_VIEW (message_view));
}

static void
ianjuta_msgview_iface_init (IAnjutaMessageViewIface *iface)
{
	iface->append = ianjuta_msgview_append;
	iface->clear = ianjuta_msgview_clear;
	iface->select_next = ianjuta_msgview_select_next;
	iface->select_previous = ianjuta_msgview_select_previous;
	iface->get_line = ianjuta_msgview_get_line;
	iface->get_message = ianjuta_msgview_get_message;
	iface->get_all_messages = ianjuta_msgview_get_all_messages;
}

ANJUTA_TYPE_BEGIN(MessageView, message_view, GTK_TYPE_HBOX);
ANJUTA_INTERFACE(ianjuta_msgview, IANJUTA_TYPE_MESSAGE_VIEW);
ANJUTA_TYPE_END;
