/*
** Gnome-Print support
** Author: Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
** Original Author: Chema Celorio <chema@celorio.com>
** Licence: GPL
*/

#ifndef AN_PRINTING_PRINT_H
#define AN_PRINTING_PRINT_H

#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include <libgnomeprint/gnome-print-preview.h>

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

/* FIXME: These should not really be hardcoded but I've no idea how to
restrict the user to choosing fixed width Type1 fonts - any ideas ? */
#define AN_PRINT_FONT_BODY   "Courier"
#define AN_PRINT_FONT_HEADER "Helvetica"
#define AN_PRINT_FONT_SIZE_BODY   10
#define AN_PRINT_FONT_SIZE_HEADER 10
#define AN_PRINT_FONT_SIZE_NUMBERS 6

typedef struct _PrintJobInfo
{
	GnomePrintMaster *master;
	GnomePrintContext *pc;
	GnomePrinter *printer;
	const GnomePaper *paper;

	TextEditor *te;
	gchar *filename;
	guchar *buffer;
	guint buffer_size;
	gboolean print_header;
	gint print_line_numbers;

	float font_char_width;
	float font_char_height;
	GnomeFont *font_body;
	GnomeFont *font_header;
	GnomeFont *font_numbers;

	guint pages;
	float page_width, page_height;
	float margin_top, margin_bottom, margin_left, margin_right,
		margin_numbers;
	float printable_width, printable_height;
	float header_height;
	guint total_lines, total_lines_real;
	guint lines_per_page;
	guint chars_per_line;
	guchar *temp;
	AnPrintOrientation orientation;

	gint range;
	gint page_first;
	gint page_last;
	gboolean print_this_page;
	gboolean preview;

	gint file_offset;
	gint current_line;

	gboolean wrapping;
	gint tab_size;

	/* Progress */
	GtkWidget *progress_bar;
	GnomeDialog *progress_dialog;
	gboolean canceled;
} PrintJobInfo;

void anjuta_print_cb(GtkWidget * widget, gpointer notused);
void anjuta_print_preview_cb(GtkWidget * widget, gpointer notused);

#ifdef __cplusplus
}
#endif

#endif /* AN_PRINTING_PRINT_H */
