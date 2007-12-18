/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <glib/gi18n.h>
#include "search-box.h"

#include <gtk/gtkbutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkentry.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>

#define ANJUTA_STOCK_GOTO_LINE "anjuta-goto-line"

typedef struct _SearchBoxPrivate SearchBoxPrivate;

struct _SearchBoxPrivate
{
	GtkWidget* search_entry;
	GtkWidget* case_check;
	GtkWidget* search_button;
	GtkWidget* close_button;
	
	GtkWidget* goto_entry;
	GtkWidget* goto_button;
	
	IAnjutaEditor* current_editor;
	AnjutaStatus* status;
	
	/* Incremental search */
	IAnjutaIterable* last_start;
};

#ifdef GET_PRIVATE
# undef GET_PRIVATE
#endif
#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), SEARCH_TYPE_BOX, SearchBoxPrivate))

G_DEFINE_TYPE (SearchBox, search_box, GTK_TYPE_HBOX);

static void
on_search_box_hide (GtkWidget* button, SearchBox* search_box)
{
	gtk_widget_hide (GTK_WIDGET (search_box));
}

static void
on_document_changed (AnjutaDocman* docman, IAnjutaDocument* doc,
					 SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	if (!doc || !IANJUTA_IS_EDITOR (doc))
	{
		gtk_widget_hide (GTK_WIDGET (search_box));
		private->current_editor = NULL;
	}
	else
	{
		private->current_editor = IANJUTA_EDITOR (doc);
	}
}

static void
on_goto_activated (GtkWidget* widget, SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	const gchar* str_line = gtk_entry_get_text (GTK_ENTRY (private->goto_entry));
	
	gint line = atoi (str_line);
	if (line > 0)
	{
		ianjuta_editor_goto_line (private->current_editor, line, NULL);
	}
}

static void
search_box_set_entry_color (SearchBox* search_box, gboolean found)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	if (!found)
	{
		GdkColor red;
		GdkColor white;

		/* FIXME: a11y and theme */

		gdk_color_parse ("#FF6666", &red);
		gdk_color_parse ("white", &white);

		gtk_widget_modify_base (private->search_entry,
				        GTK_STATE_NORMAL,
				        &red);
		gtk_widget_modify_text (private->search_entry,
				        GTK_STATE_NORMAL,
				        &white);
	}
	else
	{
		gtk_widget_modify_base (private->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);
		gtk_widget_modify_text (private->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);
	}
}

static gboolean
on_goto_key_pressed (GtkWidget* entry, GdkEventKey* event, SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	switch (event->keyval)
	{
		case GDK_0:
		case GDK_1:
		case GDK_2:
		case GDK_3:
		case GDK_4:
		case GDK_5:
		case GDK_6:
		case GDK_7:
		case GDK_8:
		case GDK_9:
		case GDK_KP_0:
		case GDK_KP_1:
		case GDK_KP_2:
		case GDK_KP_3:
		case GDK_KP_4:
		case GDK_KP_5:
		case GDK_KP_6:
		case GDK_KP_7:
		case GDK_KP_8:
		case GDK_KP_9:
		case GDK_Return:
		case GDK_KP_Enter:
		case GDK_BackSpace:
		case GDK_Delete:
		{
			/* This is a number or enter which is ok */
			break;
		}
		case GDK_Escape:
		{
			gtk_widget_hide (GTK_WIDGET (search_box));
			search_box_set_entry_color (search_box, TRUE);
			if (private->current_editor)
			{
				ianjuta_document_grab_focus (IANJUTA_DOCUMENT (private->current_editor), 
											 NULL);
			}
		}
		default:
		{
			/* Not a number */
			gdk_beep ();
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
on_entry_key_pressed (GtkWidget* entry, GdkEventKey* event, SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	switch (event->keyval)
	{
		case GDK_Escape:
		{
			gtk_widget_hide (GTK_WIDGET (search_box));
			search_box_set_entry_color (search_box, TRUE);
			if (private->current_editor)
			{
				ianjuta_document_grab_focus (IANJUTA_DOCUMENT (private->current_editor), 
											 NULL);
			}
		}
		default:
		{
			/* Do nothing... */
		}
	}
	return FALSE;
}

static gboolean
on_search_focus_out (GtkWidget* widget, GdkEvent* event, SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	if (private->last_start)
	{
		g_object_unref (private->last_start);
		private->last_start = NULL;
	}
	anjuta_status_pop (private->status);

	return FALSE;
}

static void
on_incremental_search (GtkWidget* widget, SearchBox* search_box)
{
	IAnjutaEditorCell* search_start;
	IAnjutaEditorCell* search_end;
	IAnjutaEditorCell* result_start;
	IAnjutaEditorCell* result_end;
	IAnjutaEditorSelection* selection;
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	gboolean case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (private->case_check));
	const gchar* search_text = gtk_entry_get_text (GTK_ENTRY (private->search_entry));
	gboolean found = FALSE;
	
	if (!private->current_editor || !search_text || !strlen (search_text))
		return;
	
	if (private->last_start)
	{
		search_start = IANJUTA_EDITOR_CELL (private->last_start);
	}
	else
	{
		search_start = 
			IANJUTA_EDITOR_CELL (ianjuta_editor_get_position_iter (private->current_editor, 
																   NULL));
	}
	
	search_end = 
		IANJUTA_EDITOR_CELL (ianjuta_iterable_clone (IANJUTA_ITERABLE (search_start), NULL));
	ianjuta_iterable_last (IANJUTA_ITERABLE (search_end), NULL);
	
	if (ianjuta_editor_search_forward (IANJUTA_EDITOR_SEARCH (private->current_editor),
									   search_text, case_sensitive,
									   search_start, search_end, 
									   &result_start,
									   &result_end, NULL))
	{
		found = TRUE;
		anjuta_status_pop (ANJUTA_STATUS (private->status));
	}
	if (found)
	{
		selection = IANJUTA_EDITOR_SELECTION (private->current_editor);
		ianjuta_editor_selection_set_iter (selection,
										   result_start, result_end, NULL);
		g_object_unref (result_start);
		g_object_unref (result_end);
		
		private->last_start = IANJUTA_ITERABLE (search_start);
	}
	else
	{
		g_object_unref (search_start);
		private->last_start = NULL;
	}
	
	search_box_set_entry_color (search_box, found);
	g_object_unref (search_end);	
}

static void
on_search_activated (GtkWidget* widget, SearchBox* search_box)
{
	IAnjutaEditorCell* search_start;
	IAnjutaIterable* real_start;
	IAnjutaEditorCell* search_end;
	IAnjutaEditorCell* result_start;
	IAnjutaEditorCell* result_end;
	IAnjutaEditorSelection* selection;
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	gboolean case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (private->case_check));
	const gchar* search_text = gtk_entry_get_text (GTK_ENTRY (private->search_entry));
	gboolean found = FALSE;
	
	if (!private->current_editor || !search_text || !strlen (search_text))
		return;
	
	search_start = 
		IANJUTA_EDITOR_CELL (ianjuta_editor_get_position_iter (private->current_editor, 
															   NULL));
	real_start =
			ianjuta_iterable_clone (IANJUTA_ITERABLE (search_start), NULL);
	
	search_end = 
		IANJUTA_EDITOR_CELL (ianjuta_iterable_clone (IANJUTA_ITERABLE (search_start), NULL));
	ianjuta_iterable_last (IANJUTA_ITERABLE (search_end), NULL);
	
	selection = IANJUTA_EDITOR_SELECTION (private->current_editor);
	
	/* If a search_result is already selected, move the search start
	 * forward by one
	 */
	if (ianjuta_editor_selection_has_selection (selection,
												NULL))
	{
		IAnjutaIterable* selection_start = 
			ianjuta_editor_selection_get_start_iter (selection, NULL);
		if (ianjuta_iterable_get_position (IANJUTA_ITERABLE (search_start), NULL) ==
			ianjuta_iterable_get_position (selection_start, NULL))
		{
			gchar* selected_text =
				ianjuta_editor_selection_get (selection, NULL);
			if (case_sensitive)
			{
				if (g_str_has_prefix (selected_text, search_text))
				{
					ianjuta_iterable_next (IANJUTA_ITERABLE (search_start), NULL);
				}
			}
			else if (strlen (selected_text) >= strlen (search_text))
			{
				gchar* selected_up = g_utf8_casefold (selected_text, strlen (search_text));
				gchar* search_text_up = g_utf8_casefold (search_text, strlen (search_text));
				if (g_str_equal (selected_up, search_text_up))
				{
					ianjuta_iterable_next (IANJUTA_ITERABLE (search_start), NULL);
				}
				g_free (selected_up);
				g_free (search_text_up);
			}
			g_free (selected_text);
		}
	}
	
	if (ianjuta_editor_search_forward (IANJUTA_EDITOR_SEARCH (private->current_editor),
									   search_text, case_sensitive,
									   search_start, search_end, 
									   &result_start,
									   &result_end, NULL))
	{
		found = TRUE;
		anjuta_status_pop (ANJUTA_STATUS (private->status));
	}
	else
	{
		/* Try to continue on top */
		ianjuta_iterable_first (IANJUTA_ITERABLE (search_start), NULL);
		if (ianjuta_editor_search_forward (IANJUTA_EDITOR_SEARCH (private->current_editor),
										   search_text, case_sensitive,
										   search_start, search_end, 
										   &result_start,
										   &result_end, NULL))
		{
			if (ianjuta_iterable_get_position (IANJUTA_ITERABLE (result_start), NULL)
				!= ianjuta_iterable_get_position (real_start, NULL))
			{
				found = TRUE;
				anjuta_status_push (private->status, 
									_("Search for \"%s\" reached end and was continued on top!"), search_text);
			}
			else if (ianjuta_editor_selection_has_selection (selection, NULL))
			{
				anjuta_status_pop (private->status);
				anjuta_status_push (private->status, 
									_("Search for \"%s\" reached end and was continued on top but no new match was found!"), search_text);
			}				
		}
	}
	if (found)
	{
		ianjuta_editor_selection_set_iter (selection,
										   result_start, result_end, NULL);
		g_object_unref (result_start);
		g_object_unref (result_end);
	}
	search_box_set_entry_color (search_box, found);
	g_object_unref (real_start);
	g_object_unref (search_end);
	
	// We end the incremental search here
	
	if (private->last_start)
	{
		g_object_unref (private->last_start);
		private->last_start = NULL;
	}
	else
	{
		g_object_unref (search_start);
	}
}
	
static void
search_box_init (SearchBox *object)
{
	SearchBoxPrivate* private = GET_PRIVATE(object);
	
	/* Button images */
	GtkWidget* close = 
		gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	GtkWidget* search =
		gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
	GtkWidget* goto_image = 
		gtk_image_new_from_stock (ANJUTA_STOCK_GOTO_LINE, GTK_ICON_SIZE_SMALL_TOOLBAR);		
	
	/* Searching */
	private->search_entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (private->search_entry), "activate", 
					  G_CALLBACK (on_search_activated),
					  object);	
	g_signal_connect (G_OBJECT (private->search_entry), "key-press-event",
					  G_CALLBACK (on_entry_key_pressed),
					  object);
	g_signal_connect (G_OBJECT (private->search_entry), "changed",
					  G_CALLBACK (on_incremental_search),
					  object);
	g_signal_connect (G_OBJECT (private->search_entry), "focus-out-event",
					  G_CALLBACK (on_search_focus_out),
					  object);	
	
	private->case_check = gtk_check_button_new_with_label (_("Case sensitive"));
	
	private->search_button = gtk_button_new ();
	gtk_button_set_image (GTK_BUTTON (private->search_button), search);
	gtk_button_set_relief (GTK_BUTTON (private->search_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (private->search_button), "clicked", 
					  G_CALLBACK (on_search_activated),
					  object);
	
	
	private->close_button = gtk_button_new();
	gtk_button_set_image (GTK_BUTTON (private->close_button), close);
	gtk_button_set_relief (GTK_BUTTON (private->close_button), GTK_RELIEF_NONE);
	
	g_signal_connect (G_OBJECT (private->close_button), "clicked",
					  G_CALLBACK (on_search_box_hide), object);
		
	/* Goto line */
	private->goto_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (private->goto_entry), 5);
	g_signal_connect (G_OBJECT (private->goto_entry), "activate", 
					  G_CALLBACK (on_goto_activated),
					  object);
	g_signal_connect (G_OBJECT (private->goto_entry), "key-press-event",
					  G_CALLBACK (on_goto_key_pressed),
					  object);
	private->goto_button = gtk_button_new();
	gtk_button_set_image (GTK_BUTTON (private->goto_button), goto_image);
	gtk_button_set_relief (GTK_BUTTON (private->goto_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (private->goto_button), "clicked", 
					  G_CALLBACK (on_goto_activated),
					  object);

	gtk_box_pack_start (GTK_BOX (object), private->goto_entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (object), private->goto_button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (object), private->search_entry, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (object), private->search_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (object), private->case_check, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (object), private->close_button, FALSE, FALSE, 0);
	
	gtk_widget_show_all (GTK_WIDGET (object));
	
	private->last_start = NULL;
}

static void
search_box_finalize (GObject *object)
{

	G_OBJECT_CLASS (search_box_parent_class)->finalize (object);
}

static void
search_box_class_init (SearchBoxClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SearchBoxPrivate));
	
	object_class->finalize = search_box_finalize;
}

GtkWidget*
search_box_new (AnjutaDocman *docman)
{
	GtkWidget* search_box;
	SearchBoxPrivate* private;

	search_box = GTK_WIDGET (g_object_new (SEARCH_TYPE_BOX, "homogeneous",
											FALSE, NULL));
	g_signal_connect (G_OBJECT (docman), "document-changed",
					  G_CALLBACK (on_document_changed), search_box);

	private = GET_PRIVATE (search_box);
	private->status = anjuta_shell_get_status (docman->shell, NULL);

	return search_box;
}

void
search_box_grab_search_focus (SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	gtk_widget_grab_focus (private->search_entry);
}

void
search_box_grab_line_focus (SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	gtk_widget_grab_focus (private->goto_entry);
}
