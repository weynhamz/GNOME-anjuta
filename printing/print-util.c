/*
** Gnome-Print support
** Author: Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
** Original Author: Chema Celorio <chema@celorio.com>, Gnumeric Team
** Licence: GPL
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

static int g_unichar_to_utf8(gint c, gchar * outbuf)
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

#ifndef _PROPER_I18N
static int anjuta_print_show_iso8859_1(GnomePrintContext * pc, char const *text)
{
	gchar *p, *utf, *udyn, ubuf[4096];
	gint len, ret, i;

	g_return_val_if_fail (pc && text, -1);

	if (!*text)
		return 0;

	len = strlen (text);
	if (len * 2 > sizeof (ubuf))
	{
		udyn = g_new (gchar, len * 2);
		utf = udyn;
	}
	else
	{
		udyn = NULL;
		utf = ubuf;
	}
	p = utf;

	for (i = 0; i < len; i++)
	{
		p += g_unichar_to_utf8 (((guchar *) text)[i], p);
	}

	ret = gnome_print_show_sized (pc, utf, p - utf);

	if (udyn)
		g_free (udyn);

	return ret;
}
#endif

int anjuta_print_show(GnomePrintContext * pc, char const *text)
{
#ifdef _PROPER_I18N
	wchar_t *wcs, wcbuf[4096];
	char *utf8, utf8buf[4096];

	int conv_status;
	int n = strlen (text);
	int retval;

	g_return_val_if_fail (pc && text, -1);

	if (n > (sizeof (wcbuf) / sizeof (wcbuf[0])))
		wcs = g_new (wchar_t, n);
	else
		wcs = wcbuf;

	conv_status = mbstowcs (wcs, text, n);

	if (conv_status == (size_t) (-1))
	{
		if (wcs != wcbuf)
			g_free (wcs);
		return 0;
	};
	if (conv_status * 6 > sizeof (utf8buf))
		utf8 = g_new (gchar, conv_status * 6);
	else
		utf8 = utf8buf;
	{
		int i;
		char *p = utf8;
		for (i = 0; i < conv_status; ++i)
			p += g_unichar_to_utf8 ((gint) wcs[i], p);
		if (wcs != wcbuf)
			g_free (wcs);
		retval = gnome_print_show_sized (pc, utf8, p - utf8);
	}

	if (utf8 != utf8buf)
		g_free (utf8);
	return retval;
#else
	return anjuta_print_show_iso8859_1 (pc, text);
#endif
};

static gboolean anjuta_print_verify_one_font(const gchar *font_name)
{
	GnomeFont *test_font;
	if (NULL == (test_font = gnome_font_new(font_name, 10)))
	{
		anjuta_error(_("Unable to find the font \"%s\".\n"
		  "Anjuta is unable to print without this font installed.")
		  , font_name);
		return FALSE;
	}
	gnome_font_unref(test_font);
	return TRUE;
}

/* FIXME: Do not hardcode font names */
gboolean anjuta_print_verify_fonts(void)
{
	if (!anjuta_print_verify_one_font(AN_PRINT_FONT_BODY))
		return FALSE;
	if (!anjuta_print_verify_one_font(AN_PRINT_FONT_HEADER))
		return FALSE;
	return TRUE;
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

	if (pji->page_last - pji->page_first < 31)
		return;
	progress_bar = gtk_progress_bar_new();
	dialog = GNOME_DIALOG (gnome_dialog_new (_("Printing .."),
	  GNOME_STOCK_BUTTON_CANCEL, NULL));
	gnome_dialog_button_connect (dialog, 0
	  ,GTK_SIGNAL_FUNC(anjuta_print_progress_clicked), pji);
	pji->progress_dialog = dialog;
	pji->progress_bar = progress_bar;
	label = gtk_label_new (_("Printing ..."));
	gtk_box_pack_start (GTK_BOX (dialog->vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), progress_bar, FALSE, FALSE, 0);
	gtk_widget_show_all (GTK_WIDGET (dialog));
}

void anjuta_print_progress_tick(PrintJobInfo * pji, gint page)
{
	gfloat percentage;

	while (gtk_events_pending())
		gtk_main_iteration ();
	if (pji->progress_dialog == NULL)
		return;
	percentage = (gfloat) (page - pji->page_first) /
	  (gfloat) (pji->page_last - pji->page_first);
	if (percentage > 0.0 && percentage < 1.0)
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
