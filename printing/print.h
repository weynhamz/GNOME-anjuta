/*
 * print.h
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

#ifndef AN_PRINTING_PRINT_H
#define AN_PRINTING_PRINT_H

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-master-preview.h>
#include <libgnomeprintui/gnome-print-preview.h>

#include "text_editor.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _AnPrintOrientation
{
	PRINT_ORIENT_PORTRAIT,
	PRINT_ORIENT_LANDSCAPE
} AnPrintOrientation;

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
#define TEXT_AT(buf, index)  (buf)[(index)*2]
#define STYLE_AT(buf, index) (buf)[(index)*2+1]

typedef struct _PrintJobInfoStyle
{
	gchar          *font_name;
	GnomeFont      *font;
	GdkColor        fore_color;
	GdkColor        back_color;
	gboolean        italics;
	GnomeFontWeight weight;
	gint            size;
} PrintJobInfoStyle;

typedef struct _PrintJobInfo
{
	GnomePrintMaster *master;
	GnomePrintContext *pc;
	// GnomePrint *printer;
	const GnomePrintPaper *paper;
	
	PrintJobInfoStyle* styles_pool[AN_PRINT_MAX_STYLES];

	TextEditor *te;

	/* Print Buffer */
	guchar  *buffer;
	guint   buffer_size;
	
	/* Document zoom factor */
	gint zoom_factor;
	
	/* Preferences */
	gfloat page_width;
	gfloat page_height;
	gfloat margin_top;
	gfloat margin_bottom;
	gfloat margin_left;
	gfloat margin_right;
	gfloat margin_header;
	gfloat margin_numbers;
	
	gboolean print_header;
	gboolean print_color;
	gint     print_line_numbers;
	gboolean preview;
	gboolean wrapping;
	gint     tab_size;
	
	AnPrintOrientation orientation;

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

PrintJobInfo* anjuta_print_job_info_new (void);
void anjuta_print_job_info_destroy(PrintJobInfo *pji);
void anjuta_print_begin (PrintJobInfo * pji);
void anjuta_print_end (PrintJobInfo * pji);
void anjuta_print_document(PrintJobInfo * pji);
void anjuta_print_set_style (PrintJobInfo *pji, gint style);
void anjuta_print_set_orientation (PrintJobInfo * pji);
gfloat anjuta_print_get_width (PrintJobInfo *pji, gint style, const char *str, gint len);
void anjuta_print_new_line (PrintJobInfo *pji);
void anjuta_print_new_page (PrintJobInfo *pji);
gint anjuta_print_show_char_styled (PrintJobInfo *pji, const char ch, const char style);
void anjuta_print_show_linenum (PrintJobInfo * pji, guint line, guint padding);

#ifdef __cplusplus
}
#endif

#endif /* AN_PRINTING_PRINT_H */
