/*
 * print-doc.c
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

#include <gnome.h>
#include <libgnomeprint/gnome-print.h>

#include "print.h"
#include "print-doc.h"
#include "print-util.h"

static gboolean anjuta_print_run_dialog(PrintJobInfo *pji)
{
	GnomeDialog *dialog;
	gint selection_flag;

	if (text_editor_has_selection (pji->te))
		selection_flag = GNOME_PRINT_RANGE_SELECTION;
	else
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	dialog = (GnomeDialog *) gnome_print_dialog_new(_("Print Document"),
		GNOME_PRINT_DIALOG_RANGE);
	gnome_print_dialog_construct_range_page ((GnomePrintDialog *) dialog,
		GNOME_PRINT_RANGE_ALL | GNOME_PRINT_RANGE_RANGE | selection_flag,
		1, text_editor_get_total_lines(pji->te), "A", _("Lines"));
	
	switch (gnome_dialog_run(GNOME_DIALOG (dialog)))
	{
		case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
			break;
		case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
			pji->preview = TRUE;
			break;
		case -1:
			return TRUE;
		default:
			gnome_dialog_close(GNOME_DIALOG (dialog));
			return TRUE;
	}
	
/*
	pji->printer = gnome_print_dialog_get_printer(GNOME_PRINT_DIALOG(dialog));
	if (pji->printer && !pji->preview)
		gnome_print_master_set_printer(pji->master, pji->printer);
*/	
	pji->range_type = gnome_print_dialog_get_range_page(GNOME_PRINT_DIALOG (dialog),
						&pji->range_start_line,
						&pji->range_end_line);
	
	gnome_dialog_close (GNOME_DIALOG (dialog));
	return FALSE;
}

static void anjuta_print_preview_real(PrintJobInfo *pji)
{
	/* gchar *title = g_strdup_printf (); */
	GnomePrintMasterPreview *gpmp =
		gnome_print_master_preview_new (pji->master, _("Print Preview"));
	/* g_free (title); */
	gtk_widget_show (GTK_WIDGET (gpmp));
}

static void anjuta_print(gboolean preview)
{
	PrintJobInfo *pji;
	gboolean cancel = FALSE;

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
