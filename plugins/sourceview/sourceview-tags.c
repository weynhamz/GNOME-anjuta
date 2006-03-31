/*
 * sourceview-tooltip.c (c) 2006 Johannes Schmid
 * Based on the Python-Code from Guillaume Chazarain
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "sourceview-autocomplete.h"
#include "sourceview-private.h"
#include "sourceview-tags.h"
#include "tag-window.h"
#include "tm_tagmanager.h"

#include <libanjuta/interfaces/ianjuta-editor.h> 
#include <libanjuta/anjuta-debug.h>

#include <gdk/gdkkeysyms.h>

static const GPtrArray*
find_tags (Sourceview* sv)
{
	TMTagAttrType attrs[] =
	{
		tm_tag_attr_name_t, tm_tag_attr_type_t, tm_tag_attr_none_t
	};
	
	g_return_val_if_fail(sv->priv->tags->current_word != NULL, NULL);
	
	return tm_workspace_find(sv->priv->tags->current_word,
											  tm_tag_enumerator_t |
											  tm_tag_struct_t |
											  tm_tag_class_t |
											  tm_tag_prototype_t |
											  tm_tag_function_t |
											  tm_tag_macro_with_arg_t,
											  attrs, TRUE, TRUE);
}

/* Return a tuple containing the (x, y) position of the cursor */
static void
get_coordinates(Sourceview* sv, int* x, int* y)
{
	int xor, yor;
	/* We need to Rectangles because if we step to the next line
	the x position is lost */
	GdkRectangle rectx;
	GdkRectangle recty;
	GdkWindow* window;
	GtkTextIter cursor;
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextView* view = GTK_TEXT_VIEW(sv->priv->view);
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer)); 
	gtk_text_view_get_iter_location(view, &cursor, &rectx);
	gtk_text_iter_forward_lines(&cursor, 1);
	gtk_text_view_get_iter_location(view, &cursor, &recty);
	window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT, 
		rectx.x + rectx.width, recty.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
}

static void
tag_selected(GtkWidget* window, gchar* tag_name, Sourceview* sv)
{
	GtkTextBuffer * buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter cursor_iter, *word_iter;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(buffer));
	word_iter = gtk_text_iter_copy(&cursor_iter);
	gtk_text_iter_set_line_index(word_iter, 
		gtk_text_iter_get_line_index(&cursor_iter) - strlen(sv->priv->tags->current_word));
	gtk_text_buffer_delete(buffer, word_iter, &cursor_iter);
	gtk_text_buffer_insert_at_cursor(buffer, tag_name, 
		strlen(tag_name));
		
	g_free(sv->priv->tags->current_word);
	sv->priv->tags->current_word = NULL;
	sv->priv->tags->tag_window = NULL;
	gtk_text_iter_free(word_iter);
}

static void
update_tags_window(Sourceview* sv)
{
	const GPtrArray* tags = find_tags(sv);
	
	if (tags && tags->len)
	{
		tag_window_update(sv->priv->tags->tag_window, tags);
	}
	else
		sourceview_tags_stop(sv);
}

static void
show_tags_window(Sourceview* sv)
{
	GtkWidget* tag_window;
	gint x, y;
	const GPtrArray* tags = find_tags(sv);
	
	if (tags && tags->len)
	{
		get_coordinates(sv, &x, &y);
		tag_window = tag_window_new(tags);
	
		g_signal_connect(G_OBJECT(tag_window), "selected", G_CALLBACK(tag_selected), sv);
	
		gtk_window_move(GTK_WINDOW(tag_window), x, y);
		gtk_widget_show_all(tag_window);
		sv->priv->tags->tag_window = TAG_WINDOW(tag_window);
	}
}

void sourceview_tags_show(Sourceview* sv)
{
	GtkTextBuffer * buffer = GTK_TEXT_BUFFER(sv->priv->document);
	
	if (sv->priv->tags->current_word != NULL)
		g_free(sv->priv->tags->current_word);
	
	sv->priv->tags->current_word = sourceview_autocomplete_get_current_word(buffer);
	if (sv->priv->tags->current_word == NULL || strlen(sv->priv->tags->current_word) <= 3)
	{
		return;
	}
	else if (sv->priv->tags->tag_window != NULL)
	{
		update_tags_window(sv);
	}
	else
	{
		show_tags_window(sv);
	}
}

void sourceview_tags_init(Sourceview* sv)
{
	sv->priv->tags = g_new0(SourceviewTags, 1);
}

void sourceview_tags_destroy(Sourceview* sv)
{
	g_free(sv->priv->tags);
}

void sourceview_tags_stop(Sourceview* sv)
{
	if (sv->priv->tags->tag_window != NULL)
	{
		gdk_keyboard_ungrab(GDK_CURRENT_TIME);
		gtk_widget_hide(GTK_WIDGET(sv->priv->tags->tag_window));
		gtk_widget_destroy(GTK_WIDGET(sv->priv->tags->tag_window));
		sv->priv->tags->tag_window = NULL;
	}
}

gboolean
sourceview_tags_up(Sourceview* sv)
{
	if (sv->priv->tags->tag_window != NULL)
	{
		return tag_window_up(sv->priv->tags->tag_window);
	}
	return FALSE;
}

gboolean
sourceview_tags_down(Sourceview* sv)
{
	if (sv->priv->tags->tag_window != NULL)
	{
		tag_window_down(sv->priv->tags->tag_window);
		return TRUE;
	}
	return FALSE;
}

gboolean
sourceview_tags_select(Sourceview* sv)
{
	if (sv->priv->tags->tag_window != NULL)
	{
		return tag_window_select(sv->priv->tags->tag_window);
	}
	return FALSE;
}
