/*
 * print-util.c
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
#include <locale.h>
#include <libgnomeprint/gnome-print.h>

#include "anjuta.h"
#include "print.h"
#include "print-util.h"

#define _PROPER_I18N

int
anjuta_print_unichar_to_utf8(gint c, gchar * outbuf)
{
	size_t len = 0;
	int first;
	int i;

	if (c < 0x80)
	{
		first = 0;
		len = 1;
	}
	else if (c < 0x800)
	{
		first = 0xc0;
		len = 2;
	}
	else if (c < 0x10000)
	{
		first = 0xe0;
		len = 3;
	}
	else if (c < 0x200000)
	{
		first = 0xf0;
		len = 4;
	}
	else if (c < 0x4000000)
	{
		first = 0xf8;
		len = 5;
	}
	else
	{
		first = 0xfc;
		len = 6;
	}

	if (outbuf)
	{
		for (i = len - 1; i > 0; --i)
		{
			outbuf[i] = (c & 0x3f) | 0x80;
			c >>= 6;
		}
		outbuf[0] = c | first;
	}

	return len;
}

static void anjuta_print_progress_clicked(GtkWidget * button, gpointer data)
{
	PrintJobInfo *pji = (PrintJobInfo *) data;

	if (pji->progress_dialog == NULL)
		return;
	pji->canceled = TRUE;
}

void anjuta_print_progress_start(PrintJobInfo * pji)
{
	GnomeDialog *dialog;
	GtkWidget *label;
	GtkWidget *progress_bar;

	progress_bar = gtk_progress_bar_new();
	dialog = GNOME_DIALOG (gnome_dialog_new (_("Printing .."),
	  GNOME_STOCK_BUTTON_CANCEL, NULL));
	/* gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(app->widgets.window)); */
	gnome_dialog_button_connect (dialog, 0
	  ,GTK_SIGNAL_FUNC(anjuta_print_progress_clicked), pji);
	pji->progress_dialog = dialog;
	pji->progress_bar = progress_bar;
	label = gtk_label_new (_("Printing ..."));
	gtk_box_pack_start (GTK_BOX (dialog->vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), progress_bar, FALSE, FALSE, 0);
	gtk_widget_show_all (GTK_WIDGET (dialog));
}

void anjuta_print_progress_tick(PrintJobInfo * pji, guint idx)
{
	gfloat percentage;

	while (gtk_events_pending())
		gtk_main_iteration ();
	if (pji->progress_dialog == NULL)
		return;
	percentage = (float)idx/pji->buffer_size;
	if (percentage < 0.0) percentage = 0.0;
	if (percentage > 1.0) percentage = 1.0;
	gtk_progress_set_percentage(GTK_PROGRESS(pji->progress_bar), percentage);
}

void anjuta_print_progress_end(PrintJobInfo * pji)
{
	if (pji->progress_dialog == NULL)
		return;
	gnome_dialog_close(pji->progress_dialog);
	pji->progress_dialog = NULL;
	pji->progress_bar = NULL;
}
