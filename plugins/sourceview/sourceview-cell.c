/***************************************************************************
 *            sourceview-cell.c
 *
 *  So Mai 21 14:44:13 2006
 *  Copyright  2006  Johannes Schmid
 *  jhs@cvs.gnome.org
 ***************************************************************************/

/*
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

#include "sourceview-cell.h"

#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-cell-style.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#include <gtk/gtktextview.h>
#include <string.h>
 
static void sourceview_cell_class_init(SourceviewCellClass *klass);
static void sourceview_cell_instance_init(SourceviewCell *sp);
static void sourceview_cell_finalize(GObject *object);

struct _SourceviewCellPrivate {
	GtkTextMark* mark;
	GtkTextView* view;
	GtkTextBuffer* buffer;
};

static gpointer sourceview_cell_parent_class = NULL;

static void
sourceview_cell_class_init(SourceviewCellClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	sourceview_cell_parent_class = g_type_class_peek_parent(klass);


	object_class->finalize = sourceview_cell_finalize;
}

static void
sourceview_cell_instance_init(SourceviewCell *obj)
{
	obj->priv = g_new0(SourceviewCellPrivate, 1);
	/* Initialize private members, etc. */
}

static void
sourceview_cell_finalize(GObject *object)
{
	SourceviewCell *cobj;
	cobj = SOURCEVIEW_CELL(object);
	
	gtk_text_buffer_delete_mark(cobj->priv->buffer, cobj->priv->mark);
	
	g_free(cobj->priv);
	G_OBJECT_CLASS(sourceview_cell_parent_class)->finalize(object);
}

SourceviewCell *
sourceview_cell_new(GtkTextIter* iter, GtkTextView* view)
{
	SourceviewCell *obj;
	
	obj = SOURCEVIEW_CELL(g_object_new(SOURCEVIEW_TYPE_CELL, NULL));
	
	obj->priv->buffer = gtk_text_view_get_buffer(view);
	obj->priv->mark = gtk_text_buffer_create_mark(obj->priv->buffer,
												  NULL,
												  iter,
												  FALSE);
	obj->priv->view = view;
	
	return obj;
}

static gchar*
icell_get_character(IAnjutaEditorCell* icell, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(icell);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &iter,
									 cell->priv->mark);
	GtkTextIter* iter_prev = gtk_text_iter_copy(&iter);
	if (gtk_text_iter_backward_char(iter_prev))
	{
		return gtk_text_iter_get_text(&iter, iter_prev);
	}
	else
		return strdup("");
}

static gint 
icell_get_length(IAnjutaEditorCell* icell, GError** e)
{
	/* FIXME: This is not efficient */
	gint length;
	gchar* s = icell_get_character(icell, NULL);
	length = strlen(s);
	g_free (s);
	return length;
}

static gchar
icell_get_char(IAnjutaEditorCell* icell, gint index, GError** e)
{
	gchar* characters = icell_get_character(icell, NULL);
	
	if (strlen(characters) > index)
		return characters[index];
	else
	{
		return '\0';
	}
}

static void
icell_iface_init(IAnjutaEditorCellIface* iface)
{
	iface->get_character = icell_get_character;
	iface->get_char = icell_get_char;
	iface->get_length = icell_get_length;
}

static
GtkTextAttributes* get_attributes(GtkTextIter* iter, GtkTextView* view)
{
	GtkTextAttributes* atts = gtk_text_view_get_default_attributes(view);
	gtk_text_iter_get_attributes(iter, atts);
	return atts;
}

static gchar*
icell_style_get_font_description(IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	const gchar* font;
	SourceviewCell* cell = SOURCEVIEW_CELL(icell_style);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &iter,
									 cell->priv->mark);
	GtkTextAttributes* atts = get_attributes(&iter, 
											 cell->priv->view);
	font = pango_font_description_to_string(atts->font);
	g_free(atts);
	return g_strdup(font);
}

static gchar*
icell_style_get_color(IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	gchar* color;
	SourceviewCell* cell = SOURCEVIEW_CELL(icell_style);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &iter,
									 cell->priv->mark);
	GtkTextAttributes* atts = get_attributes(&iter, cell->priv->view);
	color = anjuta_util_string_from_color(atts->appearance.fg_color.red,
		atts->appearance.fg_color.green, atts->appearance.fg_color.blue);
	g_free(atts);
	return color;
}

static gchar*
icell_style_get_background_color(IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	gchar* color;
	SourceviewCell* cell = SOURCEVIEW_CELL(icell_style);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &iter,
									 cell->priv->mark);
	GtkTextAttributes* atts = get_attributes(&iter, cell->priv->view);
	color = anjuta_util_string_from_color(atts->appearance.bg_color.red,
		atts->appearance.bg_color.green, atts->appearance.bg_color.blue);
	g_free(atts);
	return color;
}

static void
icell_style_iface_init(IAnjutaEditorCellStyleIface* iface)
{
	iface->get_font_description = icell_style_get_font_description;
	iface->get_color = icell_style_get_color;
	iface->get_background_color = icell_style_get_background_color;
}

static gboolean
iiter_first(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	GtkTextIter text_iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &text_iter,
									 cell->priv->mark);
	gtk_text_iter_set_offset(&text_iter, 0);
	gtk_text_buffer_move_mark(cell->priv->buffer, cell->priv->mark, &text_iter);
	return TRUE;
}

static gboolean
iiter_next(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	GtkTextIter text_iter;
	
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &text_iter,
									 cell->priv->mark);

	if (gtk_text_iter_forward_char(&text_iter))
	{
		gtk_text_buffer_move_mark(cell->priv->buffer, cell->priv->mark, &text_iter);
		return TRUE;
	}
	else
		return FALSE;
}

static gboolean
iiter_previous(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	GtkTextIter text_iter;
	
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &text_iter,
									 cell->priv->mark);

	if (gtk_text_iter_backward_char(&text_iter))
	{
		gtk_text_buffer_move_mark(cell->priv->buffer, cell->priv->mark, &text_iter);
		return TRUE;
	}
	else
		return FALSE;
}

static gboolean
iiter_last(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	GtkTextIter text_iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &text_iter,
									 cell->priv->mark);

	gtk_text_iter_forward_to_end(&text_iter);
	iiter_previous (iter, NULL);
	return TRUE;
}

static void
iiter_foreach(IAnjutaIterable* iter, GFunc callback, gpointer data, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	
	GtkTextIter text_iter;
	gint saved_offset;
	
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &text_iter,
									 cell->priv->mark);
	saved_offset = gtk_text_iter_get_offset (&iter);
	gtk_text_iter_set_offset(&text_iter, 0);
	while (gtk_text_iter_forward_char(&text_iter))
	{
		gtk_text_buffer_move_mark(cell->priv->buffer, cell->priv->mark,
								  &text_iter);
		(*callback)(cell, data);
	}
	gtk_text_iter_set_offset(&text_iter, saved_offset);
}

static gboolean
iiter_set_position (IAnjutaIterable* iter, gint position, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	GtkTextIter old_iter;

	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &old_iter,
									 cell->priv->mark);
	gtk_text_iter_set_offset (&old_iter, position);
	return TRUE;
}

static gint
iiter_get_position(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	GtkTextIter text_iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &text_iter,
									 cell->priv->mark);
	return gtk_text_iter_get_offset(&text_iter);
}

static gint
iiter_get_length(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	GtkTextIter text_iter;
	gtk_text_buffer_get_iter_at_mark(cell->priv->buffer, &text_iter,
									 cell->priv->mark);
	
	return gtk_text_buffer_get_char_count (gtk_text_iter_get_buffer(&text_iter));
}

static void
iiter_iface_init(IAnjutaIterableIface* iface)
{
	iface->first = iiter_first;
	iface->next = iiter_next;
	iface->previous = iiter_previous;
	iface->last = iiter_last;
	iface->foreach = iiter_foreach;
	iface->set_position = iiter_set_position;
	iface->get_position = iiter_get_position;
	iface->get_length = iiter_get_length;
}


ANJUTA_TYPE_BEGIN(SourceviewCell, sourceview_cell, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE(icell, IANJUTA_TYPE_EDITOR_CELL);
ANJUTA_TYPE_ADD_INTERFACE(icell_style, IANJUTA_TYPE_EDITOR_CELL_STYLE);
ANJUTA_TYPE_ADD_INTERFACE(iiter, IANJUTA_TYPE_ITERABLE);
ANJUTA_TYPE_END;
