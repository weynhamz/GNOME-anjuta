/*
 * print.c
 * Copyright (C) 2002
 *     Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 *     Naba Kumar <kh_naba@users.sourceforge.net>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <gnome.h>
#include <libgnomevfs/gnome-vfs.h>

#include "anjuta.h"
#include "preferences.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1

#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "text_editor.h"
#include "print.h"
#include "print-doc.h"
#include "print-util.h"

/*
static void anjuta_print_calculate_pages (PrintJobInfo *pji)
{
	pji->total_lines = anjuta_print_document_determine_lines(pji, FALSE);
	pji->total_lines_real = anjuta_print_document_determine_lines(pji, TRUE);
	pji->pages = ((int) (pji->total_lines_real-1)/pji->lines_per_page)+1;
	pji->page_first = 1;
	pji->page_last = pji->pages;
}
*/
static void
anjuta_print_job_info_style_destroy (PrintJobInfoStyle *pis) {
	if (pis) {
		if (pis->font) gnome_font_unref(pis->font);
		if (pis->font_name) g_free(pis->font_name);
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
string_to_color (const char *val, GdkColor* color) {
	/* g_print ("Color = %s\n", val); */
	color->red   = IntFromHexDigit(val[1]) * 16 + IntFromHexDigit(val[2]);
	color->green = IntFromHexDigit(val[3]) * 16 + IntFromHexDigit(val[4]);
	color->blue  = IntFromHexDigit(val[5]) * 16 + IntFromHexDigit(val[6]);
}

static void
anjuta_print_job_info_style_init (PrintJobInfoStyle *pis, PropsID prop,
						gchar* lang, guint style)
{
	gchar *style_key, *style_string, *val, *opt;
	
	style_key = g_strdup_printf ("style.%s.%0d", lang, style);
	style_string = prop_get_expanded (prop, style_key);
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
			pis->weight = GNOME_FONT_BOLD;
		if (0 == strcmp(opt, "notbold"))
			pis->weight = GNOME_FONT_MEDIUM;
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
	if (val)
		g_free(val);
}

static PrintJobInfoStyle*
anjuta_print_job_info_style_new (PropsID prop, gchar* lang, guint style)
{
	PrintJobInfoStyle* pis;
	
	g_return_val_if_fail (prop > 0, NULL);
	g_return_val_if_fail (style < 256, NULL);
	
	pis = g_new (PrintJobInfoStyle, 1);
	
	pis->font_name = g_strdup(AN_PRINT_FONT_BODY_DEFAULT);
	pis->weight = GNOME_FONT_MEDIUM;
	
	/* Black */
	pis->fore_color.red = 0;
	pis->fore_color.green = 0;
	pis->fore_color.blue = 0;
	
	/* White */
	pis->back_color.red = (gushort)(-1);
	pis->back_color.green = (gushort)(-1);
	pis->back_color.blue = (gushort)(-1);
	
	pis->italics = FALSE;
	pis->size = AN_PRINT_FONT_SIZE_BODY_DEFAULT;
	
	pis->font = gnome_font_new_closest (pis->font_name, pis->weight,
					pis->italics, pis->size);

	anjuta_print_job_info_style_init (pis, prop, "*", style);
	if (lang && strlen(lang) > 0) {
		anjuta_print_job_info_style_init (pis, prop, lang, style);
	}
	
	if (!pis->font) {
		g_warning ("Cannot load document font: %s. Trying Default font: %s.",
			pis->font_name, AN_PRINT_FONT_BODY_DEFAULT);
		pis->font = gnome_font_new_closest (AN_PRINT_FONT_BODY_DEFAULT,
			pis->weight, pis->italics, pis->size);
	}
	if (!pis->font) {
		pis->font = gnome_font_new_closest (AN_PRINT_FONT_BODY_DEFAULT,
			GNOME_FONT_MEDIUM, pis->italics, pis->size);
	}
	if (!pis->font) {
		pis->font = gnome_font_new_closest (AN_PRINT_FONT_BODY_DEFAULT,
			GNOME_FONT_MEDIUM, FALSE, pis->size);
	}
	if (!pis->font) {
		pis->font = gnome_font_new_closest (AN_PRINT_FONT_BODY_DEFAULT,
			GNOME_FONT_MEDIUM, FALSE, AN_PRINT_FONT_SIZE_BODY_DEFAULT);
	}
	if (!pis->font) {
		anjuta_print_job_info_style_destroy (pis);
		return NULL;
	}
	return pis;
}

void 
anjuta_print_job_info_destroy(PrintJobInfo *pji)
{
	int i;
	if (pji)
	{
		if (pji->master) gnome_print_master_close(pji->master);
		if (pji->buffer) g_free(pji->buffer);
		for (i = 0; i < AN_PRINT_MAX_STYLES; i++)
		{
			if (pji->styles_pool[i])
				anjuta_print_job_info_style_destroy (pji->styles_pool[i]);
		}
		g_free(pji);
	}
}

PrintJobInfo*
anjuta_print_job_info_new (void)
{
	PrintJobInfo *pji;
	Preferences *p = app->preferences;
	char *paper_name;
	gint i;

	pji = g_new0(PrintJobInfo, 1);
	if (NULL == (pji->te = anjuta_get_current_text_editor()))
	{
		anjuta_error(_("No file to print!"));
		g_free(pji);
		return NULL;
	}
	pji->preview = FALSE;
	pji->master = gnome_print_master_new();
	pji->pc = gnome_print_master_get_context(pji->master);
	pji->printer = NULL;

	if (!GNOME_IS_PRINT_MASTER(pji->master))
	{
		anjuta_error(_("Unable to get GnomePrintMaster"));
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	if (!GNOME_IS_PRINT_CONTEXT(pji->pc))
	{
		anjuta_error(_("Unable to get GnomePrintContext"));
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	
	/* Load Buffer to be printed. The buffer loaded is the text/style combination.*/
	pji->buffer_size = scintilla_send_message(SCINTILLA(pji->te->widgets.editor), SCI_GETLENGTH, 0, 0);
	pji->buffer = (gchar *) aneditor_command(pji->te->editor_id, ANE_GETSTYLEDTEXT, 0, pji->buffer_size);
	if (NULL == pji->buffer)
	{
		anjuta_error(_("Unable to get text buffer for printing"));
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	
	/* Load paper specifications */
	if (NULL == (paper_name = preferences_get(p, PRINT_PAPER_SIZE)))
		paper_name = g_strdup("A4");
	if (NULL == (pji->paper = gnome_paper_with_name(paper_name)))
	{
		anjuta_error(_("Unable to set paper %s"), paper_name);
		g_free(paper_name);
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	g_free(paper_name);
	gnome_print_master_set_paper(pji->master, pji->paper);
	
	/* Orientations */
	if (preferences_get_int_with_default(p, PRINT_LANDSCAPE, 0))
	{
		pji->orientation = PRINT_ORIENT_LANDSCAPE;
		pji->page_width  = gnome_paper_psheight(pji->paper);
		pji->page_height = gnome_paper_pswidth(pji->paper);
	}
	else
	{
		pji->orientation = PRINT_ORIENT_PORTRAIT;
		pji->page_width  = gnome_paper_pswidth(pji->paper);
		pji->page_height = gnome_paper_psheight(pji->paper);
	}
	
	/* Margin settings */
	pji->margin_left    = preferences_get_int_with_default(p, PRINT_MARGIN_LEFT, 54);
	pji->margin_right   = preferences_get_int_with_default(p, PRINT_MARGIN_RIGHT, 54);
	pji->margin_top     = preferences_get_int_with_default(p, PRINT_MARGIN_TOP, 54);
	pji->margin_bottom  = preferences_get_int_with_default(p, PRINT_MARGIN_BOTTOM, 54);
	pji->margin_header  = preferences_get_int_with_default(p, PRINT_MARGIN_HEADER, 18);
	pji->margin_numbers = preferences_get_int_with_default(p, PRINT_MARGIN_NUMBERS, 36);
	
	/* Line number printing details */
	pji->print_line_numbers = preferences_get_int_with_default (p, PRINT_LINENUM_COUNT, 1);
	pji->print_line_numbers = pji->print_line_numbers >= 0? pji->print_line_numbers: 0;
	if (pji->print_line_numbers == 0) {
		pji->margin_numbers = 0.0;
	}
	/* Other preferences. */
	pji->print_header = preferences_get_int_with_default (p, PRINT_HEADER, 1);
	pji->print_color = preferences_get_int_with_default (p, PRINT_COLOR, 1);
	pji->wrapping = preferences_get_int_with_default (p, PRINT_WRAP, 1);
	pji->tab_size = preferences_get_int_with_default (p, TAB_SIZE, 8);
	pji->range_type = GNOME_PRINT_RANGE_ALL;
	
	/* State variables initializations */
	pji->canceled = FALSE;
	pji->current_style_num = 0;
	pji->current_style = NULL;
	pji->current_page = 0;
	pji->cursor_x = pji->margin_left + pji->margin_numbers;
	pji->cursor_y = pji->page_height - pji->margin_top - pji->margin_header;
	
	for (i = 0; i < AN_PRINT_MAX_STYLES; i++) pji->styles_pool[i] = NULL;
	return pji;
}

gfloat anjuta_print_get_width (PrintJobInfo *pji, gint style, const char *str, gint len)
{
	char buff[64];
	strncpy(buff, str, len);
	buff[len] = '\0';
	return aneditor_command(pji->te->editor_id, ANE_TEXTWIDTH, style, (long)buff);
}

void anjuta_print_set_orientation (PrintJobInfo * pji)
{
	double affine[6];

	if (pji->orientation == PRINT_ORIENT_PORTRAIT)
		return;
	art_affine_rotate(affine, 90.0);
	gnome_print_concat(pji->pc, affine);
	art_affine_translate(affine, 0, -pji->page_height);
	gnome_print_concat(pji->pc, affine);
}

static void
anjuta_print_show_header (PrintJobInfo * pji)
{
	guchar *text1 = g_strdup_printf (_("File: %s"), pji->te->filename);
	guchar *text2 = g_strdup_printf (_("%d"), pji->current_page);
	gchar  *ch;
	gfloat len;
	gboolean save_wrapping;

	save_wrapping = pji->wrapping;
	pji->wrapping = FALSE;
	/* Print filename on left */
	pji->cursor_x = pji->margin_left;
	pji->cursor_y = pji->page_height - pji->margin_top;
	gnome_print_moveto (pji->pc, pji->cursor_x, pji->cursor_y);
	ch = text1;
	while (*ch != '\0') {
		if (anjuta_print_show_char_styled(pji, *ch, 32)) break;
		ch++;
	}

	/* Print page number on right */
	len = aneditor_command(pji->te->editor_id, ANE_TEXTWIDTH, 32, (long)text2);
	pji->cursor_x = pji->page_width - pji->margin_right - len - 2;
	pji->cursor_y = pji->page_height - pji->margin_top - pji->margin_header / 2;
	gnome_print_moveto (pji->pc, pji->cursor_x, pji->cursor_y);
	ch = text2;
	while (*ch != '\0') {
		if (anjuta_print_show_char_styled(pji, *ch, 32)) break;
		ch++;
	}
	pji->wrapping = save_wrapping;
	g_free (text1);
	g_free (text2);
}

void
anjuta_print_show_linenum (PrintJobInfo * pji, guint line)
{
	guchar *text = g_strdup_printf (_("%04u"), line);
	gchar  *ch;
	gboolean save_wrapping;
	gfloat save_x, save_y;

	save_wrapping = pji->wrapping;
	save_x = pji->cursor_x;
	save_y = pji->cursor_y;
	
	pji->wrapping = FALSE;
	pji->cursor_x = pji->margin_left;
	gnome_print_moveto (pji->pc, pji->cursor_x, pji->cursor_y);
	ch = text;
	while (*ch != '\0') {
		if (anjuta_print_show_char_styled(pji, *ch, 33)) break;
		ch++;
	}

	pji->wrapping = save_wrapping;
	pji->cursor_x = save_x;
	pji->cursor_y = save_y;
	g_free (text);
}

void anjuta_print_begin (PrintJobInfo * pji)
{
	gchar page[256];
	
	pji->current_page = 1;
	
	sprintf(page, "%d", pji->current_page);
	gnome_print_beginpage (pji->pc, page);
	anjuta_print_set_orientation(pji);
	if (pji->print_header)
		anjuta_print_show_header(pji);
	pji->cursor_y = pji->page_height - pji->margin_top - pji->margin_header;
	pji->cursor_x = pji->margin_left + pji->margin_numbers;
}

void anjuta_print_end(PrintJobInfo * pji)
{
	gnome_print_showpage(pji->pc);
	gnome_print_context_close (pji->pc);
}

void anjuta_print_new_page (PrintJobInfo *pji)
{
	gchar page[256];
	
	gnome_print_showpage (pji->pc);
	
	pji->current_page++;
	sprintf(page, "%d", pji->current_page);
	gnome_print_beginpage (pji->pc, page);
	anjuta_print_set_orientation(pji);
	if (pji->print_header)
		anjuta_print_show_header(pji);
	pji->cursor_y = pji->page_height - pji->margin_top - pji->margin_header;
	pji->cursor_x = pji->margin_left + pji->margin_numbers;
}

void anjuta_print_new_line (PrintJobInfo *pji)
{
	if ((pji->cursor_y - pji->current_font_height) < (pji->margin_top)) {
		anjuta_print_new_page (pji);
	} else {
		pji->cursor_x  = pji->margin_left + pji->margin_numbers;
		pji->cursor_y -=  pji->current_font_height;
	}
}

static PrintJobInfoStyle*
anjuta_print_get_style (PrintJobInfo *pji, gint style)
{
	PrintJobInfoStyle* pis;
	
	pis = pji->styles_pool[style];
	
	if (!pis) {
		gchar* language;
		language = (gchar*) aneditor_command(pji->te->editor_id, ANE_GETLANGUAGE, 0, 0);
		pis = anjuta_print_job_info_style_new (pji->te->props_base, language, style);
		pji->styles_pool[style] = pis;
	}
	if (!pis && style != 32) {
		pis = anjuta_print_get_style (pji, 32);
	}
	
	return pis;
}

void anjuta_print_set_style (PrintJobInfo *pji, gint style)
{
	PrintJobInfoStyle* pis;
	gint font_height;
	
	pis = anjuta_print_get_style (pji, style);
	g_return_if_fail (pis != NULL);
	
	gnome_print_setfont(pji->pc, pis->font);
	if (pji->print_color) {
		gnome_print_setrgbcolor(pji->pc,
				(gfloat)pis->fore_color.red/256.0 ,
				(gfloat)pis->fore_color.green/256.0,
				(gfloat)pis->fore_color.blue/256.0);
	}	
	pji->current_style_num = style;
	pji->current_style = pis;
	
	/* FIXME: How to find it? */
	/* font_height = gnome_display_font_height (gnome_font_get_display_font(pji->current_style->font)); */
	font_height = .14 * 72;
	pji->current_font_height = (pji->current_font_height > font_height)? pji->current_font_height : font_height;
}

gint
anjuta_print_show_char_styled (PrintJobInfo *pji, const char ch, const char style)
{
	wchar_t wcs[64];
	gchar utf8[64];
	gint  len;
	gfloat width;
	
	g_return_val_if_fail (pji != NULL, -1);
	
	if (ch == '\n') {
		anjuta_print_new_line (pji);
	} else {
		int conv_status;
		int i;
		char *p;
		
		/* Convert input char to wchar string */
		conv_status = mbstowcs (wcs, &ch, 1);
		if (conv_status == (size_t) (-1))
				return 0;
		
		/* Convert wchar string to utf8 */
		p = utf8;
		for (i = 0; i < conv_status; i++)
			p += anjuta_print_unichar_to_utf8 ((gint) wcs[i], p);
		
		/* Calculate final string length and width */
		len = p - utf8;
		width = anjuta_print_get_width (pji, style, &ch, 1);
		/* Determine wrapping */
		if ((pji->cursor_x + width) > (pji->page_width - pji->margin_right)) {
			anjuta_print_new_line (pji);
			if (!pji->wrapping) {
				return 1;/* Text wrapping, return true */
			}
		}
		
		/* Set style */
		anjuta_print_set_style(pji, style);
		
		/* Print it */
		gnome_print_moveto (pji->pc, pji->cursor_x, pji->cursor_y);
		gnome_print_show_sized (pji->pc, utf8, len);
		pji->cursor_x += width;
	}
	return 0; /* Return text wrap status */
}

static void
anjuta_print_set_buffer_as_selection (PrintJobInfo *pji)
{
	gint from, to;
	gint proper_from, proper_to;
	
	pji->range_start_line = 1;
	pji->range_end_line = text_editor_get_total_lines (pji->te);
	
	from = scintilla_send_message (SCINTILLA (pji->te->widgets.editor),
				SCI_GETSELECTIONSTART, 0, 0);
	to = scintilla_send_message (SCINTILLA (pji->te->widgets.editor),
				SCI_GETSELECTIONEND, 0, 0);
	if (from == to) return;
	proper_from = MIN(from, to);
	proper_to   = MAX(from, to);
	
	if (pji->buffer) g_free(pji->buffer);
	
	/* Print Buffers */
	pji->buffer = (gchar *) aneditor_command(pji->te->editor_id,
				ANE_GETSTYLEDTEXT, proper_from, proper_to);
	g_return_if_fail(pji->buffer!=NULL);
	pji->buffer_size = proper_to - proper_from;
	pji->range_start_line = scintilla_send_message (SCINTILLA (pji->te->widgets.editor),
				SCI_LINEFROMPOSITION, proper_from, 0);
	pji->range_end_line = scintilla_send_message (SCINTILLA (pji->te->widgets.editor),
				SCI_LINEFROMPOSITION, proper_to, 0);
	
	/* Same crapy misalignment */
	pji->range_start_line++;
}

static void
anjuta_print_set_buffer_as_range (PrintJobInfo *pji)
{
	gint from, to;
	gint proper_from, proper_to;
	
	from = scintilla_send_message (SCINTILLA (pji->te->widgets.editor),
				SCI_POSITIONFROMLINE, pji->range_start_line - 1, 0);
	to = scintilla_send_message (SCINTILLA (pji->te->widgets.editor),
				SCI_POSITIONFROMLINE, pji->range_end_line, 0);
	if (from == to) return;
	to--;
	proper_from = MIN(from, to);
	proper_to   = MAX(from, to);
	
	if (pji->buffer) g_free(pji->buffer);
	
	/* Print Buffers */
	pji->buffer = (gchar *) aneditor_command(pji->te->editor_id,
				ANE_GETSTYLEDTEXT, proper_from, proper_to);
	g_return_if_fail(pji->buffer!=NULL);
	pji->buffer_size = proper_to - proper_from;
}

void anjuta_print_document(PrintJobInfo * pji)
{
	guint i, ret, current_line;

	current_line = 1;
	
	anjuta_print_progress_start (pji);
	
	switch (pji->range_type) {
		case GNOME_PRINT_RANGE_SELECTION:
			anjuta_print_set_buffer_as_selection (pji);
			current_line = pji->range_start_line;
			break;
		
		case GNOME_PRINT_RANGE_RANGE:
			anjuta_print_set_buffer_as_range (pji);
			current_line = pji->range_start_line;
			break;
		default:
	}
	
	anjuta_print_begin (pji);
	
	if (pji->print_line_numbers > 0)
		anjuta_print_show_linenum (pji, current_line);
	
	for (i=0; i<pji->buffer_size; i++ ) {
		gchar text;
		gchar style;
		
		ret = 0;
		text = TEXT_AT(pji->buffer, i);
		style = STYLE_AT(pji->buffer, i);
		
		if (text == '\t') {
			int j;
			for (j = 0; j < pji->tab_size; j++ ) {
				ret = anjuta_print_show_char_styled(pji, ' ', 32);
				if (ret == 1) break;
			}
		} else {
			ret = anjuta_print_show_char_styled(pji, text, style);
		}
		if (ret == 1) {
			while (TEXT_AT(pji->buffer, i) != '\n') {
				i++;
				if (i >= pji->buffer_size)
					break;
			}
		}
		if (text == '\n' || ret == 1) {
			current_line++;
			if (pji->print_line_numbers > 0 &&
					((current_line+1) % pji->print_line_numbers == 0))
				anjuta_print_show_linenum (pji, current_line);
		}
		if (i % 50 ==  0) anjuta_print_progress_tick (pji, i);
		if (pji->canceled)
			break;
	}
	anjuta_print_end (pji);
	anjuta_print_progress_end (pji);
}
