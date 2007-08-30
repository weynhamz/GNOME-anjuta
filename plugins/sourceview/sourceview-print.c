/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#include "sourceview-print.h"
#include "sourceview-private.h"

#include <libanjuta/anjuta-debug.h>

#include <libgnomeprintui/gnome-print-job-preview.h>
#include <libgnomeprintui/gnome-print-dialog.h>

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>
#include <gtksourceview/gtksourceprintjob.h>

#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document.h>

static GtkSourcePrintJob*
create_print_job(Sourceview* sv)
{
	GtkSourcePrintJob *job;
	GtkSourceView *view;
	GtkSourceBuffer *buffer;
	const gchar *filename;

	g_return_val_if_fail (sv != NULL, NULL);
	
	view = GTK_SOURCE_VIEW(sv->priv->view);
	buffer = GTK_SOURCE_BUFFER (sv->priv->document);
	
	job = gtk_source_print_job_new (NULL);
	gtk_source_print_job_setup_from_view (job, view);
	gtk_source_print_job_set_wrap_mode (job, GTK_WRAP_CHAR);
	gtk_source_print_job_set_highlight (job, TRUE);
	gtk_source_print_job_set_print_numbers (job, 1);

	gtk_source_print_job_set_header_format (job,
						"Printed on %A",
						NULL,
						"%F",
						TRUE);

	filename = ianjuta_document_get_filename(IANJUTA_DOCUMENT(sv), NULL);
	
	gtk_source_print_job_set_footer_format (job,
						"%T",
						filename,
						"Page %N/%Q",
						TRUE);

	gtk_source_print_job_set_print_header (job, TRUE);
	gtk_source_print_job_set_print_footer (job, TRUE);
	
	return job;
}

static GtkWidget *
sourceview_print_dialog_new (GtkSourcePrintJob *job, GtkSourceBuffer* buffer)
{
	GtkWidget *dialog;
	gint selection_flag;
	gint lines;
	GnomePrintConfig *config;

	if (!gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (buffer), NULL, NULL))
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	else
		selection_flag = GNOME_PRINT_RANGE_SELECTION;

	config = gtk_source_print_job_get_config (GTK_SOURCE_PRINT_JOB (job));

	dialog = g_object_new (GNOME_TYPE_PRINT_DIALOG, "print_config", config, NULL);

	gnome_print_dialog_construct (GNOME_PRINT_DIALOG (dialog), 
				      (const unsigned char *)_("Print"),
			              GNOME_PRINT_DIALOG_RANGE | GNOME_PRINT_DIALOG_COPIES);

	lines = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (buffer));

	gnome_print_dialog_construct_range_page (GNOME_PRINT_DIALOG (dialog),
						 GNOME_PRINT_RANGE_ALL |
						 GNOME_PRINT_RANGE_RANGE |
						 selection_flag,
						 1, lines, (const unsigned char *)"A",
						 (const unsigned char *)_("Lines"));

	return dialog;
}

void 
sourceview_print(Sourceview* sv)
{
	GtkSourcePrintJob* job = create_print_job(sv);
	GtkWidget* print= sourceview_print_dialog_new(job, 
		GTK_SOURCE_BUFFER(sv->priv->document));
	GnomePrintButtons result = gtk_dialog_run(GTK_DIALOG(print));
	
	switch (result)
	{
		case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
			sourceview_print_preview(sv);
			break;
		case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
		{
			GnomePrintJob* gjob = gtk_source_print_job_print (job);
			gnome_print_job_print(gjob);
			g_object_unref(gjob);
			break;
		}
		case GNOME_PRINT_DIALOG_RESPONSE_CANCEL:
			break;
		default:
			DEBUG_PRINT("Unknown print response");
	}
	gtk_widget_destroy(print);
	g_object_unref(job);
}

void
sourceview_print_preview(Sourceview* sv)
{
	GtkSourcePrintJob* job;
	GnomePrintJob* gjob;
	GtkWidget *preview;
	
	job = create_print_job(sv);

	gjob = gtk_source_print_job_print (job);
	preview = gnome_print_job_preview_new (gjob,
		(const guchar *)_("Print preview"));
 	
 	gtk_widget_show(preview);
	
	g_object_unref(job);
	g_object_unref(gjob);
}

