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

static void anjuta_print_calculate_pages (PrintJobInfo *pji)
{
	pji->total_lines = anjuta_print_document_determine_lines(pji, FALSE);
	pji->total_lines_real = anjuta_print_document_determine_lines(pji, TRUE);
	pji->pages = ((int) (pji->total_lines_real-1)/pji->lines_per_page)+1;
	pji->page_first = 1;
	pji->page_last = pji->pages;
}

static void anjuta_print_job_info_destroy(PrintJobInfo *pji)
{
	if (pji)
	{
		if (pji->master) gnome_print_master_close(pji->master);
		if (pji->buffer) g_free(pji->buffer);
		if (pji->filename) g_free(pji->filename);
		if (pji->font_body) gnome_font_unref(pji->font_body);
		if (pji->font_header) gnome_font_unref(pji->font_header);
		if (pji->font_numbers) gnome_font_unref(pji->font_numbers);
		g_free(pji);
	}
}

static PrintJobInfo *anjuta_print_job_info_new (void)
{
	PrintJobInfo *pji;
	Preferences *p = app->preferences;
	gint pref_val;
	char *paper_name;

	pji = g_new0(PrintJobInfo, 1);
	if (NULL == (pji->te = anjuta_get_current_text_editor()))
	{
		anjuta_error(_("No file to print!"));
		g_free(pji);
		return NULL;
	}
	pji->preview = FALSE;
	if (NULL == pji->te->filename)
		pji->filename = g_strdup("Untitled");
	else
		pji->filename = gnome_vfs_unescape_string_for_display (pji->te->full_filename);
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
	pji->buffer_size = scintilla_send_message(SCINTILLA(pji->te->widgets.editor), SCI_GETLENGTH, 0, 0);
	pji->buffer = (gchar *) aneditor_command(pji->te->editor_id, ANE_GETTEXTRANGE, 0, pji->buffer_size);
	if (NULL == pji->buffer)
	{
		anjuta_error(_("Unable to get text buffer for printing"));
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	if (NULL == (paper_name = preferences_get(p, PAPER_SIZE)))
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
	if (preferences_get_int(p, PRINT_LANDSCAPE))
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
	pji->margin_numbers = .25 * 72;
	pji->margin_top = .75 * 72;
	pji->margin_bottom = .75 * 72;
	pji->margin_left = .75 * 72;
	pji->margin_right = .75 * 72;
	pji->header_height = 72;
	pref_val = preferences_get_int(p, PRINT_LINENUM);
	if (pref_val)
		pref_val = preferences_get_int(p, PRINT_LINECOUNT);
	if (0 > pref_val)
		pref_val = 0;
	if (pref_val > 0)
		pji->margin_left += pji->margin_numbers;
	pji->printable_width  = pji->page_width - pji->margin_left - pji->margin_right -
	  ((pref_val > 0)?pji->margin_numbers:0);
	pji->printable_height = pji->page_height - pji->margin_top - pji->margin_bottom;
	
	/* FIXME: get font and font characteristics from preferences */
	pji->font_char_width  = 0.0808 * 72;
	pji->font_char_height = .14 * 72;
	pji->font_body = gnome_font_new(AN_PRINT_FONT_BODY, AN_PRINT_FONT_SIZE_BODY);
	pji->font_header = gnome_font_new(AN_PRINT_FONT_HEADER, AN_PRINT_FONT_SIZE_HEADER);
	pji->font_numbers = gnome_font_new(AN_PRINT_FONT_BODY, AN_PRINT_FONT_SIZE_NUMBERS);

	if (!GNOME_IS_FONT(pji->font_body))
	{
		anjuta_error(_("Unable to get body font %s"), AN_PRINT_FONT_BODY);
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	if (!GNOME_IS_FONT(pji->font_header))
	{
		anjuta_error(_("Unable to get header font %s"), AN_PRINT_FONT_HEADER);
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}
	if (!GNOME_IS_FONT(pji->font_numbers))
	{
		anjuta_error(_("Unable to get line-number font %s"), AN_PRINT_FONT_BODY);
		anjuta_print_job_info_destroy(pji);
		return NULL;
	}

	pji->wrapping = preferences_get_int(p, PRINT_WRAP);
	pji->print_line_numbers = pref_val;
	pji->print_header = preferences_get_int(p, PRINT_HEADER);
	pji->chars_per_line = (gint)(pji->printable_width / pji->font_char_width);
	pji->lines_per_page = ((pji->printable_height - pji->header_height) / pji->font_char_height -  1);
	pji->tab_size = preferences_get_int(p, TAB_SIZE);
	pji->range = GNOME_PRINT_RANGE_ALL;
	anjuta_print_calculate_pages(pji);
	pji->canceled = FALSE;
	return pji;
}


static void anjuta_print_range_is_selection(PrintJobInfo *pji)
{
	if (pji->buffer) g_free(pji->buffer);
	pji->buffer = text_editor_get_selection(pji->te);
	g_return_if_fail(pji->buffer!=NULL);
	pji->buffer_size = strlen(pji->buffer);
	anjuta_print_calculate_pages(pji);
	pji->page_first = 1;
	pji->page_last = pji->pages;
}

static gboolean anjuta_print_run_dialog(PrintJobInfo *pji)
{
	GnomeDialog *dialog;
	gint selection_flag;
	gchar *buf;
	guint buflen = 0;

	buf = text_editor_get_selection(pji->te);
	if (NULL != buf)
		buflen = strlen(buf);
	if (NULL != buf && 0 < buflen)
		selection_flag = GNOME_PRINT_RANGE_SELECTION;
	else
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	dialog = (GnomeDialog *) gnome_print_dialog_new(_("Print Document")
	  , GNOME_PRINT_DIALOG_RANGE);
	gnome_print_dialog_construct_range_page ((GnomePrintDialog *) dialog
	  , GNOME_PRINT_RANGE_ALL | GNOME_PRINT_RANGE_RANGE | selection_flag
	  , 1, pji->pages, "A", _("Pages"));
	switch (gnome_dialog_run(GNOME_DIALOG (dialog)))
	{
		case GNOME_PRINT_PRINT:
			break;
		case GNOME_PRINT_PREVIEW:
			pji->preview = TRUE;
			break;
		case -1:
			return TRUE;
		default:
			gnome_dialog_close(GNOME_DIALOG (dialog));
			return TRUE;
	}
	pji->printer = gnome_print_dialog_get_printer(GNOME_PRINT_DIALOG(dialog));
	if (pji->printer && !pji->preview)
		gnome_print_master_set_printer(pji->master, pji->printer);
	pji->range = gnome_print_dialog_get_range_page(GNOME_PRINT_DIALOG (dialog)
	  , &pji->page_first, &pji->page_last);
	if (pji->range == GNOME_PRINT_RANGE_SELECTION)
		anjuta_print_range_is_selection(pji);
	gnome_dialog_close (GNOME_DIALOG (dialog));
	return FALSE;
}
	
static void anjuta_print_preview_real(PrintJobInfo *pji)
{
	gchar *title = g_strdup_printf (_("Print Preview"));
	GnomePrintMasterPreview *gpmp = gnome_print_master_preview_new (pji->master, title);
	g_free (title);
	gtk_widget_show (GTK_WIDGET (gpmp));
}

static void anjuta_print(gboolean preview)
{
	PrintJobInfo *pji;
	gboolean cancel = FALSE;

	if (!anjuta_print_verify_fonts())
		return;
	if (NULL == (pji = anjuta_print_job_info_new()))
		return;
	pji->preview = preview;
	if (!pji->preview)
		cancel = anjuta_print_run_dialog(pji);
	if (cancel)
	{
		anjuta_print_job_info_destroy(pji);
		return;
	}
	anjuta_print_document(pji);
	if (pji->canceled)
	{
		anjuta_print_job_info_destroy(pji);
		return;
	}
	if (pji->preview)
		anjuta_print_preview_real(pji);
	else
		gnome_print_master_print(pji->master);
	anjuta_print_job_info_destroy (pji);
}

void anjuta_print_cb (GtkWidget *widget, gpointer notused)
{
	anjuta_print(FALSE);
}

void anjuta_print_preview_cb(GtkWidget *widget, gpointer notused)
{
	anjuta_print(TRUE);
}
