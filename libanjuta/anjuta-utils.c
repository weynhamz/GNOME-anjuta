/*
    anjuta-utils.c
    Copyright (C) 2003 Naba Kumar  <naba@gnome.org>

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
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <libgnome/gnome-i18n.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/properties.h>

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

gboolean
anjuta_util_dialog_boolean_question (GtkWindow *parent, gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	gint ret;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	dialog = gtk_message_dialog_new (parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_YES_NO, message);

	ret = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (message);
	
	return (ret == GTK_RESPONSE_YES);
}

gboolean
anjuta_util_prog_is_installed (gchar * prog, gboolean show)
{
	gchar* prog_path = g_find_program_in_path (prog);
	if (prog_path)
	{
		g_free (prog_path);
		return TRUE;
	}
	if (show)
	{
		anjuta_util_dialog_error (NULL, _("The \"%s\" utility is not installed.\n"
					  "Please install it."), prog);
	}
	return FALSE;
}

gchar *
anjuta_util_get_a_tmp_file (void)
{
	static gint count = 0;
	gchar *filename;
	const gchar *tmpdir;

	tmpdir = g_get_tmp_dir ();
	filename =
		g_strdup_printf ("%s/anjuta_%d.%d", tmpdir, count++, getpid ());
	return filename;
}

/* GList of strings operations */
GList *
anjuta_util_glist_from_string (const gchar *string)
{
	gchar *str, *temp, buff[256];
	GList *list;
	gchar *word_start, *word_end;
	gboolean the_end;

	list = NULL;
	the_end = FALSE;
	temp = g_strdup (string);
	str = temp;
	if (!str)
		return NULL;

	while (1)
	{
		gint i;
		gchar *ptr;

		/* Remove leading spaces */
		while (isspace (*str) && *str != '\0')
			str++;
		if (*str == '\0')
			break;

		/* Find start and end of word */
		word_start = str;
		while (!isspace (*str) && *str != '\0')
			str++;
		word_end = str;

		/* Copy the word into the buffer */
		for (ptr = word_start, i = 0; ptr < word_end; ptr++, i++)
			buff[i] = *ptr;
		buff[i] = '\0';
		if (strlen (buff))
			list = g_list_append (list, g_strdup (buff));
		if (*str == '\0')
			break;
	}
	if (temp)
		g_free (temp);
	return list;
}

/* Get the list of strings as GList from a property value.
   Strings are splitted from white spaces */
GList *
anjuta_util_glist_from_data (guint props, const gchar *id)
{
	gchar *str;
	GList *list;

	str = prop_get (props, id);
	list = anjuta_util_glist_from_string (str);
	g_free(str);
	return list;
}

/* Prefix the strings */
void
anjuta_util_glist_strings_prefix (GList * list, const gchar *prefix)
{
	GList *node;
	node = list;
	
	g_return_if_fail (prefix != NULL);
	while (node)
	{
		gchar* tmp;
		tmp = node->data;
		node->data = g_strconcat (prefix, tmp, NULL);
		if (tmp) g_free (tmp);
		node = g_list_next (node);
	}
}

/* Suffix the strings */
void
anjuta_util_glist_strings_sufix (GList * list, const gchar *sufix)
{
	GList *node;
	node = list;
	
	g_return_if_fail (sufix != NULL);
	while (node)
	{
		gchar* tmp;
		tmp = node->data;
		node->data = g_strconcat (tmp, sufix, NULL);
		if (tmp) g_free (tmp);
		node = g_list_next (node);
	}
}

/* Duplicate list of strings */
GList*
anjuta_util_glist_strings_dup (GList * list)
{
	GList *node;
	GList *new_list;
	
	new_list = NULL;
	node = list;
	while (node)
	{
		if (node->data)
			new_list = g_list_append (new_list, g_strdup(node->data));
		else
			new_list = g_list_append (new_list, NULL);
		node = g_list_next (node);
	}
	return new_list;
}

static gchar*
get_real_path(const gchar *file_name)
{
	if (file_name)
	{
		gchar path[PATH_MAX+1];
		memset(path, '\0', PATH_MAX+1);
		realpath(file_name, path);
		return g_strdup(path);
	}
	else
		return NULL;
}

/* Dedup a list of paths - duplicates are removed from the tail.
** Useful for deduping Recent Files and Recent Projects */
GList*
anjuta_util_glist_path_dedup(GList *list)
{
	GList *nlist = NULL, *tmp, *tmp1;
	gchar *path;
	struct stat s;
	for (tmp = list; tmp; tmp = g_list_next(tmp))
	{
		path = get_real_path((const gchar *) tmp->data);
		if (0 != stat(path, &s))
		{
			g_free(path);
		}
		else
		{
			for (tmp1 = nlist; tmp1; tmp1 = g_list_next(tmp1))
			{
				if (0 == strcmp((const char *) tmp1->data, path))
				{
					g_free(path);
					path = NULL;
					break;
				}
			}
			if (path)
			nlist = g_list_prepend(nlist, path);
		}
	}
	anjuta_util_glist_strings_free(list);
	nlist = g_list_reverse(nlist);
	return nlist;
}

static gint
sort_node (gchar* a, gchar *b)
{
	if ( !a && !b) return 0;
	else if (!a) return -1;
	else if (!b) return 1;
	return strcmp (a, b);
}

/* Sort the list alphabatically */
GList*
anjuta_util_glist_strings_sort (GList * list)
{
	return g_list_sort(list, (GCompareFunc)sort_node);
}

/* Free the strings and GList */
void
anjuta_util_glist_strings_free (GList * list)
{
	GList *node;
	node = list;
	while (node)
	{
		if (node->data)
			g_free (node->data);
		node = g_list_next (node);
	}
	g_list_free (list);
}

int
anjuta_util_type_from_string (AnjutaUtilStringMap *map, const char *str)
{
	int i = 0;

	while (-1 != map[i].type)
	{
		if (0 == strcmp(map[i].name, str))
			return map[i].type;
		++ i;
	}
	return -1;
}

const char*
anjuta_util_string_from_type (AnjutaUtilStringMap *map, int type)
{
	int i = 0;
	while (-1 != map[i].type)
	{
		if (map[i].type == type)
			return map[i].name;
		++ i;
	}
	return "";
}

GList*
anjuta_util_glist_from_map (AnjutaUtilStringMap *map)
{
	GList *out_list = NULL;
	int i = 0;
	while (-1 != map[i].type)
	{
		out_list = g_list_append(out_list, map[i].name);
		++ i;
	}
	return out_list;
}
