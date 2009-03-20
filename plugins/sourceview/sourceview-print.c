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
#include <gtksourceview/gtksourceprintcompositor.h>
#include <libanjuta/interfaces/ianjuta-document.h>

#define PRINT_LINEWRAP "print.linewrap"
#define PRINT_HEADER "print.header"
#define PRINT_FOOTER "print.footer"
#define PRINT_HIGHLIGHT "print.highlight"
#define PRINT_LINENUMBERS "print.linenumbers"

typedef struct _SourceviewPrinting SourceviewPrinting;

struct _SourceviewPrinting
{
	Sourceview* sv;
	GtkSourcePrintCompositor *compositor;
	AnjutaStatus* status;
};


static gboolean
paginate (GtkPrintOperation        *operation, 
					GtkPrintContext          *context,
					SourceviewPrinting* printing)
{
	if (gtk_source_print_compositor_paginate (printing->compositor, context))
	{
		gint n_pages;	
		anjuta_status_progress_tick (printing->status, NULL,
																 _("Preparing pages for printing"));
		n_pages = gtk_source_print_compositor_get_n_pages (printing->compositor);
		gtk_print_operation_set_n_pages (operation, n_pages);

		return TRUE;
	}
     
	return FALSE;
}


static void
draw_page (GtkPrintOperation        *operation,
					 GtkPrintContext          *context,
					 gint                      page_nr,
					 SourceviewPrinting* printing)
{
	gtk_source_print_compositor_draw_page (printing->compositor, context, page_nr);
}

static void
end_print (GtkPrintOperation        *operation, 
					 GtkPrintContext          *context,
					 SourceviewPrinting* printing)
{
	g_object_unref (printing->compositor);
	g_slice_free (SourceviewPrinting, printing);
}

static GtkPrintOperation*
print_setup (Sourceview* sv)
{
	GtkSourceView *view = GTK_SOURCE_VIEW (sv->priv->view);
	GtkSourcePrintCompositor *compositor;
	GtkPrintOperation *operation;
	SourceviewPrinting* printing = g_slice_new0(SourceviewPrinting);
	const gchar *filename;
	gchar *basename;
	
	filename = ianjuta_document_get_filename (IANJUTA_DOCUMENT (sv), NULL);
	basename = g_filename_display_basename (filename);
	
	compositor = gtk_source_print_compositor_new_from_view (view);
	
	if (anjuta_preferences_get_int (sv->priv->prefs, PRINT_LINEWRAP))
	{
		gtk_source_print_compositor_set_wrap_mode (compositor,
																							 GTK_WRAP_WORD_CHAR);
	}
	else
		gtk_source_print_compositor_set_wrap_mode (compositor,
																						 GTK_WRAP_NONE);
	
	gtk_source_print_compositor_set_print_line_numbers (compositor,
																											anjuta_preferences_get_bool (sv->priv->prefs,
																											                             PRINT_LINENUMBERS));
	
	gtk_source_print_compositor_set_header_format (compositor,
																								 TRUE,
																								 "%x",
																								 basename,
																								 "Page %N/%Q");
	
	gtk_source_print_compositor_set_footer_format (compositor,
																								 TRUE,
																								 "%T",
																								 basename,
																								 "Page %N/%Q");
	
	gtk_source_print_compositor_set_print_header (compositor,
																								anjuta_preferences_get_bool (sv->priv->prefs,
																																						PRINT_HEADER));
	gtk_source_print_compositor_set_print_footer (compositor,
																								anjuta_preferences_get_bool (sv->priv->prefs,
																																						PRINT_FOOTER));
	
	
	gtk_source_print_compositor_set_highlight_syntax (compositor,
																										anjuta_preferences_get_bool (sv->priv->prefs,
																																								PRINT_HIGHLIGHT)),
	
	operation = gtk_print_operation_new ();
	
	gtk_print_operation_set_job_name (operation, basename);
	
	gtk_print_operation_set_show_progress (operation, TRUE);
	
	printing->compositor = compositor;
	printing->sv = sv;
	printing->status = anjuta_shell_get_status (sv->priv->plugin->shell, NULL);
	
	g_signal_connect (G_OBJECT (operation), "paginate", 
			  G_CALLBACK (paginate), printing);
	g_signal_connect (G_OBJECT (operation), "draw-page", 
										G_CALLBACK (draw_page), printing);
	g_signal_connect (G_OBJECT (operation), "end-print", 
										G_CALLBACK (end_print), printing);
	
	anjuta_status_progress_reset (printing->status);
	anjuta_status_progress_add_ticks (printing->status, 100);
	g_free (basename);
	
	return operation;
}

void 
sourceview_print(Sourceview* sv)
{
	GtkPrintOperation* operation = print_setup (sv);
	gtk_print_operation_run (operation, 
													 GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
													 NULL, NULL);
	g_object_unref (operation);	
}


void
sourceview_print_preview(Sourceview* sv)
{
	GtkPrintOperation* operation = print_setup (sv);
	gtk_print_operation_run (operation, 
													 GTK_PRINT_OPERATION_ACTION_PREVIEW,
													 NULL, NULL);
	g_object_unref (operation);	
}

