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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "sourceview-cell.h"

#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-cell-style.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#include <gtk/gtk.h>
#include <string.h>
#include <gtksourceview/gtksourcebuffer.h>

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
	obj->priv = g_slice_new(SourceviewCellPrivate);
	
	/* Initialize private members, etc. */	
}

static void
sourceview_cell_finalize(GObject *object)
{
	SourceviewCell *cobj;
	cobj = SOURCEVIEW_CELL(object);
	
	g_slice_free(SourceviewCellPrivate, cobj->priv);
	G_OBJECT_CLASS(sourceview_cell_parent_class)->finalize(object);
}

SourceviewCell *
sourceview_cell_new(GtkTextIter* iter, GtkTextView* view)
{
	SourceviewCell *obj;
	
	obj = SOURCEVIEW_CELL(g_object_new(SOURCEVIEW_TYPE_CELL, NULL));
	
	obj->priv->buffer = gtk_text_view_get_buffer(view);
	obj->priv->mark = gtk_text_buffer_create_mark (obj->priv->buffer, NULL, iter, FALSE);
	obj->priv->view = view;
	
	return obj;
}

void
sourceview_cell_get_iter (SourceviewCell* cell, GtkTextIter* iter)
{
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, iter, cell->priv->mark);
}

static gchar*
icell_get_character(IAnjutaEditorCell* icell, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(icell);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);	
	gunichar c = gtk_text_iter_get_char (&iter);
	gchar* outbuf = g_new0(gchar, 6);
	g_unichar_to_utf8 (c, outbuf);
	return outbuf;
}

static gint 
icell_get_length(IAnjutaEditorCell* icell, GError** e)
{
	gchar* text = icell_get_character(icell, e);
	gint retval = 0;
	if (text)
		retval = g_utf8_strlen (text, -1);
	g_free(text);
	return retval;
}

static gchar
icell_get_char(IAnjutaEditorCell* icell, gint index, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(icell);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gunichar c = gtk_text_iter_get_char (&iter);
	gchar* outbuf = g_new0(gchar, 6);
	gint len = g_unichar_to_utf8 (c, outbuf);
	gchar retval;
	if (index < len)
		retval = outbuf[index];
	else
		retval = 0;
	g_free (outbuf);
	return retval;
}

static IAnjutaEditorAttribute
icell_get_attribute (IAnjutaEditorCell* icell, GError **e)
{
	IAnjutaEditorAttribute attrib = IANJUTA_EDITOR_TEXT;
	SourceviewCell* cell = SOURCEVIEW_CELL(icell);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);

	if (gtk_source_buffer_iter_has_context_class (GTK_SOURCE_BUFFER(cell->priv->buffer), &iter, "string"))
		attrib = IANJUTA_EDITOR_STRING;
	else if (gtk_source_buffer_iter_has_context_class (GTK_SOURCE_BUFFER(cell->priv->buffer), &iter, "comment"))
		attrib = IANJUTA_EDITOR_COMMENT;
	else if (gtk_source_buffer_iter_has_context_class (GTK_SOURCE_BUFFER(cell->priv->buffer), &iter, "keyword"))
		attrib = IANJUTA_EDITOR_KEYWORD;
	
	return attrib;
}

static void
icell_iface_init(IAnjutaEditorCellIface* iface)
{
	iface->get_character = icell_get_character;
	iface->get_char = icell_get_char;
	iface->get_length = icell_get_length;
	iface->get_attribute = icell_get_attribute;
}

static gboolean
iiter_first(IAnjutaIterable* iiter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gboolean retval = gtk_text_iter_is_start (&iter);
	if (!retval)
	{
		gtk_text_iter_set_offset (&iter, 0);
		gtk_text_buffer_move_mark (cell->priv->buffer, cell->priv->mark, &iter);
	}
	return retval;
}

static gboolean
iiter_next(IAnjutaIterable* iiter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gboolean retval = gtk_text_iter_forward_char(&iter);
	if (retval)
			gtk_text_buffer_move_mark (cell->priv->buffer, cell->priv->mark, &iter);
	return retval;
}

static gboolean
iiter_previous(IAnjutaIterable* iiter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gboolean retval = gtk_text_iter_backward_char(&iter);
	if (retval)
			gtk_text_buffer_move_mark (cell->priv->buffer, cell->priv->mark, &iter);
	return retval;
}

static gboolean
iiter_last(IAnjutaIterable* iiter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gboolean retval = gtk_text_iter_is_end (&iter);
	if (!retval)
	{
		gtk_text_iter_forward_to_end(&iter);
		gtk_text_buffer_move_mark (cell->priv->buffer, cell->priv->mark, &iter);
	}
	return retval;
}

static void
iiter_foreach(IAnjutaIterable* iiter, GFunc callback, gpointer data, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	GtkTextMark* cached = gtk_text_buffer_create_mark (cell->priv->buffer, NULL, &iter, FALSE);
	
	iiter_first (IANJUTA_ITERABLE(cell), NULL);
	
	while (iiter_next(IANJUTA_ITERABLE(cell), NULL))
	{
		(*callback)(cell, data);
	}
	gtk_text_buffer_delete_mark (cell->priv->buffer, cell->priv->mark);
	cell->priv->mark = cached;
}

static gboolean
iiter_set_position (IAnjutaIterable* iiter, gint position, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gtk_text_iter_set_offset(&iter, position);
	gtk_text_buffer_move_mark (cell->priv->buffer, cell->priv->mark, &iter);
	return TRUE;
}

static gint
iiter_get_position(IAnjutaIterable* iiter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	return gtk_text_iter_get_offset(&iter);
}

static gint
iiter_get_length(IAnjutaIterable* iiter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	
	return gtk_text_buffer_get_char_count (cell->priv->buffer);
}

static IAnjutaIterable *
iiter_clone (IAnjutaIterable *iiter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	return IANJUTA_ITERABLE (sourceview_cell_new (&iter, cell->priv->view));
}

static void
iiter_assign (IAnjutaIterable *iiter, IAnjutaIterable *src_iter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	SourceviewCell* src_cell = SOURCEVIEW_CELL(src_iter);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (src_cell->priv->buffer, &iter, src_cell->priv->mark);
	gtk_text_buffer_move_mark (cell->priv->buffer, cell->priv->mark, &iter);
}

static gint
iiter_compare (IAnjutaIterable *iiter, IAnjutaIterable *iother_iter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	SourceviewCell* other_cell = SOURCEVIEW_CELL(iother_iter);
	GtkTextIter iter;
	GtkTextIter other_iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gtk_text_buffer_get_iter_at_mark (other_cell->priv->buffer, &other_iter, other_cell->priv->mark);

	return gtk_text_iter_compare (&iter, &other_iter);
}

static gint
iiter_diff (IAnjutaIterable *iiter, IAnjutaIterable *iother_iter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iiter);
	SourceviewCell* other_cell = SOURCEVIEW_CELL(iother_iter);

	GtkTextIter iter;
	GtkTextIter other_iter;
	gtk_text_buffer_get_iter_at_mark (cell->priv->buffer, &iter, cell->priv->mark);
	gtk_text_buffer_get_iter_at_mark (other_cell->priv->buffer, &other_iter, other_cell->priv->mark);
	
	return (gtk_text_iter_get_offset (&other_iter) 
					- gtk_text_iter_get_offset (&iter));
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
	iface->assign = iiter_assign;
	iface->clone = iiter_clone;
	iface->diff = iiter_diff;
	iface->compare = iiter_compare;
}


ANJUTA_TYPE_BEGIN(SourceviewCell, sourceview_cell, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE(icell, IANJUTA_TYPE_EDITOR_CELL);
ANJUTA_TYPE_ADD_INTERFACE(iiter, IANJUTA_TYPE_ITERABLE);
ANJUTA_TYPE_END;
