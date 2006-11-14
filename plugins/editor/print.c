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
#include <string.h>

#include <gnome.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>
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
#define AN_PRINT_FONT_HEADER_DEFAULT "helvetica"
#define AN_PRINT_FONT_SIZE_BODY_DEFAULT   10
#define AN_PRINT_FONT_SIZE_HEADER_DEFALT 10
#define AN_PRINT_FONT_SIZE_NUMBERS_DEFAULT 6
#define AN_PRINT_MAX_STYLES 256
#define AN_PRINT_LINENUMBER_STYLE 33
#define AN_PRINT_DEFAULT_TEXT_STYLE 32
#define AN_PRINT_LINENUM_PADDING '0'

/* Boiler plate */
#define STYLE_AT(pji, index) (pji)->styles[(index)]

typedef struct _PrintJobInfoStyle
{
	GnomeFont      *font;

	gchar          *font_name;
	gboolean        italics;
	gboolean        bold;
	gint            size;
	
	GdkColor        fore_color;
	GdkColor        back_color;

} PrintJobInfoStyle;

typedef struct _PrintJobInfo
{
	GnomePrintJob     *print_job;
	GnomePrintConfig  *config;
	GnomePrintContext *pc;

	// const GnomePrintPaper *paper;
	
	PrintJobInfoStyle* styles_pool[AN_PRINT_MAX_STYLES];

	TextEditor *te;

	/* Print Buffer */
	gchar  *buffer;
	gchar  *styles;
	guint   buffer_size;
	
	/* Document zoom factor */
	gint zoom_factor;
	
	/* Preferences */
	gdouble page_width;
	gdouble page_height;
	gdouble margin_top;
	gdouble margin_bottom;
	gdouble margin_left;
	gdouble margin_right;
	gdouble margin_header;
	gdouble margin_numbers;
	
	gboolean print_header;
	gboolean print_color;
	gint     print_line_numbers;
	gboolean preview;
	gboolean wrapping;
	gint     tab_size;
	
	/* GC state */
	gfloat cursor_x;
	gfloat cursor_y;
	gfloat current_font_height;
	guint  current_style_num;
	PrintJobInfoStyle* current_style;
	guint  current_page;
	
	/* Printing range */
	gint range_type;
	gint range_start_line;
	gint range_end_line;
	
	/* Progress */
	GtkWidget *progress_bar;
	GtkWidget *progress_dialog;
	gboolean canceled;
	
} PrintJobInfo;

static GnomePrintConfig *print_config;

static PrintJobInfoStyle* anjuta_print_get_style (PrintJobInfo* pji, gint style);
static void anjuta_print_show_header (PrintJobInfo * pji);

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
string_to_color (const char *val, GdkColor* color)
{
	/* g_print ("Color = %s\n", val); */
	color->red   = IntFromHexDigit(val[1]) * 16 + IntFromHexDigit(val[2]);
	color->green = IntFromHexDigit(val[3]) * 16 + IntFromHexDigit(val[4]);
	color->blue  = IntFromHexDigit(val[5]) * 16 + IntFromHexDigit(val[6]);
}

static void
anjuta_print_job_info_style_load_font (PrintJobInfoStyle *pis)
{
	gchar *font_desc, *tmp;
	PangoFontDescription *desc;
	GnomeFontFace *font_face;
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
	if (pis->font)
		gnome_font_unref (pis->font);
	
	// pis->font = gnome_font_find_closest_from_full_name (font_desc);
	DEBUG_PRINT ("Print font loading: %s", font_desc);
	desc = pango_font_description_from_string (font_desc);
	font_face = gnome_font_face_find_closest_from_pango_description (desc);
	pis->font = gnome_font_face_get_font_default (font_face, size);
	g_assert (pis->font);
	DEBUG_PRINT ("Full font name: %s", gnome_font_get_full_name (pis->font));
	
	g_object_unref (font_face);
	pango_font_description_free (desc);
	g_free (font_desc);
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

static PrintJobInfoStyle*
anjuta_print_job_info_style_new (PropsID prop, gchar* lang,
			guint style, gint font_zoom_factor)
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
	return pis;
}

#define _PROPER_I18N

static gfloat
anjuta_print_get_font_height (PrintJobInfo *pji, gint style)
{
	PrintJobInfoStyle* pis = anjuta_print_get_style (pji, style);
	return gnome_font_get_size (pis->font);
	/*
	return (gnome_font_get_ascender (pis->font) + 
				gnome_font_get_descender (pis->font)); */
}

static gfloat
anjuta_print_get_text_width (PrintJobInfo *pji, gint style,
		gboolean utf8, const char *buff)
{
	PrintJobInfoStyle* style_info;
	gfloat width;
	
	style_info = anjuta_print_get_style (pji, style);
	if (style_info) {
		width = gnome_font_get_width_utf8 (style_info->font, buff);
	} else {
		width = aneditor_command(pji->te->editor_id,
				ANE_TEXTWIDTH, style, (long)buff);
	}
	return width;
}

static gfloat
anjuta_print_get_text_width_sized (PrintJobInfo *pji,
		gint style, gboolean utf8, const char *str, gint len)
{
	gfloat width;
	char *buff = g_new (gchar, len+1);
	
	strncpy(buff, str, len);
	buff[len] = '\0';
	width = anjuta_print_get_text_width (pji, style, utf8, buff);
	g_free (buff);
	return width;
}

static void 
anjuta_print_job_info_destroy(PrintJobInfo *pji)
{
	int i;
	
	g_return_if_fail (pji);
	
	if (pji->config != NULL)
		gnome_print_config_unref (pji->config);

	if (pji->pc != NULL)
		g_object_unref (pji->pc);

	if (pji->print_job != NULL)
		g_object_unref (pji->print_job);
	
	if (pji->buffer) g_free(pji->buffer);
	if (pji->styles) g_free(pji->styles);
	for (i = 0; i < AN_PRINT_MAX_STYLES; i++)
	{
		if (pji->styles_pool[i])
			anjuta_print_job_info_style_destroy (pji->styles_pool[i]);
	}
	g_free(pji);
}

static void
anjuta_print_update_page_size_and_margins (PrintJobInfo *pji)
{
	const GnomePrintUnit *unit;
	
	gnome_print_job_get_page_size_from_config (pji->config, 
											   &pji->page_width,
											   &pji->page_height);

	if (gnome_print_config_get_length (pji->config,
									   (const guchar *)GNOME_PRINT_KEY_PAGE_MARGIN_LEFT, 
									   &pji->margin_left, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_left, unit,
									  GNOME_PRINT_PS_UNIT);
	}
	
	if (gnome_print_config_get_length (pji->config,
									   (const guchar *)GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT, 
									   &pji->margin_right, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_right, unit,
									  GNOME_PRINT_PS_UNIT);
	}
	
	if (gnome_print_config_get_length (pji->config,
									   (const guchar *)GNOME_PRINT_KEY_PAGE_MARGIN_TOP, 
									   &pji->margin_top, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_top, unit,
									  GNOME_PRINT_PS_UNIT);
	}
	if (gnome_print_config_get_length (pji->config,
									   (const guchar *)GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM, 
									   &pji->margin_bottom, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_bottom, unit,
									  GNOME_PRINT_PS_UNIT);
	}
	if (pji->print_line_numbers <= 0)
	{
		pji->margin_numbers = 0.0;
	}
	if (pji->print_header)
	{
		pji->margin_header  =
				anjuta_print_get_font_height (pji, AN_PRINT_DEFAULT_TEXT_STYLE);
		pji->margin_header  *= 2.5;
	}
	else
	{
		pji->margin_header = 0.0;
	}
	if (pji->print_line_numbers > 0)
	{
		pji->margin_numbers =
				anjuta_print_get_text_width (pji, AN_PRINT_LINENUMBER_STYLE,
											 FALSE, "0");
		pji->margin_numbers *= 5; /* Digits in linenumbers */
		pji->margin_numbers += 5; /* Spacer */
	}
	else
	{
		pji->margin_numbers = 0.0;
	}
}

static PrintJobInfo*
anjuta_print_job_info_new (AnjutaPreferences *p, TextEditor *te)
{
	PrintJobInfo *pji;
	gint i;
	gchar *buffer;

	pji = g_new0(PrintJobInfo, 1);
	if (NULL == (pji->te = te))
	{
		anjuta_util_dialog_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET(te))),
								  _("No file to print!"));
		g_free(pji);
		return NULL;
	}
	pji->preview = FALSE;

	/* Set print config */
	if (!print_config)
	{
		print_config = gnome_print_config_default ();
		g_return_val_if_fail (print_config != NULL, NULL);

		gnome_print_config_set (print_config, (const guchar *)"Settings.Transport.Backend", (const guchar *)"lpr");
		gnome_print_config_set (print_config, (const guchar *)"Printer", (const guchar *)"GENERIC");
	}
	pji->config = print_config;
	gnome_print_config_ref (pji->config);
	
	/* Load Buffer to be printed. The buffer loaded is the text/style combination.*/
	pji->buffer_size = scintilla_send_message(SCINTILLA(pji->te->scintilla), SCI_GETLENGTH, 0, 0);
	buffer = (gchar *) aneditor_command(pji->te->editor_id, ANE_GETSTYLEDTEXT, 0, pji->buffer_size);
	if (NULL == buffer)
	{
		anjuta_util_dialog_error(NULL, _("Unable to get text buffer for printing"));
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	pji->buffer = g_new(char, pji->buffer_size + 1);
	pji->styles = g_new(char, pji->buffer_size + 1);
	pji->buffer[pji->buffer_size] = '\0';
	pji->styles[pji->buffer_size] = '\0';
	for (i = 0; i < pji->buffer_size; i++)
	{
		pji->buffer[i] = buffer[i << 1];
		pji->styles[i] = buffer[(i << 1) + 1];
	}
	g_free (buffer);
	
	/* Zoom factor */
	pji->zoom_factor = anjuta_preferences_get_int (te->preferences,
						       TEXT_ZOOM_FACTOR);
	
	/* Line number printing details */
	pji->print_line_numbers =
		anjuta_preferences_get_int_with_default (p, PRINT_LINENUM_COUNT, 1);
	pji->print_line_numbers = pji->print_line_numbers >= 0? pji->print_line_numbers: 0;
	
	/* Other preferences. */
	pji->print_header =
		anjuta_preferences_get_int_with_default (p, PRINT_HEADER, 1);
	pji->print_color =
		anjuta_preferences_get_int_with_default (p, PRINT_COLOR, 1);
	pji->wrapping =
		anjuta_preferences_get_int_with_default (p, PRINT_WRAP, 1);
	pji->tab_size =
		anjuta_preferences_get_int_with_default (p, TAB_SIZE, 8);
	pji->range_type = GNOME_PRINT_RANGE_ALL;
	
	/* State variables initializations */
	pji->canceled = FALSE;
	pji->current_style_num = 0;
	pji->current_style = NULL;
	pji->current_page = 0;
	pji->cursor_x = pji->margin_left + pji->margin_numbers;
	pji->cursor_y = pji->page_height - pji->margin_top - pji->margin_header;

	/* Init style pool */
	for (i = 0; i < AN_PRINT_MAX_STYLES; i++) pji->styles_pool[i] = NULL;
	
	/* Margin settings */
	// pji->margin_left    = preferences_get_int_with_default(p, PRINT_MARGIN_LEFT, 54);
	// pji->margin_right   = preferences_get_int_with_default(p, PRINT_MARGIN_RIGHT, 54);
	// pji->margin_top     = preferences_get_int_with_default(p, PRINT_MARGIN_TOP, 54);
	// pji->margin_bottom  = preferences_get_int_with_default(p, PRINT_MARGIN_BOTTOM, 54);
	
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
						language, style, pji->zoom_factor);
		pji->styles_pool[style] = pis;
	}
	if (!pis && style != AN_PRINT_DEFAULT_TEXT_STYLE) {
		pis = anjuta_print_get_style (pji, AN_PRINT_DEFAULT_TEXT_STYLE);
	}
	
	return pis;
}

static void
anjuta_print_set_style (PrintJobInfo *pji, gint style)
{
	PrintJobInfoStyle* pis;
	gint font_height;
	
	pis = anjuta_print_get_style (pji, style);
	g_return_if_fail (pis != NULL);
	g_assert (GNOME_IS_FONT(pis->font));
	gnome_print_setfont(pji->pc, pis->font);
	if (pji->print_color) {
		gnome_print_setrgbcolor(pji->pc,
				(gfloat)pis->fore_color.red/256.0 ,
				(gfloat)pis->fore_color.green/256.0,
				(gfloat)pis->fore_color.blue/256.0);
	}
	pji->current_style_num = style;
	pji->current_style = pis;
	
	font_height = anjuta_print_get_font_height (pji, style);

	pji->current_font_height = (pji->current_font_height > font_height)?
									pji->current_font_height : font_height;
}

static void
anjuta_print_new_page (PrintJobInfo *pji)
{
	gchar page[256];
	
	gnome_print_showpage (pji->pc);
	
	DEBUG_PRINT ("Printing new page...");
	
	pji->current_page++;
	sprintf(page, "%d", pji->current_page);
	gnome_print_beginpage (pji->pc, (const guchar *)page);
	if (pji->print_header)
		anjuta_print_show_header(pji);
	pji->cursor_y = pji->page_height - pji->margin_top - pji->margin_header;
	pji->cursor_x = pji->margin_left + pji->margin_numbers;
}

static void
anjuta_print_new_line (PrintJobInfo *pji)
{
	if ((pji->cursor_y - pji->current_font_height) < (pji->margin_top)) {
		anjuta_print_new_page (pji);
	} else {
		pji->cursor_x  = pji->margin_left + pji->margin_numbers;
		pji->cursor_y -=  pji->current_font_height;
	}
}


static gint
anjuta_print_show_chars_styled (PrintJobInfo *pji, const char *chars, gint size,
							   const char style)
{
	gfloat width;
	
	g_return_val_if_fail (pji != NULL, -1);
	g_return_val_if_fail (size > 0, -1);
	
	if (chars[0] == '\n') {
		anjuta_print_new_line (pji);
	} else {
		width = anjuta_print_get_text_width_sized (pji, style, TRUE, chars, size);
		
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
		gnome_print_show_sized (pji->pc, (const guchar *)chars, size);
		pji->cursor_x += width;
	}
	return 0; /* Return text wrap status */
}

static void
anjuta_print_set_buffer_as_selection (PrintJobInfo *pji)
{
	gint from, to;
	gint proper_from, proper_to;
	gchar *buffer;
	gint i;
	
	pji->range_start_line = 1;
	pji->range_end_line = text_editor_get_total_lines (pji->te);
	
	from = scintilla_send_message (SCINTILLA (pji->te->scintilla),
				SCI_GETSELECTIONSTART, 0, 0);
	to = scintilla_send_message (SCINTILLA (pji->te->scintilla),
				SCI_GETSELECTIONEND, 0, 0);
	if (from == to) return;
	proper_from = MIN(from, to);
	proper_to   = MAX(from, to);
	
	if (pji->buffer) g_free(pji->buffer);
	pji->buffer = NULL;
	if (pji->styles) g_free (pji->styles);
	pji->styles = NULL;
	
	/* Print Buffers */
	/* Load Buffer to be printed. The buffer loaded is the text/style combination.*/
	pji->buffer_size = proper_to - proper_from;
	buffer = (gchar *) aneditor_command(pji->te->editor_id, ANE_GETSTYLEDTEXT, proper_from, proper_to);
	g_return_if_fail(buffer!=NULL);
	pji->buffer = g_new(char, pji->buffer_size + 1);
	pji->styles = g_new(char, pji->buffer_size + 1);
	pji->buffer[pji->buffer_size] = '\0';
	pji->styles[pji->buffer_size] = '\0';
	for (i = 0; i < pji->buffer_size; i++)
	{
		pji->buffer[i] = buffer[i << 1];
		pji->styles[i] = buffer[(i << 1) + 1];
	}
	g_free (buffer);

	pji->range_start_line = scintilla_send_message (SCINTILLA (pji->te->scintilla),
				SCI_LINEFROMPOSITION, proper_from, 0);
	pji->range_end_line = scintilla_send_message (SCINTILLA (pji->te->scintilla),
				SCI_LINEFROMPOSITION, proper_to, 0);
	
	/* Same crapy misalignment */
	pji->range_start_line++;
}

static void
anjuta_print_set_buffer_as_range (PrintJobInfo *pji)
{
	gint from, to, i;
	gint proper_from, proper_to;
	gchar *buffer;
	
	from = scintilla_send_message (SCINTILLA (pji->te->scintilla),
				SCI_POSITIONFROMLINE, pji->range_start_line - 1, 0);
	to = scintilla_send_message (SCINTILLA (pji->te->scintilla),
				SCI_POSITIONFROMLINE, pji->range_end_line, 0);
	if (from == to) return;
	to--;
	proper_from = MIN(from, to);
	proper_to   = MAX(from, to);
	
	if (pji->buffer) g_free(pji->buffer);
	pji->buffer = NULL;
	if (pji->styles) g_free (pji->styles);
	pji->styles = NULL;
	
	/* Print Buffers */
	/* Load Buffer to be printed. The buffer loaded is the text/style combination.*/
	pji->buffer_size = proper_to - proper_from;
	buffer = (gchar *) aneditor_command(pji->te->editor_id, ANE_GETSTYLEDTEXT, proper_from, proper_to);
	g_return_if_fail(buffer!=NULL);
	pji->buffer = g_new(char, pji->buffer_size + 1);
	pji->styles = g_new(char, pji->buffer_size + 1);
	pji->buffer[pji->buffer_size] = '\0';
	pji->styles[pji->buffer_size] = '\0';
	for (i = 0; i < pji->buffer_size; i++)
	{
		pji->buffer[i] = buffer[i << 1];
		pji->styles[i] = buffer[(i << 1) + 1];
	}
	g_free (buffer);
}

static void
anjuta_print_show_header (PrintJobInfo * pji)
{
	guchar *text1 = (guchar *)g_strdup_printf (_("File: %s"), pji->te->filename);
	guchar *text2 = (guchar *)g_strdup_printf (_("%d"), pji->current_page);
	gfloat width;
	gboolean save_wrapping;

	save_wrapping = pji->wrapping;
	pji->wrapping = FALSE;
	
	/* Print filename on left */
	pji->cursor_x = pji->margin_left;
	pji->cursor_y = pji->page_height - pji->margin_top;
	gnome_print_moveto (pji->pc, pji->cursor_x, pji->cursor_y);
	anjuta_print_show_chars_styled(pji, (const char *)text1, strlen((const char *)text1),
								   AN_PRINT_DEFAULT_TEXT_STYLE);

	/* Print page number on right */
	width = anjuta_print_get_text_width (pji, AN_PRINT_DEFAULT_TEXT_STYLE, FALSE, (const char *)text2);
	pji->cursor_x = pji->page_width - pji->margin_right - width - 2;
	pji->cursor_y = pji->page_height - pji->margin_top;
	gnome_print_moveto (pji->pc, pji->cursor_x, pji->cursor_y);
	anjuta_print_show_chars_styled(pji, (const char *)text2, strlen((const char *)text2),
								   AN_PRINT_DEFAULT_TEXT_STYLE);
	
	pji->wrapping = save_wrapping;
	g_free (text1);
	g_free (text2);
}

static void
anjuta_print_show_linenum (PrintJobInfo * pji, guint line, guint padding)
{
	guchar *line_num = (guchar *)g_strdup_printf ("%u", line);
	guchar *pad_str, *text;
	gboolean save_wrapping;
	gfloat save_x, save_y;
		
	pad_str = (guchar *)g_strnfill(padding - strlen((const char *)line_num), AN_PRINT_LINENUM_PADDING);
	text = (guchar *)g_strconcat((gchar *)pad_str, line_num, NULL);	
	g_free(pad_str);
	g_free(line_num);	
		
	save_wrapping = pji->wrapping;
	save_x = pji->cursor_x;
	save_y = pji->cursor_y;
	
	pji->wrapping = FALSE;
	pji->cursor_x = pji->margin_left;
	gnome_print_moveto (pji->pc, pji->cursor_x, pji->cursor_y);
	anjuta_print_show_chars_styled(pji, (const char *)text, strlen((const char *)text), 
								  AN_PRINT_LINENUMBER_STYLE);

	pji->wrapping = save_wrapping;
	pji->cursor_x = save_x;
	pji->cursor_y = save_y;
	g_free (text);
}

static void
anjuta_print_begin (PrintJobInfo * pji)
{
	gchar page[256];
	
	pji->current_page = 1;
	
	sprintf(page, "%d", pji->current_page);
	gnome_print_beginpage (pji->pc, (const guchar *)page);
	if (pji->print_header)
		anjuta_print_show_header(pji);
	pji->cursor_y = pji->page_height - pji->margin_top - pji->margin_header;
	pji->cursor_x = pji->margin_left + pji->margin_numbers;
}

static void
anjuta_print_end(PrintJobInfo * pji)
{
	gnome_print_showpage(pji->pc);
	gnome_print_context_close (pji->pc);
}

static void
anjuta_print_progress_response (GtkWidget *dialog, gint res, gpointer data)
{
	PrintJobInfo *pji = (PrintJobInfo *) data;

	if (pji->progress_dialog == NULL)
		return;
	pji->canceled = TRUE;
	gtk_widget_destroy (dialog);
	pji->progress_dialog = NULL;
}

static gboolean
anjuta_print_progress_delete_event (GtkWidget *dialog,
									GdkEvent *event, gpointer data)
{
	PrintJobInfo *pji = (PrintJobInfo *) data;

	if (pji->progress_dialog == NULL)
		return FALSE;
	pji->progress_dialog = NULL;
	pji->canceled = TRUE;
	return FALSE;
}

static void
anjuta_print_progress_start(PrintJobInfo * pji)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *progress_bar;
	GtkWidget *window;

	window = gtk_widget_get_toplevel (pji->te->scintilla);
	dialog = gtk_dialog_new_with_buttons (_("Printing..."),
										  GTK_WINDOW (window),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CANCEL,
										  GTK_RESPONSE_CANCEL, NULL);
	g_signal_connect (dialog, "response",
	  				  G_CALLBACK (anjuta_print_progress_response), pji);
	g_signal_connect (dialog, "delete_event",
	  				  G_CALLBACK (anjuta_print_progress_delete_event), pji);
	pji->progress_dialog = dialog;
	gtk_widget_show (dialog);
	
	label = gtk_label_new (_("Printing..."));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox),
								 label, FALSE, FALSE, 0);
	
	progress_bar = gtk_progress_bar_new();
	gtk_widget_show (progress_bar);
	pji->progress_bar = progress_bar;
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox),
								 progress_bar, FALSE, FALSE, 0);
}

static void
anjuta_print_progress_tick(PrintJobInfo * pji, guint index)
{
	gfloat percentage;

	while (gtk_events_pending())
		gtk_main_iteration ();
	if (pji->progress_dialog == NULL)
		return;
	percentage = (float)index/pji->buffer_size;
	if (percentage < 0.0) percentage = 0.0;
	if (percentage > 1.0) percentage = 1.0;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pji->progress_bar),
								   percentage);
}

static void
anjuta_print_progress_end (PrintJobInfo * pji)
{
	if (pji->progress_dialog == NULL)
		return;
	gtk_widget_destroy (pji->progress_dialog);
	pji->progress_dialog = NULL;
	pji->progress_bar = NULL;
}

static void
anjuta_print_document (PrintJobInfo * pji)
{
	guint i, style_index, ret, current_line, num_lines, padding;
	gchar *current_pos;

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
			break;
	}
	
	num_lines = text_editor_get_total_lines (pji->te)+1;
	
	for (padding = 1; num_lines >= 10; padding++)
		num_lines /= 10;
	
	anjuta_print_begin (pji);
	
	if (pji->print_line_numbers > 0)
		anjuta_print_show_linenum (pji, current_line, padding);
	
	current_pos = pji->buffer;
	style_index = 0;
	i = 0;
	while (current_pos < (pji->buffer + pji->buffer_size))
	{
		gchar *previous_pos;
		gchar style;
		
		ret = 0;
		/* text = TEXT_AT(pji->buffer, i); */
		style = STYLE_AT(pji, style_index);
		
		if (current_pos[0] == '\t') {
			int j;
			for (j = 0; j < pji->tab_size; j++ ) {
				ret = anjuta_print_show_chars_styled(pji, " ", 1, style);
				if (ret == 1) break;
			}
		} else {
			ret = anjuta_print_show_chars_styled(pji, current_pos,
												 g_utf8_next_char(current_pos)
													- current_pos,
												 style);
		}
		/* Skip to next line */
		if (ret == 1) {
			while ((current_pos < (pji->buffer + pji->buffer_size))
					&& (current_pos[0] != '\n')) {
						
				/* Advance to next character */
				previous_pos = current_pos;
				current_pos = g_utf8_next_char (current_pos);
				style_index += current_pos - previous_pos;
				i++;
			}
		}
		if (current_pos[0] == '\n' || ret == 1) {
			current_line++;
			if (pji->print_line_numbers > 0 &&
					((current_line) % pji->print_line_numbers == 0))
				anjuta_print_show_linenum (pji, current_line, padding);
		}
		if (i % 50 ==  0)
			anjuta_print_progress_tick (pji, i);
		
		/* advance to next character. */
		previous_pos = current_pos;
		current_pos = g_utf8_next_char (current_pos);
		style_index += current_pos - previous_pos;
		i++;
		
		/* Exit if canceled. */
		if (pji->canceled)
			break;
	}
	anjuta_print_end (pji);
	anjuta_print_progress_end (pji);
}

static gboolean
anjuta_print_run_dialog(PrintJobInfo *pji)
{
	GtkWidget *dialog;
	gint selection_flag;
	gint lines;

	if (text_editor_has_selection (pji->te))
		selection_flag = GNOME_PRINT_RANGE_SELECTION;
	else
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	
	dialog = g_object_new (GNOME_TYPE_PRINT_DIALOG, "print_config",
						   pji->config, NULL);

	gnome_print_dialog_construct (GNOME_PRINT_DIALOG (dialog),
								  (const guchar *)_("Print"),
								  GNOME_PRINT_DIALOG_RANGE |
								  GNOME_PRINT_DIALOG_COPIES);
	
	lines = text_editor_get_total_lines (pji->te);
	
	gnome_print_dialog_construct_range_page (GNOME_PRINT_DIALOG (dialog),
											 GNOME_PRINT_RANGE_ALL |
											 selection_flag,
											 1, lines, (const guchar *)"A", (const guchar *)_("Lines"));
	switch (gtk_dialog_run(GTK_DIALOG (dialog)))
	{
		case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
			break;
		case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
			pji->preview = TRUE;
			break;
		case -1:
			return TRUE;
		default:
			gtk_widget_destroy (GTK_WIDGET (dialog));
			return TRUE;
	}
	
	pji->range_type = gnome_print_dialog_get_range_page(GNOME_PRINT_DIALOG (dialog),
						&pji->range_start_line,
						&pji->range_end_line);
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	return FALSE;
}

static void
anjuta_print_preview_real (PrintJobInfo *pji)
{
	GtkWidget *gpmp = gnome_print_job_preview_new (pji->print_job,
												   (const guchar *)_("Print Preview"));
	gtk_widget_show (GTK_WIDGET (gpmp));
}

void
anjuta_print (gboolean preview, AnjutaPreferences *p, TextEditor *te)
{
	PrintJobInfo *pji;
	gboolean cancel = FALSE;

	scintilla_send_message (SCINTILLA (te->scintilla), SCI_COLOURISE, 0, -1);
	
	if (NULL == (pji = anjuta_print_job_info_new(p, te)))
		return;
	pji->preview = preview;
	if (!pji->preview)
		cancel = anjuta_print_run_dialog(pji);
	if (cancel)
	{
		anjuta_print_job_info_destroy(pji);
		return;
	}
	g_return_if_fail (pji->config != NULL);

	pji->print_job = gnome_print_job_new (pji->config);
	g_return_if_fail (pji->print_job != NULL);

	pji->pc = gnome_print_job_get_context (pji->print_job);
	g_return_if_fail (pji->pc != NULL);

	anjuta_print_update_page_size_and_margins (pji);
	
	anjuta_print_document(pji);
	
	if (pji->canceled)
	{
		anjuta_print_job_info_destroy(pji);
		return;
	}
	
	gnome_print_job_close (pji->print_job);
	
	if (pji->preview)
		anjuta_print_preview_real(pji);
	else
		gnome_print_job_print (pji->print_job);
	
	anjuta_print_job_info_destroy (pji);
}
