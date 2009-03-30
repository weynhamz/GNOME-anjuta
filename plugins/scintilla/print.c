/*
 * print.c
 * 
 * Copyright (C) 2002 Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 * Copyright (C) 2002 Naba Kumar <kh_naba@users.sourceforge.net>
 * Copyright (C) 2008 Sebastien Granjoux <seb.sfo@free.fr>
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
#include <stdlib.h>
#include <string.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-debug.h>

#include "print.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1

#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "print.h"
#include "properties.h"

#define AN_PRINT_FONT_BODY_DEFAULT   "courier"
#define AN_PRINT_FONT_SIZE_BODY_DEFAULT   10
#define AN_PRINT_MAX_STYLES 256
#define AN_PRINT_LINENUMBER_STYLE 33
#define AN_PRINT_DEFAULT_TEXT_STYLE 32
#define AN_PRINT_HEADER_SIZE_FACTOR 2.2
#define AN_PRINT_LINE_NUMBER_SEPARATION 12
#define AN_PRINT_PAGINATION_CHUNK_SIZE 3

typedef struct _PrintJobInfoStyle
{
	PangoFontDescription      *font;
	GList								*attrs;	

	gchar *font_name;
	gboolean italics;
	gboolean bold;
	gint size;
	
	GdkColor fore_color;
	GdkColor back_color;	
	
} PrintJobInfoStyle;

typedef struct _PrintPageInfo
{
	guint pos;
	guint line;
} PrintPageInfo;

typedef struct _PrintJobInfo
{
	TextEditor *te;

	/* Print Buffer */
	gchar  *buffer;
	guint   buffer_size;

	/* Page offset */
	GArray *pages;

	/* Style pool */
	PrintJobInfoStyle* styles_pool[AN_PRINT_MAX_STYLES];
	
	/* Preferences */
	gboolean print_header;
	gboolean print_color;
	gboolean print_line_numbers;
	gboolean wrapping;
	gint     tab_width;
	gint zoom_factor;
	
	/* Margin in points */
	gdouble page_width;
	gdouble page_height;
	gdouble margin_top;
	gdouble margin_bottom;
	gdouble margin_left;
	gdouble margin_right;
	gdouble header_height;
	gdouble numbers_width;
	gdouble numbers_height;
	
	/* GC state */
	guint  current_style;
	guint current_page;
	guint current_pos;
	guint current_line;
	gdouble current_height;
	
	/* layout objects */
	PangoLayout *layout;
	PangoLayout *line_numbers_layout;
	PangoLayout *header_layout;
	
	/* Progress */
	AnjutaStatus* status;
	
} PrintJobInfo;

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
set_layout_tab_width (PangoLayout *layout, gint width)
{
	gchar *str;
	gint tab_width = 0;

	str = g_strnfill (width, ' ');
	pango_layout_set_text (layout, str, -1);
	g_free (str);

	pango_layout_get_size (layout, &tab_width, NULL);

	if (tab_width > 0)
	{
		PangoTabArray *tab_array;
	
		tab_array = pango_tab_array_new (1, FALSE);

		pango_tab_array_set_tab (tab_array,
								 0,
								 PANGO_TAB_LEFT,
								 tab_width);
		pango_layout_set_tabs (layout, tab_array);

		pango_tab_array_free (tab_array);
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
	color->red   = (IntFromHexDigit(val[1]) * 16 + IntFromHexDigit(val[2])) * 256;
	color->green = (IntFromHexDigit(val[3]) * 16 + IntFromHexDigit(val[4])) * 256;
	color->blue  = (IntFromHexDigit(val[5]) * 16 + IntFromHexDigit(val[6])) * 256;
}

/* Style info object
 *---------------------------------------------------------------------------*/

static void
anjuta_print_job_info_style_load_font (PrintJobInfoStyle *pis)
{
	gchar *font_desc, *tmp;
	gint size = 12;
	
	g_return_if_fail (pis->font_name);
	
	/* Setup font */
	font_desc = g_strdup (pis->font_name);
	if (pis->size > 0)
	{
		size = pis->size;
		tmp = font_desc;
		font_desc = g_strdup_printf ("%s %d", tmp, pis->size);
		g_free (tmp);
	}
	if (pis->font)
		pango_font_description_free (pis->font);
	
	DEBUG_PRINT ("Print font loading: %s", font_desc);
	pis->font = pango_font_description_from_string (font_desc);
	g_free (font_desc);
}

static void
anjuta_print_job_info_style_clear_attributes (PrintJobInfoStyle *pis)
{
	if (pis->attrs != NULL)
			g_list_foreach (pis->attrs, (GFunc)pango_attribute_destroy, NULL);
			g_list_free (pis->attrs);
	pis->attrs = NULL;
}

static void
anjuta_print_job_info_style_set_attributes (PrintJobInfoStyle *pis)
{
	PangoAttribute *attr;
	
	/* Use attribute for font shape */
	if (pis->bold)
	{
		attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
		pis->attrs = g_list_prepend (pis->attrs, attr);
	}
	if (pis->italics)
	{
		attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
		pis->attrs = g_list_prepend (pis->attrs, attr);
	}
}

static void
anjuta_print_job_info_style_set_color_attributes (PrintJobInfoStyle *pis)
{
	PangoAttribute *attr;
	
	/* Use attribute for color */
	
	/* Remove background colors */
	/*attr = pango_attr_background_new (pis->back_color.red, pis->back_color.green, pis->back_color.blue);
	pis->attrs = g_list_prepend (pis->attrs, attr);*/
	attr = pango_attr_foreground_new (pis->fore_color.red, pis->fore_color.green, pis->fore_color.blue);
	pis->attrs = g_list_prepend (pis->attrs, attr);
}

static void
anjuta_print_job_info_style_init (PrintJobInfoStyle *pis, PropsID prop,
						gchar* lang, guint style)
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

static void
anjuta_print_job_info_style_destroy (PrintJobInfoStyle *pis) {
	if (pis) {
		if (pis->attrs)
		{
			g_list_foreach (pis->attrs, (GFunc)pango_attribute_destroy, NULL);
			g_list_free (pis->attrs);
		}
		if (pis->font) pango_font_description_free (pis->font);
		if (pis->font_name) g_free (pis->font_name);
		g_free(pis);
	}
}

static PrintJobInfoStyle*
anjuta_print_job_info_style_new (PropsID prop, gchar* lang,
			guint style, gint font_zoom_factor, gboolean color)
{
	PrintJobInfoStyle* pis;
	
	g_return_val_if_fail (prop > 0, NULL);
	g_return_val_if_fail (style < 256, NULL);
	
	pis = g_new0 (PrintJobInfoStyle, 1);

	pis->font = NULL;
	pis->font_name = g_strdup(AN_PRINT_FONT_BODY_DEFAULT);
	pis->bold = FALSE;
	pis->italics = FALSE;
	pis->size = AN_PRINT_FONT_SIZE_BODY_DEFAULT;
	
	/* Black */
	pis->fore_color.red = 0;
	pis->fore_color.green = 0;
	pis->fore_color.blue = 0;
	
	/* White */
	pis->back_color.red = (gushort)(-1);
	pis->back_color.green = (gushort)(-1);
	pis->back_color.blue = (gushort)(-1);
	
	/* Set default style first */
	anjuta_print_job_info_style_init (pis, prop, "*", 32);
	if (lang && strlen(lang) > 0) {
		anjuta_print_job_info_style_init (pis, prop, lang, 32);
	}
	/* Then the specific style */
	anjuta_print_job_info_style_init (pis, prop, "*", style);
	if (lang && strlen(lang) > 0) {
		anjuta_print_job_info_style_init (pis, prop, lang, style);
	}
	
	pis->size += font_zoom_factor;

	anjuta_print_job_info_style_clear_attributes	(pis);
	anjuta_print_job_info_style_load_font (pis);
	
	if (!pis->font) {
		g_warning ("Cannot load document font: %s. Trying Default font: %s.",
			pis->font_name, AN_PRINT_FONT_BODY_DEFAULT);
		if (pis->font_name)
			g_free (pis->font_name);
		pis->font_name = g_strdup (AN_PRINT_FONT_BODY_DEFAULT);
		anjuta_print_job_info_style_load_font (pis);
	}
	if (!pis->font) {
		pis->bold = FALSE;
		anjuta_print_job_info_style_load_font (pis);
	}
	if (!pis->font) {
		pis->italics = FALSE;
		anjuta_print_job_info_style_load_font (pis);
	}
	if (!pis->font) {
		pis->size = AN_PRINT_FONT_SIZE_BODY_DEFAULT;
		anjuta_print_job_info_style_load_font (pis);
	}
	if (!pis->font) {
		anjuta_print_job_info_style_destroy (pis);
		return NULL;
	}

	anjuta_print_job_info_style_set_attributes (pis);
	
	if (color)
	{
		anjuta_print_job_info_style_set_color_attributes (pis);
	}
	
	return pis;
}

static void 
anjuta_print_job_info_destroy(PrintJobInfo *pji)
{
	int i;
	
	g_return_if_fail (pji);

	if (pji->pages != NULL)
		g_array_free (pji->pages, TRUE);

	if (pji->layout != NULL)
		g_object_unref (pji->layout);	

	if (pji->line_numbers_layout != NULL)
		g_object_unref (pji->line_numbers_layout);	
	
	if (pji->header_layout != NULL)
		g_object_unref (pji->header_layout);	

	if (pji->buffer != NULL)
		g_free(pji->buffer);
	
	for (i = 0; i < AN_PRINT_MAX_STYLES; i++)
	{
		if (pji->styles_pool[i])
			anjuta_print_job_info_style_destroy (pji->styles_pool[i]);
	}
	g_free(pji);
}

static PrintJobInfo*
anjuta_print_job_info_new (TextEditor *te)
{
	PrintJobInfo *pji;

	pji = g_new0(PrintJobInfo, 1);

	pji->te = te;
	pji->pages = g_array_new (FALSE, FALSE, sizeof (PrintPageInfo));
	
	return pji;
}

static PrintJobInfoStyle*
anjuta_print_get_style (PrintJobInfo *pji, gint style)
{
	PrintJobInfoStyle* pis;
	
	pis = pji->styles_pool[style];
	
	if (!pis) {
		gchar* language;
		language = (gchar*) aneditor_command(pji->te->editor_id, ANE_GETLANGUAGE,0, 0);
		pis = anjuta_print_job_info_style_new (pji->te->props_base,
						language, style, pji->zoom_factor, pji->print_color);
		pji->styles_pool[style] = pis;
	}
	
	if (!pis && style != AN_PRINT_DEFAULT_TEXT_STYLE) {
		pis = anjuta_print_get_style (pji, AN_PRINT_DEFAULT_TEXT_STYLE);
	}
	
	return pis;
}

/* Change style of main layout */
static void
anjuta_print_apply_style (PrintJobInfo *pji, gint style, guint start, guint end)
{
	PrintJobInfoStyle* pis;
	
	pis = anjuta_print_get_style (pji, style);
	g_return_if_fail (pis != NULL);
	
	pango_layout_set_font_description (pji->layout, pis->font);

	if (pis->attrs)
	{
		PangoAttrList *attr_list;
		PangoAttrList *new_list = NULL;
		GList *node;

		attr_list = pango_layout_get_attributes (pji->layout);
		if ((attr_list == NULL) || (start == 0))
		{
			attr_list = pango_attr_list_new ();
			new_list = attr_list;
		}
		
		for (node = g_list_first (pis->attrs); node != NULL; node = g_list_next (node))
		{
			PangoAttribute *a = pango_attribute_copy ((PangoAttribute *)node->data);
			
			a->start_index = start;
			a->end_index = end;
			
			pango_attr_list_insert (attr_list, a);
		}
		pango_layout_set_attributes (pji->layout, attr_list);
		if (new_list) pango_attr_list_unref (new_list);
	}
}

static void
anjuta_setup_layout (PrintJobInfo *pji, GtkPrintContext *context)
{
	g_return_if_fail (pji->layout == NULL);

	/* Layout for header */
	if (pji->print_header)
	{
		pji->layout = gtk_print_context_create_pango_layout (context);
		anjuta_print_apply_style (pji, AN_PRINT_DEFAULT_TEXT_STYLE, 0, G_MAXUINT);

		g_return_if_fail (pji->header_layout == NULL);						
		pji->header_layout = pji->layout;
	}
	
	/* Layout for line numbers */
	if (pji->print_line_numbers)
	{
		pji->layout = gtk_print_context_create_pango_layout (context);
		anjuta_print_apply_style (pji, AN_PRINT_LINENUMBER_STYLE, 0, G_MAXUINT);
		
		g_return_if_fail (pji->line_numbers_layout == NULL);						
		pji->line_numbers_layout = pji->layout;
								
		pango_layout_set_alignment (pji->line_numbers_layout, PANGO_ALIGN_RIGHT);
	}
	
	/* Layout for the text */
	pji->layout = gtk_print_context_create_pango_layout (context);
	anjuta_print_apply_style (pji, AN_PRINT_DEFAULT_TEXT_STYLE, 0, G_MAXUINT);
	
	if (pji->wrapping)
	{
		pango_layout_set_wrap (pji->layout, PANGO_WRAP_WORD_CHAR);
	}
	set_layout_tab_width (pji->layout, pji->tab_width);
}

static void
anjuta_print_update_page_size_and_margins (PrintJobInfo *pji, GtkPrintContext *context)
{
	GtkPageSetup *page_setup;

	page_setup = gtk_print_context_get_page_setup (context);
	
	pji->margin_left = gtk_page_setup_get_left_margin (page_setup, GTK_UNIT_POINTS);
	pji->margin_right = gtk_page_setup_get_right_margin (page_setup, GTK_UNIT_POINTS);
	pji->margin_top = gtk_page_setup_get_top_margin (page_setup, GTK_UNIT_POINTS);
	pji->margin_bottom = gtk_page_setup_get_bottom_margin (page_setup, GTK_UNIT_POINTS);

	pji->page_width = gtk_page_setup_get_paper_width (page_setup, GTK_UNIT_POINTS);
	pji->page_height = gtk_page_setup_get_paper_height (page_setup, GTK_UNIT_POINTS);

	if (pji->print_line_numbers)
	{
		gint line_count;
		gchar *str;
		gint padding;
		PangoRectangle rect;		
		
		line_count = text_editor_get_total_lines (pji->te) + 1;
		for (padding = 1; line_count >= 10; padding++) line_count /= 10;
		str = g_strnfill (padding, '9'); 
		pango_layout_set_text (pji->line_numbers_layout, str, -1);
		g_free (str);
		
		pango_layout_get_extents (pji->line_numbers_layout, NULL, &rect);

		pji->numbers_width = ((gdouble) rect.width + (gdouble)AN_PRINT_LINE_NUMBER_SEPARATION) / (gdouble) PANGO_SCALE;
		pji->numbers_height = (gdouble) rect.height / (gdouble) PANGO_SCALE;
	}
	else
	{
		pji->numbers_width = 0.0;
		pji->numbers_height = 0.0;
	}
	
	if (pji->print_header)
	{
		PangoContext *pango_context;
		PangoFontMetrics* font_metrics;
		const PangoFontDescription *font;
		gdouble ascent;
		gdouble descent;

		pango_context = gtk_print_context_create_pango_context (context);
		font = pango_layout_get_font_description (pji->header_layout);
		pango_context_set_font_description (pango_context, font);

		font_metrics = pango_context_get_metrics (pango_context, font, gtk_get_default_language());

		ascent = (gdouble) pango_font_metrics_get_ascent (font_metrics);
		descent = (gdouble) pango_font_metrics_get_descent (font_metrics);
		pango_font_metrics_unref (font_metrics);
		g_object_unref (pango_context);

		pji->header_height = (gdouble)(ascent + descent) / PANGO_SCALE;
		pji->header_height *= AN_PRINT_HEADER_SIZE_FACTOR;
	}
	else
	{
		pji->header_height = 0.0;
	}
	
	pango_layout_set_width (pji->layout, (pji->page_width - pji->margin_left - pji->numbers_width - pji->margin_right) * PANGO_SCALE);
}

static void
anjuta_print_layout_line (PrintJobInfo *pji)
{
	gchar utf8_char[4];
	gchar style;
	const gchar *text;
	GString *line_buffer;
	guint last_change;
	gint current_style;
		
	/* Read a complete line */
	line_buffer = g_string_new (NULL);
	last_change = 0;
	current_style = pji->buffer[pji->current_pos *2 + 1];
	
	for (text = &pji->buffer[pji->current_pos * 2]; (*text != '\n') && (pji->current_pos < pji->buffer_size); text = &pji->buffer[pji->current_pos * 2])
	{
		gint len;
		
		/* Buffer contains data bytes merged with style bytes */
		utf8_char[0] = text[0];
		utf8_char[1] = text[2];
		utf8_char[2] = text[4];
		utf8_char[3] = text[8];
		style = text[1];

		/* Change style if necessary */
		if (style != current_style)
		{
			anjuta_print_apply_style (pji, current_style, last_change, line_buffer->len);
			last_change = line_buffer->len;
			current_style= style;
		}
		
		/* Append character */
		len = g_utf8_next_char (utf8_char) - utf8_char;
		g_string_append_len (line_buffer, utf8_char, len);
		
		pji->current_pos += len;
	}
	pji->current_pos++;
	
	anjuta_print_apply_style (pji, current_style, last_change, G_MAXUINT);
	if (line_buffer->len == 0)
	{
		/* Empty line, display just one space */
		pango_layout_set_text (pji->layout," ", 1);
	}
	else
	{
		pango_layout_set_text (pji->layout,line_buffer->str, line_buffer->len);
	}
	
	g_string_free (line_buffer, TRUE);
}			

static void
anjuta_draw_header (PrintJobInfo * pji, cairo_t *cr)
{
	gchar *text1 = g_strdup_printf (_("File: %s"), pji->te->filename);
	gchar *text2 = g_strdup_printf ("%d", pji->current_page + 1);
	gdouble baseline;
	gdouble header_width;
	gdouble layout_width;
	PangoLayoutIter *iter;
	PangoRectangle rect;
	PangoLayoutLine* line;
	gdouble x;

	pango_cairo_update_layout (cr, pji->header_layout);
	
	header_width = pji->page_width - pji->margin_left - pji->margin_right;

	/* Print filename on left */	
	pango_layout_set_text (pji->header_layout, text1, -1);
	iter = pango_layout_get_iter (pji->header_layout);
	baseline = (gdouble) pango_layout_iter_get_baseline (iter) / (gdouble) PANGO_SCALE;	
	
	x =pji->margin_left;
	
	line = pango_layout_iter_get_line_readonly (iter);
	pango_layout_iter_free (iter);
 	cairo_move_to (cr, x, pji->margin_top + baseline);
	pango_cairo_show_layout_line (cr, line);
	
	/* Print page number on right */
	pango_layout_set_text (pji->header_layout, text2, -1);
	iter = pango_layout_get_iter (pji->header_layout);
	baseline = (gdouble) pango_layout_iter_get_baseline (iter) / (gdouble) PANGO_SCALE;	
	
	pango_layout_get_extents (pji->header_layout, NULL, &rect);
	layout_width = (double) rect.width / (double) PANGO_SCALE;
	x = pji->margin_left + header_width - layout_width;

	line = pango_layout_iter_get_line_readonly (iter);
	pango_layout_iter_free (iter);
 	cairo_move_to (cr, x, pji->margin_top + baseline);
	pango_cairo_show_layout_line (cr, line);

	g_free (text1);
	g_free (text2);
}

static void
anjuta_draw_linenum (PrintJobInfo * pji, cairo_t *cr)
{
	gchar *text = g_strdup_printf ("%d", pji->current_line);
	gdouble baseline;
	gdouble layout_width;
	PangoLayoutIter *iter;
	PangoRectangle rect;
	gdouble x;
	
	pango_cairo_update_layout (cr, pji->line_numbers_layout);

	/* Print line number on right */
	pango_layout_set_text (pji->line_numbers_layout, text, -1);
	iter = pango_layout_get_iter (pji->line_numbers_layout);
	baseline = (gdouble) pango_layout_iter_get_baseline (iter) / (gdouble) PANGO_SCALE;	
	pango_layout_iter_free (iter);
	
	pango_layout_get_extents (pji->line_numbers_layout, NULL, &rect);
	layout_width = (double) rect.width / (double) PANGO_SCALE;
	x = pji->margin_left + pji->numbers_width - layout_width - AN_PRINT_LINE_NUMBER_SEPARATION;
		
 	cairo_move_to (cr, x, pji->current_height);
	pango_cairo_show_layout (cr, pji->line_numbers_layout);		

	g_free (text);
}

static void
anjuta_end_print (GtkPrintOperation        *operation, 
		   GtkPrintContext          *context,
		   PrintJobInfo *pji)
{
	anjuta_print_job_info_destroy (pji);
}

static void
anjuta_draw_page (GtkPrintOperation        *operation,
		   GtkPrintContext          *context,
		   gint                      page_nr,
		   PrintJobInfo *pji)
{
	cairo_t *cr;
	gdouble x;
	gboolean done;
	guint page_end;

	g_return_if_fail (GTK_IS_PRINT_CONTEXT (context));

	pji->current_page = page_nr;

	cr = gtk_print_context_get_cairo_context (context);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_translate (cr,
					 -1 * pji->margin_left,
					 -1 * pji->margin_top);	
	
	if (pji->print_header)
	{
		anjuta_draw_header (pji, cr);
	}

	pji->current_pos = g_array_index (pji->pages, PrintPageInfo, page_nr).pos;
	pji->current_line = g_array_index (pji->pages, PrintPageInfo, page_nr).line;
	if (pji->pages->len <= (page_nr + 1))
	{
		page_end = pji->buffer_size;
	}
	else
	{
		page_end = g_array_index (pji->pages, PrintPageInfo, page_nr + 1).pos;
	}

	pango_cairo_update_layout (cr, pji->layout);
	
	x = pji->margin_left + pji->numbers_width;
	pji->current_height = pji->margin_top + pji->header_height;
	
	done = pji->current_pos >= page_end;
	while (!done)
	{
		PangoRectangle rect;
		gdouble height;
		guint start;

		/* Draw line number */
		if (pji->print_line_numbers)
		{
			anjuta_draw_linenum (pji, cr);
		}
		
		/* Layout one line */
		start = pji->current_pos;
		anjuta_print_layout_line (pji);
		
		/* Draw line */
		cairo_move_to (cr, x, pji->current_height);
		pango_cairo_show_layout (cr, pji->layout);		

		/* Print next line */
		pango_layout_get_extents (pji->layout, NULL, &rect);
		height = rect.height / PANGO_SCALE;
		if (height < pji->numbers_height)
		{
			height = pji->numbers_height;
		}
		pji->current_height += height;
		pji->current_line ++;
		
		done = pji->current_pos >= page_end;
	}
}

static gboolean
anjuta_paginate (GtkPrintOperation        *operation, 
		  GtkPrintContext          *context,
		  PrintJobInfo *pji)
{
	gdouble text_height;
	guint page_count;
	gboolean done;
	
	text_height = pji->page_height - pji->margin_top - pji->header_height - pji->header_height;
	page_count = 0;
	done = pji->current_pos >= pji->buffer_size;
	
	/* Mark beginning of a page */
	if (pji->pages->len == pji->current_page)
	{
		PrintPageInfo info = {pji->current_pos, pji->current_line};
		g_array_append_val (pji->pages, info);
	}
	
	while (!done && (page_count < AN_PRINT_PAGINATION_CHUNK_SIZE))
	{
		PangoRectangle rect;
		gdouble height;
		guint start;

		/* Layout one line */
		start = pji->current_pos;
		anjuta_print_layout_line (pji);
		pango_layout_get_extents (pji->layout, NULL, &rect);
		height = rect.height / PANGO_SCALE;
		
		if (height < pji->numbers_height)
		{
			height = pji->numbers_height;
		}
		
		pji->current_height += height;
		if (pji->current_height > text_height)
		{
			PrintPageInfo info = {start, pji->current_line};
			
			/* New page */
			pji->current_page++;
			pji->current_height = height; 
			g_array_append_val (pji->pages, info);
			page_count++;
		}
		pji->current_line++;
		
		done = pji->current_pos >= pji->buffer_size;
	}
	
	gtk_print_operation_set_n_pages (operation, pji->pages->len);
	
	return done;
}

/* Second function called after displaying print dialog but before start
 * printing */
static void
anjuta_print_begin (GtkPrintOperation        *operation, 
					GtkPrintContext          *context,
					PrintJobInfo *pji)
{
	gint i;
	
	/* Load Buffer to be printed. The buffer loaded is the text/style combination.*/
	pji->buffer_size = scintilla_send_message(SCINTILLA(pji->te->scintilla), SCI_GETLENGTH, 0, 0);
	pji->buffer = (gchar *) aneditor_command(pji->te->editor_id, ANE_GETSTYLEDTEXT, 0, pji->buffer_size);
	if (pji->buffer == NULL)
	{
		anjuta_util_dialog_error(NULL, _("Unable to get text buffer for printing"));
		gtk_print_operation_cancel (operation);
		anjuta_print_job_info_destroy(pji);
	}
	
	/* State variables initializations */
	g_array_set_size (pji->pages, 0);
	for (i = 0; i < AN_PRINT_MAX_STYLES; i++) pji->styles_pool[i] = NULL;
	pji->current_style = 0;
	pji->current_page = 0;
	pji->current_pos = 0;
	pji->current_height = 0.0;
	pji->current_line = 1;

	/* setup layout */
	anjuta_setup_layout (pji, context);
	
	/* Margin settings */
	anjuta_print_update_page_size_and_margins (pji, context);
}

/* First print function called before displayed print dialog */
static GtkPrintOperation*
anjuta_print_setup (AnjutaPreferences *p, TextEditor *te)
{
	PrintJobInfo *pji;
	GtkPrintOperation* operation;

	scintilla_send_message (SCINTILLA (te->scintilla), SCI_COLOURISE, 0, -1);
	
	/* Anjuta print layout object */
	pji = anjuta_print_job_info_new(te);
	
	/* Set preferences */
	pji->print_line_numbers =
		anjuta_preferences_get_bool_with_default (p, PRINT_LINENUM_COUNT, 1);
	pji->print_header =
		anjuta_preferences_get_bool_with_default (p, PRINT_HEADER, 1);
	pji->print_color =
		anjuta_preferences_get_bool_with_default (p, PRINT_COLOR, 1);
	pji->wrapping =
		anjuta_preferences_get_bool_with_default (p, PRINT_WRAP, 1);
	pji->tab_width =
		anjuta_preferences_get_int_with_default (p, TAB_SIZE, 8);
	pji->zoom_factor = anjuta_preferences_get_int (te->preferences,
						       TEXT_ZOOM_FACTOR);
	
	
	/* Set progress bar */
	pji->status = anjuta_shell_get_status (te->shell, NULL);
	anjuta_status_progress_reset (pji->status);
	anjuta_status_progress_add_ticks (pji->status, 100);

	/* Gtk print operation object */
	operation = gtk_print_operation_new ();

	gtk_print_operation_set_job_name (operation, te->filename);
	gtk_print_operation_set_show_progress (operation, TRUE);
	
	g_signal_connect (G_OBJECT (operation), "begin-print", 
					  G_CALLBACK (anjuta_print_begin), pji);
	g_signal_connect (G_OBJECT (operation), "paginate", 
					  G_CALLBACK (anjuta_paginate), pji);
	g_signal_connect (G_OBJECT (operation), "draw-page", 
					  G_CALLBACK (anjuta_draw_page), pji);
	g_signal_connect (G_OBJECT (operation), "end-print", 
					  G_CALLBACK (anjuta_end_print), pji);
	
	return operation;
}

void
anjuta_print (gboolean preview, AnjutaPreferences *p, TextEditor *te)
{
	GtkPrintOperation* operation;

	if (te == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET(te))),
								  _("No file to print!"));
		return;
	}
	
	operation = anjuta_print_setup (p, te);
	gtk_print_operation_run (operation, 
							 preview ? GTK_PRINT_OPERATION_ACTION_PREVIEW :
								 GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
								 NULL, NULL);
	g_object_unref (operation);	
}
