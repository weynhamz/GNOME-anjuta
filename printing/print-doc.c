/*
** Gnome-Print support
** Author: Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
** Original Author: Chema Celorio <chema@celorio.com>
** Licence: GPL
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include <libgnomeprint/gnome-print.h>

#include "print.h"
#include "print-doc.h"
#include "print-util.h"

static void print_line (PrintJobInfo * pji, int line);
static void print_ps_line (PrintJobInfo * pji, gint line, gint first_line);
static void print_header (PrintJobInfo * pji, unsigned int page);
static void print_start_job (PrintJobInfo * pji);
static void print_set_orientation (PrintJobInfo * pji);
static void print_header (PrintJobInfo * pji, unsigned int page);
static void print_end_page (PrintJobInfo * pji);
static void print_end_job (GnomePrintContext * pc);

void anjuta_print_document(PrintJobInfo * pji)
{
	int current_page, current_line;
	pji->temp = g_malloc (pji->chars_per_line + 2);
	current_line = 0;
	anjuta_print_progress_start (pji);
	print_start_job (pji);
	for (current_page = 1; current_page <= pji->pages; current_page++)
	{
		if (pji->range != GNOME_PRINT_RANGE_ALL)
		  pji->print_this_page = (current_page >= pji->page_first
		  && current_page <= pji->page_last) ? TRUE : FALSE;
		else
			pji->print_this_page = TRUE;
		if (pji->print_this_page)
		{
			gchar *pagenumbertext;
			pagenumbertext = g_strdup_printf ("%d", current_page);
			gnome_print_beginpage(pji->pc, pagenumbertext);
			g_free (pagenumbertext);
			if (pji->print_header)
				print_header(pji, current_page);
			gnome_print_setfont(pji->pc, pji->font_body);
		}
		if (pji->buffer[pji->file_offset - 1] != '\n' && current_page > 1 && pji->wrapping)
			current_line--;

		for (current_line++; current_line <= pji->total_lines; current_line++)
		{
			print_line(pji, current_line);
			if (pji->current_line % pji->lines_per_page == 0)
				break;
		}

		if (pji->print_this_page)
			print_end_page (pji);

		anjuta_print_progress_tick (pji, current_page);
		if (pji->canceled)
			break;
	}
	print_end_job (pji->pc);
	g_free (pji->temp);

	gnome_print_context_close (pji->pc);

	anjuta_print_progress_end (pji);
}


static void print_line (PrintJobInfo * pji, int line)
{
	gint chars_in_this_line = 0;
	gint i, temp;
	gint first_line = TRUE;

	while (pji->buffer[pji->file_offset] != '\n' && pji->file_offset < pji->buffer_size)
	{
		chars_in_this_line++;

		if (pji->buffer[pji->file_offset] != '\t')
		{
			pji->temp[chars_in_this_line - 1] =
				pji->buffer[pji->file_offset];
		}
		else
		{
			temp = chars_in_this_line;
			chars_in_this_line += pji->tab_size - ((chars_in_this_line - 1) % pji->tab_size) - 1;
			if (chars_in_this_line > pji->chars_per_line + 1)
				chars_in_this_line = pji->chars_per_line + 1;
			for (i = temp; i < chars_in_this_line + 1; i++)
				pji->temp[i - 1] = ' ';
		}
		if (chars_in_this_line < pji->chars_per_line + 1)
		{
			pji->file_offset++;
			continue;
		}
		if (!pji->wrapping)
		{
			while (pji->buffer[pji->file_offset] != '\n')
				pji->file_offset++;
			pji->file_offset--;
			pji->temp[chars_in_this_line] = (guchar) '\0';
			print_ps_line (pji, line, TRUE);
			pji->current_line++;
			chars_in_this_line = 0;
			pji->file_offset++;
			continue;
		}
		temp = pji->file_offset;
		while (pji->buffer[pji->file_offset] != ' ' &&
		       pji->buffer[pji->file_offset] != '\t' &&
		       pji->file_offset > temp - pji->chars_per_line - 1)
		{
			pji->file_offset--;
		}
		if (pji->file_offset == temp - pji->chars_per_line - 1)
		{
			pji->file_offset = temp;
		}
		pji->temp[chars_in_this_line + pji->file_offset - temp - 1] = (guchar) '\0';
		print_ps_line (pji, line, first_line);
		first_line = FALSE;
		pji->current_line++;
		chars_in_this_line = 0;

		while (pji->buffer[pji->file_offset] == ' '
		       || pji->buffer[pji->file_offset] == '\t')
			pji->file_offset++;
		if (pji->current_line % pji->lines_per_page == 0)
			return;
	}
	pji->temp[chars_in_this_line] = (guchar) '\0';
	print_ps_line (pji, line, first_line);
	pji->current_line++;
	pji->file_offset++;
}

static void print_ps_line(PrintJobInfo * pji, gint line, gint first_line)
{
	gfloat y;

	y = pji->page_height - pji->margin_top - pji->header_height -
		(pji->font_char_height * ((pji->current_line % pji->lines_per_page) + 1));

	if (!pji->print_this_page)
		return;

	gnome_print_moveto (pji->pc, pji->margin_left, y);
	anjuta_print_show (pji->pc, pji->temp);

	if (pji->print_line_numbers > 0 &&
	    line % pji->print_line_numbers == 0 && first_line)
	{
		char *number_text = g_strdup_printf("%i", line);

		gnome_print_setfont(pji->pc, pji->font_numbers);
		gnome_print_moveto(pji->pc, pji->margin_left - pji->margin_numbers, y);
		anjuta_print_show(pji->pc, number_text);
		g_free(number_text);
		gnome_print_setfont (pji->pc, pji->font_body);
	}
}

guint anjuta_print_document_determine_lines(PrintJobInfo * pji, gboolean real)
{
	gint lines = 0;
	gint i, temp_i;
	gint chars_in_this_line = 0;
	guchar *buffer = pji->buffer;
	gint chars_per_line = pji->chars_per_line;
	gint tab_size = pji->tab_size;
	gint wrapping = pji->wrapping;
	gint buffer_size = pji->buffer_size;

	int dump_info = FALSE;
	int dump_info_basic = FALSE;
	int dump_text = FALSE;

	if (real && !wrapping)
		real = FALSE;
	if (!real)
	{
		dump_info = FALSE;
		dump_info_basic = FALSE;
		dump_text = FALSE;
	}
	for (i = 0; i < buffer_size; i++)
	{
		chars_in_this_line++;
		if (buffer[i] == '\n')
		{
			lines++;
			chars_in_this_line = 0;
			continue;
		}
		if (buffer[i] == '\t')
		{
			temp_i = chars_in_this_line;
			chars_in_this_line += tab_size -	((chars_in_this_line - 1) % tab_size) - 1;
			if (chars_in_this_line > chars_per_line + 1)
				chars_in_this_line = chars_per_line + 1;
		}
		if (chars_in_this_line == chars_per_line + 1 && real)
		{
			temp_i = i;
			while (buffer[i] != ' ' && buffer[i] != '\t' && i > temp_i - chars_per_line - 1)
				i--;
			if (i == temp_i - chars_per_line - 1)
				i = temp_i;
			temp_i = i;
			while (buffer[i] == ' ' || buffer[i] == '\t')
				i++;
			i--;
			lines++;
			chars_in_this_line = 0;
		}

	}
	if (buffer[i] != '\n')
		lines++;
	temp_i = lines;
	for (i = buffer_size - 1; i > 0; i--)
	{
		if (buffer[i] != '\n' && buffer[i] != ' '
		    && buffer[i] != '\t')
			break;
		else if (buffer[i] == '\n')
			lines--;
	}
	return lines;
}

static void print_start_job (PrintJobInfo * pji)
{
	print_set_orientation(pji);
}

static void print_set_orientation (PrintJobInfo * pji)
{
	double affine[6];

	if (pji->orientation == PRINT_ORIENT_PORTRAIT)
		return;
	art_affine_rotate(affine, 90.0);
	gnome_print_concat(pji->pc, affine);
	art_affine_translate(affine, 0, -pji->page_height);
	gnome_print_concat(pji->pc, affine);
}

static void print_header (PrintJobInfo * pji, unsigned int page)
{
	guchar *text1 = g_strdup (pji->filename);
	guchar *text2 = g_strdup_printf (_("Page: %i/%i"), page, pji->pages);
	float x, y, len;

	gnome_print_setfont (pji->pc, pji->font_header);
	y = pji->page_height - pji->margin_top - pji->header_height / 2;
	len = gnome_font_get_width_string (pji->font_header, text1);
	x = pji->page_width / 2 - len / 2;
	gnome_print_moveto (pji->pc, x, y);
	anjuta_print_show (pji->pc, text1);
	y = pji->page_height - pji->margin_top - pji->header_height / 4;
	len = gnome_font_get_width_string (pji->font_header, text2);
	x = pji->page_width - len - 36;
	gnome_print_moveto (pji->pc, x, y);
	anjuta_print_show (pji->pc, text2);
	g_free (text1);
	g_free (text2);
}

static void print_end_page(PrintJobInfo * pji)
{
	gnome_print_showpage(pji->pc);
	print_set_orientation(pji);
}

static void print_end_job(GnomePrintContext * pc)
{
}
