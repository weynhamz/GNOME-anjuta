/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * text-editor-iterable.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-cell-style.h>

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1

#include <gtk/gtk.h>
#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "properties.h"
#include "text-editor-iterable.h"

#define TEXT_CELL_FONT_BODY_DEFAULT   "courier"
#define TEXT_CELL_FONT_HEADER_DEFAULT "helvetica"
#define TEXT_CELL_FONT_SIZE_BODY_DEFAULT   10
#define TEXT_CELL_FONT_SIZE_HEADER_DEFALT 10
#define TEXT_CELL_FONT_SIZE_NUMBERS_DEFAULT 6
#define TEXT_CELL_MAX_STYLES 256
#define TEXT_CELL_LINENUMBER_STYLE 33
#define TEXT_CELL_DEFAULT_TEXT_STYLE 32
#define TEXT_CELL_LINENUM_PADDING '0'

static void text_editor_cell_class_init(TextEditorCellClass *klass);
static void text_editor_cell_instance_init(TextEditorCell *sp);
static void text_editor_cell_finalize(GObject *object);

static gpointer parent_class;

typedef struct _CellStyle
{
	gchar          *font_desc;

	gchar          *font_name;
	gboolean        italics;
	gboolean        bold;
	gint            size;
	
	GdkColor        fore_color;
	GdkColor        back_color;

} CellStyle;

struct _TextEditorCellPrivate {
	TextEditor *editor;
	
	/* byte position in editor */
	gint position;
	
	/* Character position */
	/* gint char_position; */
	
	/* Styles cache */
	CellStyle* styles_pool[TEXT_CELL_MAX_STYLES];
};

/* Style processing */

static const CellStyle* text_editor_cell_get_style (TextEditorCell *cell, gint style);

static void
cell_style_destroy (CellStyle *pis) {
	if (pis) {
		if (pis->font_desc) g_free (pis->font_desc);
		if (pis->font_name) g_free (pis->font_name);
		g_free(pis);
	}
}

static int
IntFromHexDigit(const char ch) {
	if (isdigit(ch))
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return 0;
}

static void
string_to_color (const char *val, GdkColor* color)
{
	/* g_print ("Color = %s\n", val); */
	color->red   = IntFromHexDigit(val[1]) * 16 + IntFromHexDigit(val[2]);
	color->green = IntFromHexDigit(val[3]) * 16 + IntFromHexDigit(val[4]);
	color->blue  = IntFromHexDigit(val[5]) * 16 + IntFromHexDigit(val[6]);
}

static void
cell_style_load_font (CellStyle *pis)
{
	gchar *font_desc, *tmp;
	gint size = 12;
	
	g_return_if_fail (pis->font_name);
	
	font_desc = g_strdup (pis->font_name);
	if (pis->bold)
	{
		tmp = font_desc;
		font_desc = g_strconcat (tmp, " Bold", NULL);
		g_free (tmp);
	}
	if (pis->italics)
	{
		tmp = font_desc;
		font_desc = g_strconcat (tmp, " Italic", NULL);
		g_free (tmp);
	}
	if (pis->size > 0)
	{
		size = pis->size;
		tmp = font_desc;
		font_desc = g_strdup_printf ("%s %d", tmp, pis->size);
		g_free (tmp);
	}
	g_free (pis->font_desc);
	pis->font_desc = font_desc;
}

static void
cell_style_init (CellStyle *pis, PropsID prop, gchar* lang, guint style)
{
	gchar *style_key, *style_string, *val, *opt;
	
	style_key = g_strdup_printf ("style.%s.%0d", lang, style);
	style_string = sci_prop_get_expanded (prop, style_key);
	g_free (style_key);
	if (!style_string) return;
	
	val = g_strdup(style_string);
	opt = val;
	
	while (opt) {
		char *cpComma, *colon;
		
		cpComma = strchr(opt, ',');
		if (cpComma)
			*cpComma = '\0';
		colon = strchr(opt, ':');
		if (colon)
			*colon++ = '\0';

		if (0 == strcmp(opt, "italics"))
			pis->italics = TRUE;
		if (0 == strcmp(opt, "notitalics"))
			pis->italics = FALSE;
		if (0 == strcmp(opt, "bold"))
			pis->bold = TRUE;
		if (0 == strcmp(opt, "notbold"))
			pis->bold = FALSE;
		if (0 == strcmp(opt, "font")) {
			g_free (pis->font_name);
			pis->font_name = g_strdup(colon);
		}
		if (0 == strcmp(opt, "fore"))
			string_to_color(colon, &pis->fore_color);
		if (0 == strcmp(opt, "back"))
			string_to_color(colon, &pis->back_color);
		if (0 == strcmp(opt, "size"))
			pis->size = atoi(colon);
		if (cpComma)
			opt = cpComma + 1;
		else
			opt = 0;
	}
	g_free(val);
	g_free(style_string);
}

static CellStyle*
cell_style_new (PropsID prop, gchar* lang, guint style, gint font_zoom_factor)
{
	CellStyle* pis;
	
	g_return_val_if_fail (prop > 0, NULL);
	g_return_val_if_fail (style < 256, NULL);
	
	pis = g_new0 (CellStyle, 1);

	pis->font_name = g_strdup(TEXT_CELL_FONT_BODY_DEFAULT);
	pis->bold = FALSE;
	pis->italics = FALSE;
	pis->size = TEXT_CELL_FONT_SIZE_BODY_DEFAULT;
	
	/* Black */
	pis->fore_color.red = 0;
	pis->fore_color.green = 0;
	pis->fore_color.blue = 0;
	
	/* White */
	pis->back_color.red = (gushort)(-1);
	pis->back_color.green = (gushort)(-1);
	pis->back_color.blue = (gushort)(-1);
	
	/* Set default style first */
	cell_style_init (pis, prop, "*", 32);
	if (lang && strlen(lang) > 0) {
		cell_style_init (pis, prop, lang, 32);
	}
	/* Then the specific style */
	cell_style_init (pis, prop, "*", style);
	if (lang && strlen(lang) > 0) {
		cell_style_init (pis, prop, lang, style);
	}
	
	pis->size += font_zoom_factor;

	cell_style_load_font (pis);
	return pis;
}

static const CellStyle*
text_editor_cell_get_style (TextEditorCell *cell, gint style)
{
	CellStyle* pis;
	
	pis = cell->priv->styles_pool[style];
	
	if (!pis)
	{
		gchar* language; /* should it be freed ?*/
		language = (gchar*) aneditor_command(cell->priv->editor->editor_id,
											 ANE_GETLANGUAGE,0, 0);
		cell_style_new (cell->priv->editor->props_base,
						language, style, cell->priv->editor->zoom_factor);
		cell->priv->styles_pool[style] = pis;
	}
	if (!pis && style != TEXT_CELL_DEFAULT_TEXT_STYLE) {
		return text_editor_cell_get_style (cell, TEXT_CELL_DEFAULT_TEXT_STYLE);
	}
	return pis;
}


/* TextEditorCell implementation */

static void
text_editor_cell_class_init (TextEditorCellClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = text_editor_cell_finalize;
}

static void
text_editor_cell_instance_init (TextEditorCell *obj)
{
	obj->priv = g_new0(TextEditorCellPrivate, 1);
	/* Initialize private members, etc. */
}

static void
text_editor_cell_finalize (GObject *object)
{
	gint i;
	TextEditorCell *cobj;
	cobj = TEXT_EDITOR_CELL(object);

	g_object_unref (cobj->priv->editor);
	
	for (i = 0; i < TEXT_CELL_MAX_STYLES; i++)
	{
		if (cobj->priv->styles_pool[i])
			cell_style_destroy (cobj->priv->styles_pool[i]);
	}
	g_free(cobj->priv);
	if (G_OBJECT_CLASS(parent_class)->finalize)
	    G_OBJECT_CLASS(parent_class)->finalize(object);
}

TextEditorCell *
text_editor_cell_new (TextEditor* editor, gint position)
{
	TextEditorCell *obj;
	
	g_return_val_if_fail (IS_TEXT_EDITOR (editor), NULL);
	g_return_val_if_fail (position >= 0, NULL);
	
	obj = TEXT_EDITOR_CELL(g_object_new(TYPE_TEXT_EDITOR_CELL, NULL));
	
	g_object_ref (editor);
	obj->priv->editor = editor;
	text_editor_cell_set_position (obj, position);
	return obj;
}

TextEditor*
text_editor_cell_get_editor (TextEditorCell *cell)
{
	g_return_val_if_fail (IS_TEXT_EDITOR_CELL(cell), NULL);
	return cell->priv->editor;
}

void
text_editor_cell_set_position (TextEditorCell *cell, gint position)
{
	guchar ch;
	g_return_if_fail (IS_TEXT_EDITOR_CELL(cell));
	g_return_if_fail (position >= 0);
	
	cell->priv->position = position;
	
	/* Ensure that utf character is properly aligned */
    ch = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
										   SCI_GETCHARAT,
										   position, 0);
	/* DEBUG_PRINT ("Iterator position set at %d where char '%c' is found", position, ch); */
	if ((ch >= 0x80) && (ch < (0x80 + 0x40)))
	{
		/* un-aligned. Align it */
		cell->priv->position = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
													   SCI_POSITIONBEFORE,
													   position,
													   0);
	}
}

gint
text_editor_cell_get_position (TextEditorCell *cell)
{
	g_return_val_if_fail (IS_TEXT_EDITOR_CELL(cell), -1);
	return cell->priv->position;
}

/* IAnjutaIterable implementation */

static gchar*
icell_get_character (IAnjutaEditorCell* icell, GError** e)
{
	gint position_end;
	TextEditorCell* cell = TEXT_EDITOR_CELL(icell);
    position_end = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
										   SCI_POSITIONAFTER,
										   cell->priv->position, 0);
	return (gchar*) scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
											SCI_GETTEXT, cell->priv->position,
											position_end);
}

static gint 
icell_get_length (IAnjutaEditorCell* icell, GError** e)
{
	gint position_end;
	TextEditorCell* cell = TEXT_EDITOR_CELL(icell);
    position_end = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
									  SCI_POSITIONAFTER,
									  cell->priv->position, 0);
	return position_end - cell->priv->position;
}

static gchar
icell_get_char (IAnjutaEditorCell* icell, gint index, GError** e)
{
	gint c;
	TextEditorCell* cell = TEXT_EDITOR_CELL(icell);
	c = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
						   SCI_GETCHARAT, cell->priv->position, 0);
	return (gchar) (c);
}

static IAnjutaEditorAttribute
icell_get_attribute (IAnjutaEditorCell *icell, GError **e)
{
	TextEditorCell* cell = TEXT_EDITOR_CELL(icell);
	IAnjutaEditorAttribute attrib = IANJUTA_EDITOR_TEXT;
	TextEditorAttrib text_attrib =
		text_editor_get_attribute (cell->priv->editor, cell->priv->position);
	switch (text_attrib)
	{
		case TEXT_EDITOR_ATTRIB_TEXT:
			attrib = IANJUTA_EDITOR_TEXT;
			break;
		case TEXT_EDITOR_ATTRIB_COMMENT:
			attrib = IANJUTA_EDITOR_COMMENT;
			break;
		case TEXT_EDITOR_ATTRIB_KEYWORD:
			attrib = IANJUTA_EDITOR_KEYWORD;
			break;
		case TEXT_EDITOR_ATTRIB_STRING:
			attrib = IANJUTA_EDITOR_STRING;
			break;
	}
	return attrib;
}

static void
icell_iface_init (IAnjutaEditorCellIface* iface)
{
	iface->get_character = icell_get_character;
	iface->get_char = icell_get_char;
	iface->get_attribute = icell_get_attribute;
	iface->get_length = icell_get_length;
}

static gchar*
icell_style_get_font_description (IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	gint style;
	const CellStyle *cell_style;
	
	TextEditorCell* cell = TEXT_EDITOR_CELL(icell_style);
	style = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
									SCI_GETSTYLEAT, cell->priv->position, 0);
	cell_style = text_editor_cell_get_style (cell, style);
	
	return g_strdup (cell_style->font_desc);
}

static gchar*
icell_style_get_color (IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	gint style;
	const CellStyle *cell_style;
	
	TextEditorCell* cell = TEXT_EDITOR_CELL(icell_style);
	style = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
									SCI_GETSTYLEAT, cell->priv->position, 0);
	cell_style = text_editor_cell_get_style (cell, style);
	
	return anjuta_util_string_from_color (cell_style->fore_color.red,
										  cell_style->fore_color.green,
										  cell_style->fore_color.blue);
}

static gchar*
icell_style_get_background_color (IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	gint style;
	const CellStyle *cell_style;
	
	TextEditorCell* cell = TEXT_EDITOR_CELL(icell_style);
	style = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
									SCI_GETSTYLEAT, cell->priv->position, 0);
	cell_style = text_editor_cell_get_style (cell, style);
	
	return anjuta_util_string_from_color (cell_style->back_color.red,
										  cell_style->back_color.green,
										  cell_style->back_color.blue);
}

static void
icell_style_iface_init (IAnjutaEditorCellStyleIface* iface)
{
	iface->get_font_description = icell_style_get_font_description;
	iface->get_color = icell_style_get_color;
	iface->get_background_color = icell_style_get_background_color;
}

/* IAnjutaIterable implementation */

static gboolean
iiter_first (IAnjutaIterable* iter, GError** e)
{
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);
	cell->priv->position = 0;
	return TRUE;
}

static gboolean
iiter_next (IAnjutaIterable* iter, GError** e)
{
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);
	gint old_position;
	
	old_position = cell->priv->position;
	cell->priv->position = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
												   SCI_POSITIONAFTER, old_position,
												   0);	
	if (old_position == cell->priv->position)
		return FALSE;
	return TRUE;
}

static gboolean
iiter_previous (IAnjutaIterable* iter, GError** e)
{
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);
	/*
	gint saved_pos = cell->priv->position;
	*/
	
	if (cell->priv->position <= 0)
	{
		return FALSE;
	}
	cell->priv->position = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
												   SCI_POSITIONBEFORE,
												   cell->priv->position,
												   0);
	/*
	DEBUG_PRINT ("Iterator position changed from %d to %d", saved_pos,
				 cell->priv->position);
	*/
	return TRUE;
}

static gboolean
iiter_last (IAnjutaIterable* iter, GError** e)
{
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);
	cell->priv->position =
		scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
								SCI_GETLENGTH, 0, 0);
	return TRUE;
}

static void
iiter_foreach (IAnjutaIterable* iter, GFunc callback, gpointer data, GError** e)
{
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);
	gint saved;
	
	/* Save current position */
	saved = cell->priv->position;
	cell->priv->position = 0;
	while (ianjuta_iterable_next (iter, NULL))
	{
		(*callback)(cell, data);
	}
	
	/* Restore current position */
	cell->priv->position = saved;
}

static gboolean
iiter_set_position (IAnjutaIterable* iter, gint position, GError** e)
{
	gboolean within_range = TRUE;
	gint new_byte_position = 0;
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);

	if (position > 0)
	{
		const gchar *buffer;
		glong length;

		buffer = (const gchar *)scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla), SCI_GETCHARACTERPOINTER, 0, 0);
		length = g_utf8_strlen (buffer, -1);

		if (position < length)	
		{
			gchar *pos;

			pos = g_utf8_offset_to_pointer (buffer, position);
			new_byte_position = pos - buffer;
		}
		else
		{
			position = -1;
			within_range = FALSE;
		}
	}
	
	if (position < 0)
	{
		/* Set to end-iter (length of the doc) */
		new_byte_position =
			scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
									SCI_GETLENGTH, 0, 0);
	}

	cell->priv->position = new_byte_position;
	DEBUG_PRINT ("Editor byte position set at: %d", cell->priv->position);
	return within_range;
}

static gint
iiter_get_position (IAnjutaIterable* iter, GError** e)
{
	gint char_position = 0;
	
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);
	
	/* FIXME: Find a more optimal solution */
	if (cell->priv->position > 0)
	{
		gchar *data =
			(gchar *) aneditor_command (TEXT_EDITOR
										(cell->priv->editor)->editor_id,
										ANE_GETTEXTRANGE, 0,
										cell->priv->position);
		char_position = g_utf8_strlen (data, -1);
		g_free (data);
	}
	DEBUG_PRINT ("Byte pos = %d, char position = %d", cell->priv->position,
				 char_position);
	return char_position;
}

static gint
iiter_get_length (IAnjutaIterable* iter, GError** e)
{
	gint byte_length;
	
	TextEditorCell* cell = TEXT_EDITOR_CELL(iter);
	
	/* FIXME: Find a more optimal solution */
	byte_length = scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
										  SCI_GETLENGTH, 0, 0);
	if (byte_length > 0)
	{
		gchar *data;
		gint char_length;
		
		data = (gchar *) aneditor_command (TEXT_EDITOR
										   (cell->priv->editor)->editor_id,
										   ANE_GETTEXTRANGE, 0, byte_length);
		char_length = g_utf8_strlen (data, -1);
		g_free (data);
		return char_length;
	}
	return 0;
}

static IAnjutaIterable *
iiter_clone (IAnjutaIterable *iter, GError **e)
{
	TextEditorCell *src = TEXT_EDITOR_CELL (iter);
	TextEditorCell *cell = text_editor_cell_new (src->priv->editor,
												 src->priv->position);
	return IANJUTA_ITERABLE (cell);
}

static void
iiter_assign (IAnjutaIterable *iter, IAnjutaIterable *src_iter, GError **e)
{
	TextEditorCell *cell = TEXT_EDITOR_CELL (iter);
	TextEditorCell *src = TEXT_EDITOR_CELL (src_iter);
	cell->priv->editor = src->priv->editor;
	cell->priv->position = src->priv->position;
}

static gint
iiter_compare (IAnjutaIterable *iter, IAnjutaIterable *iter2, GError **e)
{
	gint delta;
	TextEditorCell *cell = TEXT_EDITOR_CELL (iter);
	TextEditorCell *cell2 = TEXT_EDITOR_CELL (iter2);
	delta = cell->priv->position - cell2->priv->position;
	return (delta == 0)? 0 : ((delta > 0)? 1 : -1);
}

static gint
iiter_diff (IAnjutaIterable *iter, IAnjutaIterable *iter2, GError **e)
{
	gint diff = 0;
	TextEditorCell *cell = TEXT_EDITOR_CELL (iter);
	TextEditorCell *cell2 = TEXT_EDITOR_CELL (iter2);
	
	if (cell->priv->position == cell2->priv->position)
	{
		return 0;
	}
	
	/* FIXME: Find more optimal solution */
	/* Iterate until we reach larger iter position */
	if (cell->priv->position > cell2->priv->position)
	{
		gint byte_position = cell2->priv->position;
		while (byte_position < cell->priv->position)
		{
			byte_position =
				scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
										SCI_POSITIONAFTER, byte_position, 0);
			diff--;
		}
	}
	else
	{
		gint byte_position = cell->priv->position;
		while (byte_position < cell2->priv->position)
		{
			byte_position =
				scintilla_send_message (SCINTILLA (cell->priv->editor->scintilla),
										SCI_POSITIONAFTER, byte_position, 0);
			diff++;
		}
	}
	return diff;
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
	iface->clone = iiter_clone;
	iface->assign = iiter_assign;
	iface->compare = iiter_compare;
	iface->diff = iiter_diff;
}

ANJUTA_TYPE_BEGIN(TextEditorCell, text_editor_cell, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE(icell, IANJUTA_TYPE_EDITOR_CELL);
ANJUTA_TYPE_ADD_INTERFACE(icell_style, IANJUTA_TYPE_EDITOR_CELL_STYLE);
ANJUTA_TYPE_ADD_INTERFACE(iiter, IANJUTA_TYPE_ITERABLE);
ANJUTA_TYPE_END;
