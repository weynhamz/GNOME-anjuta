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

#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>

#include "message-view.h"
#define MESSAGE_TYPE message_get_type()

struct _MessageViewPrivate
{
	//guint num_messages;
	gchar *line_buffer;

	GtkWidget *tree_view;

	AnjutaPreferences* prefs;
	GtkWidget *popup_menu;
	
	gint adj_chgd_hdlr;
	
	/* Properties */
	gchar *label;
	gboolean highlite;
	
	GdkRectangle tooltip_rect;
	GtkWidget *tooltip_window;
	gulong tooltip_timeout;
	PangoLayout *tooltip_layout;
	
	/* gconf notification ids */
	GList *gconf_notify_ids;
};

typedef struct
{
	IAnjutaMessageViewType type;
	gchar *summary;
	gchar *details;
	
} Message;

enum
{
	COLUMN_COLOR = 0,
	COLUMN_SUMMARY,
	COLUMN_MESSAGE,
	N_COLUMNS
};

enum
{
	MV_PROP_ID = 0,
	MV_PROP_LABEL,
	MV_PROP_HIGHLITE
};

static gpointer parent_class;

static void prefs_init (MessageView *mview);
static void prefs_finalize (MessageView *mview);

/* Ask the user for an uri name */
static gchar *
ask_user_for_save_uri (GtkWindow* parent)
{
	GtkWidget* dialog;
	gchar* uri;

       	dialog = gtk_file_chooser_dialog_new (_("Save file as"), parent,
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	}
	else
	{
		uri = NULL;
	}

	gtk_widget_destroy(dialog);

	return uri;
}

/* Message object creation, copy and freeing */
static Message*
message_new (IAnjutaMessageViewType type, const gchar *summary,
			 const gchar *details)
{
	/* DEBUG_PRINT ("Creating message"); */
	Message *message = g_new0 (Message, 1);
	message->type = type;
	if (summary)
		message->summary = g_strdup (summary);
	if (details)
		message->details = g_strdup (details);
	return message;
}

static Message*
message_copy (const Message *src)
{
	return message_new (src->type, src->summary, src->details);
}

static void
message_free (Message *message)
{
	/* DEBUG_PRINT ("Freeing message"); */
	g_free (message->summary);
	g_free (message->details);
	g_free (message);
}

static GType
message_get_type ()
{
	static GType type = 0;
	if (!type)
	{
		type = g_boxed_type_register_static ("MessageViewMessage",
											 (GBoxedCopyFunc) message_copy,
											 (GBoxedFreeFunc) message_free);
	}
	return type;
}

/* Utility functions */
/* Adds the char c to the string str */
static void
add_char(gchar** str, gchar c)
{
	gchar* buffer;	
	
	g_return_if_fail(str != NULL);
	
	buffer = g_strdup_printf("%s%c", *str, c);
	g_free(*str);
	*str = buffer;
}

static gchar*
escape_string (const gchar *str)
{
	GString *gstr;
	const gchar *iter;
	
	gstr = g_string_new ("");
	iter = str;
	while (*iter != '\0')
	{
		if (*iter == '>')
			gstr = g_string_append (gstr, "&gt;");
		else if (*iter == '<')
			gstr = g_string_append (gstr, "&lt;");
		else if (*iter == '&')
			gstr = g_string_append (gstr, "&amp;");
		else
			gstr = g_string_append_c (gstr, *iter);
		iter++;
	}
	return g_string_free (gstr, FALSE);
}

/* Tooltip operations -- taken from gtodo */

static gchar *
tooltip_get_display_text (MessageView *view)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view->privat->tree_view));
	
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(view->privat->tree_view),
		view->privat->tooltip_rect.x, view->privat->tooltip_rect.y,
		&path, NULL, NULL, NULL))
	{
		Message *message;
		gchar *text, *title, *desc;
		
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, COLUMN_MESSAGE, &message, -1); 
		gtk_tree_path_free(path);
		
		if (!message->details || !message->summary ||
			strlen (message->details) <= 0 ||
			strlen (message->summary) <= 0)
			return NULL;
		
		title = escape_string (message->summary);
		desc = escape_string (message->details);
		text = g_strdup_printf ("<b>%s</b>\n%s", title, desc);
		
		g_free (title);
		g_free (desc);
		
		return text;
	}
	return NULL;
}

static void
tooltip_paint (GtkWidget *widget, GdkEventExpose *event, MessageView *view)
{
	GtkStyle *style;
	gchar *tooltiptext;

	tooltiptext = tooltip_get_display_text (view);
	
	if (!tooltiptext)
		tooltiptext = g_strdup (_("No message details"));

	pango_layout_set_markup (view->privat->tooltip_layout,
							 tooltiptext,
							 strlen (tooltiptext));
	pango_layout_set_wrap(view->privat->tooltip_layout, PANGO_WRAP_CHAR);
	pango_layout_set_width(view->privat->tooltip_layout, 600000);
	style = view->privat->tooltip_window->style;

	gtk_paint_flat_box (style, view->privat->tooltip_window->window,
						GTK_STATE_NORMAL, GTK_SHADOW_OUT,
						NULL, view->privat->tooltip_window,
						"tooltip", 0, 0, -1, -1);

	gtk_paint_layout (style, view->privat->tooltip_window->window,
					  GTK_STATE_NORMAL, TRUE,
					  NULL, view->privat->tooltip_window,
					  "tooltip", 4, 4, view->privat->tooltip_layout);
	/*
	   g_object_unref(layout);
	   */
	g_free(tooltiptext);
	return;
}

static gboolean
tooltip_timeout (MessageView *view)
{
	gint scr_w,scr_h, w, h, x, y;
	gchar *tooltiptext;

	tooltiptext = tooltip_get_display_text (view);
	
	if (!tooltiptext)
		tooltiptext = g_strdup (_("No message details"));
	
	view->privat->tooltip_window = gtk_window_new (GTK_WINDOW_POPUP);
	view->privat->tooltip_window->parent = view->privat->tree_view;
	gtk_widget_set_app_paintable (view->privat->tooltip_window, TRUE);
	gtk_window_set_resizable (GTK_WINDOW(view->privat->tooltip_window), FALSE);
	gtk_widget_set_name (view->privat->tooltip_window, "gtk-tooltips");
	g_signal_connect (G_OBJECT(view->privat->tooltip_window), "expose_event",
					  G_CALLBACK(tooltip_paint), view);
	gtk_widget_ensure_style (view->privat->tooltip_window);

	view->privat->tooltip_layout =
		gtk_widget_create_pango_layout (view->privat->tooltip_window, NULL);
	pango_layout_set_wrap (view->privat->tooltip_layout, PANGO_WRAP_CHAR);
	pango_layout_set_width (view->privat->tooltip_layout, 600000);
	pango_layout_set_markup (view->privat->tooltip_layout, tooltiptext,
							 strlen (tooltiptext));
	scr_w = gdk_screen_width();
	scr_h = gdk_screen_height();
	pango_layout_get_size (view->privat->tooltip_layout, &w, &h);
	w = PANGO_PIXELS(w) + 8;
	h = PANGO_PIXELS(h) + 8;

	gdk_window_get_pointer (NULL, &x, &y, NULL);
	if (GTK_WIDGET_NO_WINDOW (view->privat->tree_view))
		y += view->privat->tree_view->allocation.y;

	x -= ((w >> 1) + 4);

	if ((x + w) > scr_w)
		x -= (x + w) - scr_w;
	else if (x < 0)
		x = 0;

	if ((y + h + 4) > scr_h)
		y = y - h;
	else
		y = y + 6;
	/*
	   g_object_unref(layout);
	   */
	gtk_widget_set_size_request (view->privat->tooltip_window, w, h);
	gtk_window_move (GTK_WINDOW (view->privat->tooltip_window), x, y);
	gtk_widget_show (view->privat->tooltip_window);
	g_free (tooltiptext);
	
	return FALSE;
}

static gboolean
tooltip_motion_cb (GtkWidget *tv, GdkEventMotion *event, MessageView *view)
{
	GtkTreePath *path;
	
	if (view->privat->tooltip_rect.y == 0 &&
		view->privat->tooltip_rect.height == 0 &&
		view->privat->tooltip_timeout)
	{
		g_source_remove (view->privat->tooltip_timeout);
		view->privat->tooltip_timeout = 0;
		if (view->privat->tooltip_window) {
			gtk_widget_destroy (view->privat->tooltip_window);
			view->privat->tooltip_window = NULL;
		}
		return FALSE;
	}
	if (view->privat->tooltip_timeout) {
		if (((int)event->y > view->privat->tooltip_rect.y) &&
			(((int)event->y - view->privat->tooltip_rect.height)
				< view->privat->tooltip_rect.y))
			return FALSE;

		if(event->y == 0)
		{
			g_source_remove (view->privat->tooltip_timeout);
			view->privat->tooltip_timeout = 0;
			return FALSE;
		}
		/* We've left the cell.  Remove the timeout and create a new one below */
		if (view->privat->tooltip_window) {
			gtk_widget_destroy (view->privat->tooltip_window);
			view->privat->tooltip_window = NULL;
		}
		g_source_remove (view->privat->tooltip_timeout);
		view->privat->tooltip_timeout = 0;
	}

	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(view->privat->tree_view),
									   event->x, event->y, &path,
									   NULL, NULL, NULL))
	{
		GtkTreeSelection *selection;
		
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(view->privat->tree_view));
		if (gtk_tree_selection_path_is_selected (selection, path))
		{
			gtk_tree_view_get_cell_area (GTK_TREE_VIEW (view->privat->tree_view),
										 path, NULL, &view->privat->tooltip_rect);
			
			if (view->privat->tooltip_rect.y != 0 &&
				view->privat->tooltip_rect.height != 0)
			{
				gchar *tooltiptext;
				
				tooltiptext = tooltip_get_display_text (view);
				if (tooltiptext == NULL)
					return FALSE;
				g_free (tooltiptext);
				
				view->privat->tooltip_timeout =
					g_timeout_add (500, (GSourceFunc) tooltip_timeout, view);
			}
		}
		gtk_tree_path_free (path);
	}
	return FALSE;
}

static void
tooltip_leave_cb (GtkWidget *w, GdkEventCrossing *e, MessageView *view)
{
	if (view->privat->tooltip_timeout) {
		g_source_remove (view->privat->tooltip_timeout);
		view->privat->tooltip_timeout = 0;
	}
	if (view->privat->tooltip_window) {
		gtk_widget_destroy (view->privat->tooltip_window);
		g_object_unref (view->privat->tooltip_layout);
		view->privat->tooltip_window = NULL;
	}
}

/* MessageView signal callbacks */
/* Send a signal if a message was double-clicked or ENTER or SPACE was pressed */
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
				const gchar* message =
					ianjuta_message_view_get_current_message(IANJUTA_MESSAGE_VIEW (view), NULL);
				if (message)
				{
					g_signal_emit_by_name (G_OBJECT (view), "message_clicked", 
										   message);
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
			const gchar* message =
				ianjuta_message_view_get_current_message(IANJUTA_MESSAGE_VIEW (view), NULL);
			if (message)
			{
				g_signal_emit_by_name (G_OBJECT (view), "message_clicked", 
									   message);
				return TRUE;
			}
		}	
		return FALSE;
	}
	else if (event->type == GDK_BUTTON_PRESS)
	{
		if (((GdkEventButton *) event)->button == 3)
		{
			gtk_menu_popup (GTK_MENU (view->privat->popup_menu), NULL, NULL, NULL, NULL,
					((GdkEventButton *) event)->button,
					((GdkEventButton *) event)->time);
			return TRUE;
		}
	}	
	return FALSE;
}

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

static void
message_view_dispose (GObject *obj)
{
	MessageView *mview = MESSAGE_VIEW (obj);
	if (mview->privat->gconf_notify_ids)
	{
		prefs_finalize (mview);
		mview->privat->gconf_notify_ids = NULL;
	}
	if (mview->privat->tooltip_timeout) {
		g_source_remove (mview->privat->tooltip_timeout);
		mview->privat->tooltip_timeout = 0;
	}
	if (mview->privat->tooltip_window) {
		gtk_widget_destroy (mview->privat->tooltip_window);
		g_object_unref (mview->privat->tooltip_layout);
		mview->privat->tooltip_window = NULL;
	}
	if (mview->privat->tree_view)
	{
		mview->privat->tree_view = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
message_view_finalize (GObject *obj)
{
	MessageView *mview = MESSAGE_VIEW (obj);
	if (mview->privat->line_buffer)
		g_free (mview->privat->line_buffer);
	if (mview->privat->label)
		g_free (mview->privat->label);
	g_free (mview->privat);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
message_view_instance_init (MessageView * self)
{
	GtkWidget *scrolled_win;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkListStore *model;
	GtkAdjustment* adj;

	g_return_if_fail(self != NULL);
	self->privat = g_new0 (MessageViewPrivate, 1);

	/* Init private data */
	self->privat->line_buffer = g_strdup("");

	/* Create the tree widget */
	model = gtk_list_store_new (N_COLUMNS, GDK_TYPE_COLOR,
								G_TYPE_STRING,
								MESSAGE_TYPE);
	self->privat->tree_view =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_widget_show (self->privat->tree_view);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW
									   (self->privat->tree_view), FALSE);

	/* Creat columns to hold text and color of a line, this
	 * columns are invisible of course. */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_title (column, _("Messages"));
	gtk_tree_view_column_add_attribute
		(column, renderer, "foreground-gdk", COLUMN_COLOR);
	gtk_tree_view_column_add_attribute
		(column, renderer, "markup", COLUMN_SUMMARY);
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
	g_signal_connect (G_OBJECT(self->privat->tree_view), "event", 
					  G_CALLBACK (on_message_event), self);
	g_signal_connect (G_OBJECT (self->privat->tree_view), "motion-notify-event",
					  G_CALLBACK (tooltip_motion_cb), self);
	g_signal_connect (G_OBJECT (self->privat->tree_view), "leave-notify-event",
					  G_CALLBACK (tooltip_leave_cb), self);
	g_object_unref (model);
}

static void
message_view_class_init (MessageViewClass * klass)
{
	GParamSpec *message_view_spec_label;
	GParamSpec *message_view_spec_highlite;
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	gobject_class->set_property = message_view_set_property;
	gobject_class->get_property = message_view_get_property;
	gobject_class->finalize = message_view_finalize;
	gobject_class->dispose = message_view_dispose;
	
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
message_view_new (AnjutaPreferences* prefs, GtkWidget* popup_menu)
{
	MessageView * mv = MESSAGE_VIEW (g_object_new (message_view_get_type (), NULL));
	mv->privat->prefs = prefs;
	mv->privat->popup_menu = popup_menu;
	prefs_init (mv);
	return GTK_WIDGET(mv);
}

void message_view_next(MessageView* view)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *select;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW
					 (view->privat->tree_view));
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW
					      (view->privat->tree_view));

	if (!gtk_tree_selection_get_selected (select, &model, &iter))
	{
		if (gtk_tree_model_get_iter_first (model, &iter))
			gtk_tree_selection_select_iter (select, &iter);
	}
	while (gtk_tree_model_iter_next (model, &iter))
	{
		Message *message;
		gtk_tree_model_get (model, &iter, COLUMN_MESSAGE,
							&message, -1);
		if (message->type != IANJUTA_MESSAGE_VIEW_TYPE_NORMAL
			&& message->type != IANJUTA_MESSAGE_VIEW_TYPE_INFO)
		{
			const gchar* message;
			gtk_tree_selection_select_iter (select, &iter);
			message =
				ianjuta_message_view_get_current_message(IANJUTA_MESSAGE_VIEW (view), NULL);
			if (message)
			{
				GtkTreePath *path;
				path = gtk_tree_model_get_path (model, &iter);
				gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW
											  (view->privat->tree_view),
											  path, NULL, FALSE, 0, 0);
				gtk_tree_path_free (path);
				g_signal_emit_by_name (G_OBJECT (view), "message_clicked", 
									   message);
			}
			break;
		}
	}
}

void message_view_previous(MessageView* view)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *select;
	GtkTreePath *path;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW
					 (view->privat->tree_view));
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW
					      (view->privat->tree_view));
	
	if (!gtk_tree_selection_get_selected (select, &model, &iter))
	{
		if (gtk_tree_model_get_iter_first (model, &iter))
			gtk_tree_selection_select_iter (select, &iter);
	}

	/* gtk_tree_model_iter_previous does not exist, use path */
	path = gtk_tree_model_get_path (model, &iter);

	while (gtk_tree_path_prev(path))
	{
		Message *message;
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get (model, &iter, COLUMN_MESSAGE,
							&message, -1);
		if (message->type != IANJUTA_MESSAGE_VIEW_TYPE_NORMAL
			&& message->type != IANJUTA_MESSAGE_VIEW_TYPE_INFO)
		{
			const gchar* message;
			
			gtk_tree_selection_select_iter (select, &iter);
			message =
				ianjuta_message_view_get_current_message(IANJUTA_MESSAGE_VIEW (view), NULL);
			if (message)
			{
				GtkTreePath *path;
				path = gtk_tree_model_get_path (model, &iter);
				gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW
											  (view->privat->tree_view),
											  path, NULL, FALSE, 0, 0);
				gtk_tree_path_free (path);
				g_signal_emit_by_name (G_OBJECT (view), "message_clicked", 
									   message);
			}
			break;
		}
	}
	gtk_tree_path_free (path);
}

static gboolean message_view_save_as(MessageView* view, gchar* uri)
{
	GnomeVFSHandle* handle;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean ok;

	if (uri == NULL) return FALSE;

	/* Create file */
	if (gnome_vfs_create (&handle, uri, GNOME_VFS_OPEN_WRITE, FALSE, 0664) != GNOME_VFS_OK)
	{
		return FALSE;
	}

	/* Save all lines of message view */	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view->privat->tree_view));

	ok = TRUE;
	gtk_tree_model_get_iter_first (model, &iter);
	while (gtk_tree_model_iter_next (model, &iter))
	{
		Message *message;
		GnomeVFSFileSize written;

		gtk_tree_model_get (model, &iter, COLUMN_MESSAGE, &message, -1);
		if (message)
		{
			if (message->details && (strlen (message->details) > 0))
			{
				if (gnome_vfs_write (handle, message->details, strlen (message->details), &written) != GNOME_VFS_OK)
				{
					ok = FALSE;
				}
			}
			else
			{
				if (gnome_vfs_write (handle, message->summary, strlen (message->summary), &written) != GNOME_VFS_OK)
				{
					ok = FALSE;
				}
			}
			if (gnome_vfs_write (handle, "\n", 1, &written) != GNOME_VFS_OK)
			{
				ok = FALSE;
			}
		}
	}
	gnome_vfs_close (handle);

	return ok;
}

void message_view_save(MessageView* view)
{
	GtkWindow* parent;
	gchar* uri;
     
       	parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view)));	

	uri = ask_user_for_save_uri (parent);
	if (uri)
	{
		if (message_view_save_as (view, uri) == FALSE)
		{
			anjuta_util_dialog_error(parent, _("Error writing %s"), uri);
		}
		g_free (uri);
	}
}

/* Preferences notifications */
static void
pref_change_color (MessageView *mview, IAnjutaMessageViewType type,
				   const gchar *color_pref_key)
{
	GdkColor *color;
	GtkListStore *store;
	GtkTreeIter iter;
	gboolean success;
	
	color = anjuta_util_convert_color (mview->privat->prefs, color_pref_key);
	store = GTK_LIST_STORE (gtk_tree_view_get_model
				(GTK_TREE_VIEW (mview->privat->tree_view)));
	success = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
	while (success)
	{
		Message *message;
		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COLUMN_MESSAGE,
							&message, -1);
		if (message && message->type == type)
		{
			gtk_list_store_set (store, &iter, COLUMN_COLOR, color, -1);
		}
		success = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
	}
	g_free(color);
}

static void
on_gconf_notify_color_info (GConfClient *gclient, guint cnxn_id,
							GConfEntry *entry, gpointer user_data)
{
	pref_change_color (MESSAGE_VIEW (user_data),
					   IANJUTA_MESSAGE_VIEW_TYPE_INFO,
					   "messages.color.info");
}

static void
on_gconf_notify_color_warning (GConfClient *gclient, guint cnxn_id,
							   GConfEntry *entry, gpointer user_data)
{
	pref_change_color (MESSAGE_VIEW (user_data),
					   IANJUTA_MESSAGE_VIEW_TYPE_WARNING,
					   "messages.color.warning");
}

static void
on_gconf_notify_color_error (GConfClient *gclient, guint cnxn_id,
							 GConfEntry *entry, gpointer user_data)
{
	pref_change_color (MESSAGE_VIEW (user_data),
					   IANJUTA_MESSAGE_VIEW_TYPE_ERROR,
					   "messages.color.error");
}

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (mview->privat->prefs, \
											   key, func, mview, NULL); \
	mview->privat->gconf_notify_ids = g_list_prepend (mview->privat->gconf_notify_ids, \
										   GINT_TO_POINTER(notify_id));
static void
prefs_init (MessageView *mview)
{
	guint notify_id;
	REGISTER_NOTIFY ("messages.color.info", on_gconf_notify_color_info);
	REGISTER_NOTIFY ("messages.color.warning", on_gconf_notify_color_warning);
	REGISTER_NOTIFY ("messages.color.error", on_gconf_notify_color_error);
}

static void
prefs_finalize (MessageView *mview)
{
	GList *node;
	node = mview->privat->gconf_notify_ids;
	while (node)
	{
		anjuta_preferences_notify_remove (mview->privat->prefs,
										  GPOINTER_TO_INT (node->data));
		node = g_list_next (node);
	}
	g_list_free (mview->privat->gconf_notify_ids);
	mview->privat->gconf_notify_ids = NULL;
}

/* IAnjutaMessageView interface implementation */

/* Appends the text in buffer. Flushes the buffer where a newline is found.
 * by emiiting buffer_flushed signal. The string is expected to be utf8.
 */
static void
imessage_view_buffer_append (IAnjutaMessageView * message_view,
									const gchar * message, GError ** e)
{
	MessageView *view;
	gint cur_char;
	int len = strlen(message);
	
	g_return_if_fail (MESSAGE_IS_VIEW (message_view));
	g_return_if_fail (message != NULL);	
	
	view = MESSAGE_VIEW (message_view);
	
	/* Check if message contains newlines */
	for (cur_char = 0; cur_char < len; cur_char++)
	{		
		/* Replace "\\\n" with " " */
		if (message[cur_char] == '\\' && cur_char < len - 1 &&
			message[cur_char+1] == '\n')
		{
			add_char(&view->privat->line_buffer, ' ');
			cur_char++;
			continue;
		}

		/* Is newline => print line */
		if (message[cur_char] != '\n')
		{
			add_char(&view->privat->line_buffer, message[cur_char]);
		}
		else
		{
			g_signal_emit_by_name (G_OBJECT (view), "buffer_flushed",
								   view->privat->line_buffer);
			g_free(view->privat->line_buffer);
			view->privat->line_buffer = g_strdup("");
		}
	}
}

static void
imessage_view_append (IAnjutaMessageView *message_view,
					  IAnjutaMessageViewType type,
					  const gchar *summary,
					  const gchar *details,
					  GError ** e)
{
	GdkColor* color;
	GtkListStore *store;
	GtkTreeIter iter;
	gboolean highlite;
	gchar *utf8_msg;
	gchar *escaped_str;
	
	MessageView *view;
	Message *message;

	g_return_if_fail (MESSAGE_IS_VIEW (message_view));
	
	view = MESSAGE_VIEW (message_view);
	
	message = message_new (type, summary, details);
	
	g_object_get (G_OBJECT (view), "highlite", &highlite, NULL);
	color = NULL;
	if (highlite)
	{
		switch (message->type)
		{
			case IANJUTA_MESSAGE_VIEW_TYPE_INFO:
				color = anjuta_util_convert_color(view->privat->prefs,
									  "messages.color.info");
				break;
			case IANJUTA_MESSAGE_VIEW_TYPE_WARNING:
				color = anjuta_util_convert_color(view->privat->prefs,
									  "messages.color.warning");
				break;
			case IANJUTA_MESSAGE_VIEW_TYPE_ERROR:
				color = anjuta_util_convert_color(view->privat->prefs,
									  "messages.color.error");
				break;
			default:
				color = NULL;
		}
	}

	/* Add the message to the tree */
	store = GTK_LIST_STORE (gtk_tree_view_get_model
				(GTK_TREE_VIEW (view->privat->tree_view)));
	gtk_list_store_append (store, &iter);

	/*
	 * Must be normalized to compose representation to be
	 * displayed correctly (Bug in gtk_list???)
	 */
	utf8_msg = g_utf8_normalize (message->summary, -1,
								 G_NORMALIZE_DEFAULT_COMPOSE);
	if (message->details && strlen (message->details) > 0)
	{
		gchar *summary;
		summary = escape_string (message->summary);
		escaped_str = g_strdup_printf ("<b>%s</b>", summary);
		g_free (summary);
	} else {
		escaped_str = escape_string (message->summary);
	}
	if (color)
	{
		gtk_list_store_set (store, &iter,
							COLUMN_COLOR, color,
							COLUMN_SUMMARY, escaped_str,
							COLUMN_MESSAGE, message,
							-1);
	
		/* Can we free the color when it's in the tree_view? */
		// gdk_color_free (color);
	} else {
		gtk_list_store_set (store, &iter,
							COLUMN_SUMMARY, escaped_str,
							COLUMN_MESSAGE, message,
							-1);
	}
	message_free (message);
	g_free (utf8_msg);
	g_free (escaped_str);
}

/* Clear all messages from the message view */
static void
imessage_view_clear (IAnjutaMessageView *message_view, GError **e)
{
	GtkListStore *store;
	MessageView *view;

	g_return_if_fail (MESSAGE_IS_VIEW (message_view));
	view = MESSAGE_VIEW (message_view);

	store = GTK_LIST_STORE (gtk_tree_view_get_model
				(GTK_TREE_VIEW (view->privat->tree_view)));
	gtk_list_store_clear (store);
}

/* Move the selection to the next line. */
static void
imessage_view_select_next (IAnjutaMessageView * message_view,
						   GError ** e)
{
	MessageView* view = MESSAGE_VIEW(message_view);
	message_view_next(view);
}

/* Move the selection to the previous line. */
static void
imessage_view_select_previous (IAnjutaMessageView * message_view,
							   GError ** e)
{
	MessageView *view = MESSAGE_VIEW(message_view);
	message_view_previous(view);	
}

/* Return the currently selected messages or the first message if no
 * message is selected or NULL if no messages are availible. The
 * returned message must not be freed.
 */
static const gchar *
imessage_view_get_current_message (IAnjutaMessageView * message_view,
								   GError ** e)
{
	MessageView *view;
	GtkTreeIter iter;
	GtkTreeSelection *select;
	GtkTreeModel *model;
	const Message *message;

	g_return_val_if_fail (MESSAGE_IS_VIEW (message_view), NULL);
	
	view = MESSAGE_VIEW (message_view);
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW
									      (view->privat->tree_view));

	if (!gtk_tree_selection_get_selected (select, &model, &iter))
	{
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter))
		{
			gtk_tree_model_get (GTK_TREE_MODEL (model),
								&iter, COLUMN_MESSAGE, &message, -1);
			if (message)
			{
				if (message->details && strlen (message->details) > 0)
					return message->details;
				else
					return message->summary;
			}
		}
	}
	else
	{
		gtk_tree_model_get (GTK_TREE_MODEL (model),
						    &iter, COLUMN_MESSAGE, &message, -1);
		if (message)
		{
			if (message->details && strlen (message->details) > 0)
				return message->details;
			else
				return message->summary;
		}
	}
	return NULL;
}

/* Returns a GList which contains all messages, the GList itself
 * must be freed, the messages are managed by the message view and
 * must not be freed. NULL is return if no messages are availible.
 */
static GList *
imessage_view_get_all_messages (IAnjutaMessageView * message_view,
								GError ** e)
{
	MessageView *view;
	GtkListStore *store;
	GtkTreeIter iter;
	Message *message;
	GList *messages = NULL;
	
	g_return_val_if_fail (MESSAGE_IS_VIEW (message_view), NULL);
	
	view = MESSAGE_VIEW (message_view);
	store = GTK_LIST_STORE (gtk_tree_view_get_model
				(GTK_TREE_VIEW (view->privat->tree_view)));
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
	{
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
							    COLUMN_MESSAGE, &message);
			messages = g_list_prepend (messages, message->details);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
	}
	return messages;
}

static void
imessage_view_iface_init (IAnjutaMessageViewIface *iface)
{
	iface->buffer_append = imessage_view_buffer_append;
	iface->append = imessage_view_append;
	iface->clear = imessage_view_clear;
	iface->select_next = imessage_view_select_next;
	iface->select_previous = imessage_view_select_previous;
	iface->get_current_message = imessage_view_get_current_message;
	iface->get_all_messages = imessage_view_get_all_messages;
}

ANJUTA_TYPE_BEGIN(MessageView, message_view, GTK_TYPE_HBOX);
ANJUTA_TYPE_ADD_INTERFACE(imessage_view, IANJUTA_TYPE_MESSAGE_VIEW);
ANJUTA_TYPE_END;
