/*
    utilities.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <libgnome/gnome-i18n.h>
#include <libanjuta/anjuta-utils.h>

#define FILE_BUFFER_SIZE 1024

gboolean
anjuta_util_copy_file (gchar * src, gchar * dest, gboolean show_error)
{
	FILE *input_fp, *output_fp;
	gchar buffer[FILE_BUFFER_SIZE];
	gint bytes_read, bytes_written;
	gboolean error;
	
	error = TRUE;
	
	input_fp = fopen (src, "rb");
	if (input_fp == NULL)
	{
		if( show_error)
			anjuta_util_dialog_error_system (NULL, errno,
											 _("Unable to read file: %s."),
											 src);
		return FALSE;
	}
	
	output_fp = fopen (dest, "wb");
	if (output_fp == NULL)
	{
		if( show_error)
			anjuta_util_dialog_error_system (NULL, errno,
											 _("Unable to create file: %s."),
											 dest);
		fclose (input_fp);
		return TRUE;
	}
	
	for (;;)
	{
		bytes_read = fread (buffer, 1, FILE_BUFFER_SIZE, input_fp);
		if (bytes_read != FILE_BUFFER_SIZE && ferror (input_fp))
		{
			error = FALSE;
			break;
		}
		
		if (bytes_read)
		{
			bytes_written = fwrite (buffer, 1, bytes_read, output_fp);
			if (bytes_read != bytes_written)
			{
				error = FALSE;
				break;
			}
		}
		
		if (bytes_read != FILE_BUFFER_SIZE && feof (input_fp))
		{
			break;
		}
	}
	
	fclose (input_fp);
	fclose (output_fp);
	
	if( show_error && (error == FALSE))
		anjuta_util_dialog_error_system (NULL, errno,
										 _("Unable to complete file copy"));
	return error;
}

static gint
int_from_hex_digit (const gchar ch)
{
	if (isdigit (ch))
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return 0;
}

void
anjuta_util_color_from_string (const gchar * val, guint8 * r, guint8 * g, guint8 * b)
{
	*r = int_from_hex_digit (val[1]) * 16 + int_from_hex_digit (val[2]);
	*g = int_from_hex_digit (val[3]) * 16 + int_from_hex_digit (val[4]);
	*b = int_from_hex_digit (val[5]) * 16 + int_from_hex_digit (val[6]);
}

gchar *
anjuta_util_string_from_color (guint8 r, guint8 g, guint8 b)
{
	gchar str[10];
	guint32 num;

	num = r;
	num <<= 8;
	num += g;
	num <<= 8;
	num += b;

	sprintf (str, "#%06X", num);
	return g_strdup (str);
}

GtkWidget* 
anjuta_util_button_new_with_stock_image (const gchar* text,
										 const gchar* stock_id)
{
	GtkWidget *button;
	GtkStockItem item;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;
	GtkWidget *align;

	button = gtk_button_new ();

 	if (GTK_BIN (button)->child)
    		gtk_container_remove (GTK_CONTAINER (button),
				      GTK_BIN (button)->child);

  	if (gtk_stock_lookup (stock_id, &item))
    	{
      		label = gtk_label_new_with_mnemonic (text);

		gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
      
		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
      		hbox = gtk_hbox_new (FALSE, 2);

      		align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      
      		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      		gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      
      		gtk_container_add (GTK_CONTAINER (button), align);
      		gtk_container_add (GTK_CONTAINER (align), hbox);
      		gtk_widget_show_all (align);

      		return button;
    	}

      	label = gtk_label_new_with_mnemonic (text);
      	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
  
  	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  	gtk_widget_show (label);
  	gtk_container_add (GTK_CONTAINER (button), label);

	return button;
}

GtkWidget*
anjuta_util_dialog_add_button (GtkDialog *dialog, const gchar* text,
							   const gchar* stock_id, gint response_id)
{
	GtkWidget *button;
	
	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = anjuta_util_button_new_with_stock_image (text, stock_id);
	g_return_val_if_fail (button != NULL, NULL);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);	

	return button;
}

void
anjuta_util_dialog_error (GtkWindow *parent, gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR,
									 GTK_BUTTONS_CLOSE, message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_warning (GtkWindow *parent, gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_WARNING,
									 GTK_BUTTONS_CLOSE, message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_info (GtkWindow *parent, gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_CLOSE, message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_error_system (GtkWindow* parent, gint errnum, gchar * mesg, ... )
{
	gchar* message;
	gchar* tot_mesg;
	va_list args;
	GtkWidget *dialog;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);

	if (0 != errnum) {
		tot_mesg = g_strconcat (message, _("\nSystem: "), g_strerror(errnum), NULL);
		g_free (message);
	} else
		tot_mesg = message;

	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR,
									 GTK_BUTTONS_CLOSE, tot_mesg);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (tot_mesg);
}
