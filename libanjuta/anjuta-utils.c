/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-utils.c
 * Copyright (C) Naba Kumar  <naba@gnome.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:anjuta-utils
 * @title: Utilities
 * @short_description: Utility functions
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-utils.h
 * 
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
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#ifndef G_OS_WIN32
#include <pwd.h>
#endif

#include <glib/gi18n.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#define FILE_BUFFER_SIZE 1024

static void
anjuta_util_from_file_to_file (GInputStream *istream,
							   GOutputStream *ostream)
{
	gsize bytes = 1;
	GError *error = NULL;
	gchar buffer[FILE_BUFFER_SIZE];
	
	while (bytes != 0 && bytes != -1)
	{
		bytes = g_input_stream_read (istream, buffer,
					     sizeof (buffer),
					     NULL, &error);
		if (error)
			break;

		g_output_stream_write (ostream, buffer,
				       bytes,
				       NULL, &error);
		if (error)
			break;
	}
	
	if (error)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	
	if (!g_output_stream_close (ostream, NULL, &error))
	{
		g_warning ("%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	if (!g_input_stream_close (istream, NULL, &error))
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}
}

/**
 * anjuta_util_copy_file:
 * @src: the file where copy
 * @dest: the path to copy the @src
 * @show_error: TRUE to show a dialog error
 *
 * Copies @src to @dest and shows a dialog error in case is needed.
 *
 * Returns: TRUE if there was an error copying the file.
 */
gboolean
anjuta_util_copy_file (const gchar * src, const gchar * dest, gboolean show_error)
{
	GFile *src_file, *dest_file;
	GFileInputStream *istream;
	GFileOutputStream *ostream;
	GError *error = NULL;
	gboolean toret = FALSE;
	
	src_file = g_file_new_for_path (src);
	dest_file = g_file_new_for_path (dest);
	
	istream = g_file_read (src_file, NULL, &error);
	if (error)
		goto free;
		
	ostream = g_file_create (dest_file, G_FILE_CREATE_NONE,
							 NULL, &error);
	if (error)
		goto free;
	
	anjuta_util_from_file_to_file (G_INPUT_STREAM (istream), G_OUTPUT_STREAM (ostream));
	
free: if (error)
	{
		if (show_error)
			anjuta_util_dialog_error_system (NULL, error->code,
											 error->message);
	
		g_warning ("%s", error->message);
		
		toret = TRUE;
	}
	
	g_object_unref (src_file);
	g_object_unref (dest_file);
	
	return toret;
}

void
anjuta_util_color_from_string (const gchar * val, guint16 * r, guint16 * g, guint16 * b)
{
	GdkColor color;
	if (gdk_color_parse(val, &color))
	{
		*r = color.red;
		*g = color.green;
		*b =color.blue;
	}
}

gchar *
anjuta_util_string_from_color (guint16 r, guint16 g, guint16 b)
{
	return g_strdup_printf("#%02x%02x%02x", r >> 8, g >> 8, b >> 8);
}

/* Get a GdkColor from preferences. Free the color with gdk_color_free() */
GdkColor*
anjuta_util_convert_color(AnjutaPreferences* prefs, const gchar* pref_name)
{
	GdkColor* color = g_new0(GdkColor, 1);
	gchar* color_string = anjuta_preferences_get(prefs, pref_name);
	gdk_color_parse(color_string, color);
	return color;
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
anjuta_util_dialog_error (GtkWindow *parent, const gchar *mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR,
									 GTK_BUTTONS_CLOSE, "%s", message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_warning (GtkWindow *parent, const gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_WARNING,
									 GTK_BUTTONS_CLOSE, "%s", message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_info (GtkWindow *parent, const gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_CLOSE, "%s", message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_error_system (GtkWindow* parent, gint errnum,
								 const gchar * mesg, ... )
{
	gchar* message;
	gchar* tot_mesg;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);

	if (0 != errnum) {
		/* Avoid space in translated string */
		tot_mesg = g_strconcat (message, "\n", _("System:"), " ",
								g_strerror(errnum), NULL);
		g_free (message);
	} else
		tot_mesg = message;

	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR,
									 GTK_BUTTONS_CLOSE, "%s", tot_mesg);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (tot_mesg);
}

gboolean
anjuta_util_dialog_boolean_question (GtkWindow *parent, const gchar *mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	gint ret;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_YES_NO, "%s", message);

	ret = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (message);
	
	return (ret == GTK_RESPONSE_YES);
}

gboolean
anjuta_util_dialog_input (GtkWindow *parent, const gchar *prompt,
						  const gchar *default_value, gchar **return_value)
{
	GtkWidget *dialog, *label, *frame, *entry, *dialog_vbox, *vbox;
	gint res;
	gchar *markup;
	GtkWindow *real_parent;

	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}

	dialog = gtk_dialog_new_with_buttons (prompt, real_parent,
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OK, GTK_RESPONSE_OK,
										  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	dialog_vbox = GTK_DIALOG (dialog)->vbox;
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, -1);
	gtk_widget_show (dialog_vbox);
	
	markup = g_strconcat ("<b>", prompt, "</b>", NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	gtk_widget_show (label);
	g_free (markup);
	
	frame = gtk_frame_new (NULL);
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	if (default_value)
		gtk_entry_set_text (GTK_ENTRY (entry), default_value);
	
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	
	if (gtk_entry_get_text (GTK_ENTRY (entry)) &&
		strlen (gtk_entry_get_text (GTK_ENTRY (entry))) > 0)
	{
		*return_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	}
	else
	{
		*return_value = NULL;
	}
	gtk_widget_destroy (dialog);	
	return (res == GTK_RESPONSE_OK);
}

gboolean
anjuta_util_prog_is_installed (const gchar * prog, gboolean show)
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

/* Join list of strings using the given delimiter */
gchar*
anjuta_util_glist_strings_join (GList * list, gchar *delimiter)
{
	GString *joined;
	gboolean first = TRUE;
	GList *node;
	
	joined = g_string_new (NULL);
	node = list;
	while (node)
	{
		if (node->data)
		{
			if (!first)
				g_string_append (joined, delimiter);
			else
				first = FALSE;
			g_string_append (joined, node->data);
		}
		node = g_list_next (node);
	}
	if (joined->len > 0)
		return g_string_free (joined, FALSE);
	else
		g_string_free (joined, TRUE);
	return NULL;
}

gchar*
anjuta_util_get_real_path (const gchar *path)
{
	if (path != NULL)
	{
		gchar *result;
#ifdef PATH_MAX
		gchar buf[PATH_MAX+1];

		result = realpath (path, buf);
		if (result != NULL)
		{
			*(buf + PATH_MAX) = '\0'; /* ensure a terminator */
			return g_strdup (buf);
		}
#else
		char *buf;
		/* the string returned by realpath should be cleaned with
		   free(), not g_free() */
		buf = realpath (path, NULL);
		if (buf != NULL)
		{
			result = g_strdup (buf);
			free (buf);
			return result;
		}
#endif
	}
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
		path = anjuta_util_get_real_path ((const gchar *) tmp->data);
		if (path)
		{
			if (stat (path, &s) != 0)
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


GList *
anjuta_util_update_string_list (GList *p_list, const gchar *p_str, gint length)
{
	gint i;
	gchar *str;
	if (!p_str)
		return p_list;
	for (i = 0; i < g_list_length (p_list); i++)
	{
		str = (gchar *) g_list_nth_data (p_list, i);
		if (!str)
			continue;
		if (strcmp (p_str, str) == 0)
		{
			p_list = g_list_remove (p_list, str);
			p_list = g_list_prepend (p_list, str);
			return p_list;
		}
	}
	p_list = g_list_prepend (p_list, g_strdup (p_str));
	while (g_list_length (p_list) > length)
	{
		str = g_list_nth_data (p_list, g_list_length (p_list) - 1);
		p_list = g_list_remove (p_list, str);
		g_free (str);
	}
	return p_list;
}

gboolean
anjuta_util_create_dir (const gchar* path)
{
	GFile *dir = g_file_new_for_path (path);
	GError *err = NULL;

	if (g_file_query_exists (dir, NULL))
	{
		GFileInfo *info = g_file_query_info (dir, 
				G_FILE_ATTRIBUTE_STANDARD_TYPE, 
				G_FILE_QUERY_INFO_NONE, 
				NULL, NULL);
		if (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY)
		{
			g_message ("Warning: %s is a file. \n \
					It is trying to be treated as a directory.",g_file_get_path (dir));
			g_object_unref (dir);
			return FALSE;
		}
		g_object_unref (info);
	}
	else if (!g_file_make_directory (dir, NULL, &err))
	{
		g_warning ("Error directory:\n %s", err->message);
		g_object_unref (dir);
		return FALSE;
	}
	g_object_unref (dir);

	return TRUE;
}

/**
 * anjuta_util_user_shell:
 * 
 * Retrieves the user's preferred shell.
 * 
 * Returns: A newly allocated string that is the path to the shell.
 */
/* copied from deprecated gnome_util_user_shell in libgnome */
char *
anjuta_util_user_shell (void)
{
#ifndef G_OS_WIN32
	struct passwd *pw;
	int i;
	const char *shell;
	const char shells [][14] = {
		/* Note that on some systems shells can also
		 * be installed in /usr/bin */
		"/bin/bash", "/usr/bin/bash",
		"/bin/zsh", "/usr/bin/zsh",
		"/bin/tcsh", "/usr/bin/tcsh",
		"/bin/ksh", "/usr/bin/ksh",
		"/bin/csh", "/bin/sh"
											};

	if (geteuid () == getuid () &&
	    getegid () == getgid ()) {
		/* only in non-setuid */
		if ((shell = g_getenv ("SHELL"))){
			if (access (shell, X_OK) == 0) {
				return g_strdup (shell);
			}
		}
	}
	pw = getpwuid(getuid());
	if (pw && pw->pw_shell) {
		if (access (pw->pw_shell, X_OK) == 0) {
			return g_strdup (pw->pw_shell);
		}
	}

	for (i = 0; i != G_N_ELEMENTS (shells); i++) {
		if (access (shells [i], X_OK) == 0) {
			return g_strdup (shells[i]);
		}
	}

	/* If /bin/sh doesn't exist, your system is truly broken.  */
	abort ();

	/* Placate compiler.  */
											return NULL;
#else
	/* g_find_program_in_path() always looks also in the Windows
 	 * and System32 directories, so it should always find either cmd.exe
 	 * or command.com.
 	 */
	char *retval = g_find_program_in_path ("cmd.exe");

	if (retval == NULL)
		retval = g_find_program_in_path ("command.com");

	g_assert (retval != NULL);

	return retval;
#endif
}

pid_t
anjuta_util_execute_shell (const gchar *dir, const gchar *command)
{
	pid_t pid;
	gchar *shell;
	gint err;
	
	g_return_val_if_fail (command != NULL, -1);
	
	shell = anjuta_util_user_shell ();
	pid = fork();
	if (pid == 0)
	{
		if(dir)
		{
			anjuta_util_create_dir (dir);
			err = chdir (dir);
		}
		execlp (shell, shell, "-c", command, NULL);
		g_warning (_("Cannot execute command: %s (using shell %s)\n"), command, shell);
		_exit(1);
	}
	if (pid < 0)
		g_warning (_("Cannot execute command: %s (using shell %s)\n"), command, shell);
	g_free (shell);
	// Anjuta will take care of child exit automatically.
	return pid;
}

gchar *
anjuta_util_convert_to_utf8 (const gchar *str)
{
	GError *error = NULL;
	gchar *utf8_msg_string = NULL;
	
	g_return_val_if_fail (str != NULL, NULL);
	g_return_val_if_fail (strlen (str) > 0, NULL);
	
	if (g_utf8_validate(str, -1, NULL))
	{
		utf8_msg_string = g_strdup (str);
	}
	else
	{
		gsize rbytes, wbytes;
		utf8_msg_string = g_locale_to_utf8 (str, -1, &rbytes, &wbytes, &error);
		if (error != NULL) {
			g_warning ("g_locale_to_utf8 failed: %s\n", error->message);
			g_error_free (error);
			/* g_free (utf8_msg_string);
			return NULL; */
		}
	}
	return utf8_msg_string;
}

GList*
anjuta_util_parse_args_from_string (const gchar* string)
{
	gboolean escaped;
	gchar    quote = 0;
	gboolean is_quote = FALSE;
	gchar* buffer = g_new0(gchar, strlen(string) + 1);
	const gchar *s;
	gint     idx;
	GList* args = NULL;
	
	idx = 0;
	escaped = FALSE;
	s = string;
	
	while (*s) {
		if (!isspace(*s))
			break;
		s++;
	}

	while (*s) {
		if (escaped) {
			/* The current char was escaped */
			buffer[idx++] = *s;
			escaped = FALSE;
		} else if (*s == '\\') {
			/* Current char is an escape */
			escaped = TRUE;
		} else if (is_quote && *s == quote) {
			/* Current char ends a quotation */
			is_quote = FALSE;
			if (!isspace(*(s+1)) && (*(s+1) != '\0')) {
				/* If there is no space after the quotation or it is not
				   the end of the string */
				g_warning ("Parse error while parsing program arguments");
			}
		} else if ((*s == '\"' || *s == '\'')) {
			if (!is_quote) {
				/* Current char starts a quotation */
				quote = *s;
				is_quote = TRUE;
			} else {
				/* Just a quote char inside quote */
				buffer[idx++] = *s;
			}
		} else if (is_quote){
			/* Any other char inside quote */
			buffer[idx++] = *s;
		} else if (isspace(*s)) {
			/* Any white space outside quote */
			if (idx > 0) {
				buffer[idx++] = '\0';
				args = g_list_append (args, g_strdup (buffer));
				idx = 0;
			}
		} else {
			buffer[idx++] = *s;
		}
		s++;
	}
	if (idx > 0) {
		/* There are chars in the buffer. Flush as the last arg */
		buffer[idx++] = '\0';
		args = g_list_append (args, g_strdup (buffer));
		idx = 0;
	}
	if (is_quote) {
		g_warning ("Unclosed quotation encountered at the end of parsing");
	}
	g_free (buffer);
	return args;
}

gchar*
anjuta_util_escape_quotes(const gchar* str)
{
	gchar *buffer;
	gint idx, max_size;
	const gchar *s = str;
	
	g_return_val_if_fail(str, NULL);
	idx = 0;
	
	/* We are assuming there will be less than 2048 chars to escape */
	max_size = strlen(str) + 2048;
	buffer = g_new (gchar, max_size);
	max_size -= 2;
	
	while(*s) {
		if (idx > max_size)
			break;
		if (*s == '\"' || *s == '\'' || *s == '\\')
			buffer[idx++] = '\\';
		buffer[idx++] = *s;
		s++;
	}
	buffer[idx] = '\0';
	return buffer;
}

/* Diff the text contained in uri with text. Return true if files
differ, FALSE if they are identical.*/

gboolean anjuta_util_diff(const gchar* uri, const gchar* text)
{
	GFile *file;
	GFileInfo *file_info;
	guint64 size;
	gchar* file_text = NULL;
	gsize bytes_read;

	file = g_file_new_for_uri (uri);
	file_info = g_file_query_info (file,
			G_FILE_ATTRIBUTE_STANDARD_SIZE,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			NULL);

	if (file_info == NULL)
	{
		g_object_unref (file);
		return TRUE;
	}

	size = g_file_info_get_attribute_uint64(file_info,
			G_FILE_ATTRIBUTE_STANDARD_SIZE);
	g_object_unref (file_info);

	if (size == 0 && text == NULL)
	{
		g_object_unref (file);
		return FALSE;
	}
	else if (size == 0 || text == NULL)
	{
		g_object_unref (file);
		return TRUE;
	}

	if (!g_file_load_contents(file, 
				NULL,
				&file_text, 
				&bytes_read,
				NULL,
				NULL))
	{
		g_object_unref (file);
		return TRUE;
	}
	g_object_unref (file);

	if (bytes_read != size)
	{
		g_free (file_text);
		return TRUE;
	}

	/* according to g_file_load_contents's documentation
	 * file_text is guaranteed to end with \0.
	 */
	if (strcmp (file_text, text) == 0)
	{
		g_free (file_text);
		return FALSE;
	}

	g_free (file_text);
	return TRUE;
}

/**
 * anjuta_util_is_project_file:
 * @filename: the file name
 *
 * Return TRUE if the file is an anjuta project file. It is implemented by
 * checking only the file extension. So it does not check the existence
 * of the file. But it is working on an URI if it does not containt a
 * fragment.
 *
 * Returns: TRUE if the file is a project file, else FALSE
 */
gboolean
anjuta_util_is_project_file (const gchar *filename)
{
	gsize len = strlen (filename);
	return (len > 8) && (strcmp (filename + len - 7, ".anjuta") == 0);
}

/**
 * anjuta_util_get_file_mine_type:
 * @file: the file
 *
 * Check if a file exists and return its mime type.
 *
 * Returns: NULL if the corresponding file doesn't exist or the mime type as a newly
 * allocated string that must be freed with g_free().
 */
gchar *
anjuta_util_get_file_mime_type (GFile *file)
{
	GFileInfo *info;
	gchar *mime_type = NULL;
	
	g_return_val_if_fail (file != NULL, NULL);
	
	/* Get file information, check that the file exist at the same time */
	info = g_file_query_info (file,
										  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
										  G_FILE_QUERY_INFO_NONE,
										  NULL,
										  NULL);
	
	if (info != NULL)
	{
		const gchar *extension;
		gchar *name;
		
		/* If Anjuta is not installed in system gnome prefix, the mime types 
		 * may not have been correctly registed. In that case, we use the
	 	 * following mime detection
	 	 */
		name = g_file_get_basename (file);
		extension = strrchr(name, '.');
		if (extension != NULL)
		{
			const static struct {gchar *extension; gchar *type;} anjuta_types[] = {
									{"anjuta", "application/x-anjuta"},
									{"prj", "application/x-anjuta-old"},
									{"ui", "text/xml"},
									{"glade", "application/x-glade"},
									{NULL, NULL}};
			gint i;
				
			for (i = 0; anjuta_types[i].extension != NULL; i++)
			{
				if (strcmp(extension + 1, anjuta_types[i].extension) == 0)
				{
					mime_type = g_strdup (anjuta_types[i].type);
					break;
				}
			}
		}
		g_free (name);
	
		/* Use mime database if it is not an Anjuta type */
		if (mime_type == NULL)
		{
			mime_type = g_content_type_get_mime_type (g_file_info_get_content_type(info));
		}
		
		g_object_unref (info);
	}

	return mime_type;
}

gchar *
anjuta_util_get_local_path_from_uri (const gchar *uri)
{
	GFile *file;
	gchar *local_path;

	file = g_file_new_for_uri (uri);
	local_path = g_file_get_path (file);
	g_object_unref (file);

	return local_path;
}

#ifndef HAVE_LIBUTIL
#include <grp.h>

static int ptym_open (char *pts_name);
static int ptys_open (int fdm, char * pts_name);

int
login_tty(int ttyfd)
{
  int fd;
  char *fdname;

#ifdef HAVE_SETSID
  setsid();
#endif
#ifdef HAVE_SETPGID
  setpgid(0, 0);
#endif
	
  /* First disconnect from the old controlling tty. */
#ifdef TIOCNOTTY
  fd = open("/dev/tty", O_RDWR|O_NOCTTY);
  if (fd >= 0)
    {
      ioctl(fd, TIOCNOTTY, NULL);
      close(fd);
    }
  else
    //syslog(LOG_WARNING, "NO CTTY");
#endif /* TIOCNOTTY */
  
  /* Verify that we are successfully disconnected from the controlling tty. */
  fd = open("/dev/tty", O_RDWR|O_NOCTTY);
  if (fd >= 0)
    {
      //syslog(LOG_WARNING, "Failed to disconnect from controlling tty.");
      close(fd);
    }
  
  /* Make it our controlling tty. */
#ifdef TIOCSCTTY
  ioctl(ttyfd, TIOCSCTTY, NULL);
#endif /* TIOCSCTTY */

  fdname = ttyname (ttyfd);
  fd = open(fdname, O_RDWR);
  if (fd < 0)
    ;//syslog(LOG_WARNING, "open %s: %s", fdname, strerror(errno));
  else
    close(fd);

  /* Verify that we now have a controlling tty. */
  fd = open("/dev/tty", O_WRONLY);
  if (fd < 0)
    {
      //syslog(LOG_WARNING, "open /dev/tty: %s", strerror(errno));
      return 1;
    }

  close(fd);
#if defined(HAVE_VHANGUP) && !defined(HAVE_REVOKE)
  {
    RETSIGTYPE (*sig)();
    sig = signal(SIGHUP, SIG_IGN);
    vhangup();
    signal(SIGHUP, sig);
  }
#endif
  fd = open(fdname, O_RDWR);
  if (fd == -1)
    {
      //syslog(LOG_ERR, "can't reopen ctty %s: %s", fdname, strerror(errno));
      return -1;
    }
	
  close(ttyfd);

  if (fd != 0)
    close(0);
  if (fd != 1)
    close(1);
  if (fd != 2)
    close(2);

  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
  if (fd > 2)
    close(fd);
  return 0;
}

int
openpty(int *amaster, int *aslave, char *name, struct termios *termp,
		struct winsize *winp)
{
	char line[20];
	*amaster = ptym_open(line);
	if (*amaster < 0)
		return -1;
	*aslave = ptys_open(*amaster, line);
	if (*aslave < 0) {
		close(*amaster);
		return -1;
	}
	if (name)
		strcpy(name, line);
#ifndef TCSAFLUSH
#define TCSAFLUSH TCSETAF
#endif
	if (termp)
		(void) tcsetattr(*aslave, TCSAFLUSH, termp);
#ifdef TIOCSWINSZ
	if (winp)
		(void) ioctl(*aslave, TIOCSWINSZ, (char *)winp);
#endif
	return 0;
}

static int
ptym_open(char * pts_name)
{
	int fdm;
#ifdef HAVE_PTSNAME
	char *ptr;

	strcpy(pts_name, "/dev/ptmx");
	fdm = open(pts_name, O_RDWR);
	if (fdm < 0)
		return -1;
	if (grantpt(fdm) < 0) { /* grant access to slave */
		close(fdm);
		return -2;
	}
	if (unlockpt(fdm) < 0) { /* clear slave's lock flag */
		close(fdm);
		return -3;
	}
	ptr = ptsname(fdm);
	if (ptr == NULL) { /* get slave's name */
		close (fdm);
		return -4;
	}
	strcpy(pts_name, ptr); /* return name of slave */
	return fdm;            /* return fd of master */
#else
	char *ptr1, *ptr2;

	strcpy(pts_name, "/dev/ptyXY");
	/* array index: 012345689 (for references in following code) */
	for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1 != 0; ptr1++) {
		pts_name[8] = *ptr1;
		for (ptr2 = "0123456789abcdef"; *ptr2 != 0; ptr2++) {
			pts_name[9] = *ptr2;
			/* try to open master */
			fdm = open(pts_name, O_RDWR);
			if (fdm < 0) {
				if (errno == ENOENT) /* different from EIO */
					return -1;  /* out of pty devices */
				else
					continue;  /* try next pty device */
			}
			pts_name[5] = 't'; /* chage "pty" to "tty" */
			return fdm;   /* got it, return fd of master */
		}
	}
	return -1; /* out of pty devices */
#endif
}

static int
ptys_open(int fdm, char * pts_name)
{
	int fds;
#ifdef HAVE_PTSNAME
	/* following should allocate controlling terminal */
	fds = open(pts_name, O_RDWR);
	if (fds < 0) {
		close(fdm);
		return -5;
	}
	if (ioctl(fds, I_PUSH, "ptem") < 0) {
		close(fdm);
		close(fds);
		return -6;
	}
	if (ioctl(fds, I_PUSH, "ldterm") < 0) {
		close(fdm);
		close(fds);
		return -7;
	}
	if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
		close(fdm);
		close(fds);
		return -8;
	}

	if (ioctl(fdm, I_PUSH, "pckt") < 0) {
		close(fdm);
		close(fds);
		return -8;
	}
	
	if (ioctl(fdm, I_SRDOPT, RMSGN|RPROTDAT) < 0) {
		close(fdm);
		close(fds);
		return -8;
	}

	return fds;
#else
	int gid;
	struct group *grptr;

	grptr = getgrnam("tty");
	if (grptr != NULL)
		gid = grptr->gr_gid;
	else
		gid = -1;  /* group tty is not in the group file */
	/* following two functions don't work unless we're root */
	chown(pts_name, getuid(), gid);
	chmod(pts_name, S_IRUSR | S_IWUSR | S_IWGRP);
	fds = open(pts_name, O_RDWR);
	if (fds < 0) {
		close(fdm);
		return -1;
	}
	return fds;
#endif
}

int
forkpty(int *amaster, char *name, struct termios *termp, struct winsize *winp)
{
  int master, slave, pid;

  if (openpty(&master, &slave, name, termp, winp) == -1)
    return (-1);
  switch (pid = fork()) {
  case -1:
    return (-1);
  case 0:
    /*
     * child
     */
    close(master);
    login_tty(slave);
    return (0);
  }
  /*
   * parent
   */
  *amaster = master;
  close(slave);
  return (pid);
}

int scandir(const char *dir, struct dirent ***namelist,
            int (*select)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
  DIR *d;
  struct dirent *entry;
  register int i=0;
  size_t entrysize;

  if ((d=opendir(dir)) == NULL)
     return(-1);

  *namelist=NULL;
  while ((entry=readdir(d)) != NULL)
  {
    if (select == NULL || (select != NULL && (*select)(entry)))
    {
      *namelist=(struct dirent **)realloc((void *)(*namelist),
                 (size_t)((i+1)*sizeof(struct dirent *)));
	if (*namelist == NULL) return(-1);
	entrysize=sizeof(struct dirent)-sizeof(entry->d_name)+strlen(entry->d_name)+1;
	(*namelist)[i]=(struct dirent *)malloc(entrysize);
	if ((*namelist)[i] == NULL) return(-1);
	memcpy((*namelist)[i], entry, entrysize);
	i++;
    }
  }
  if (closedir(d)) return(-1);
  if (i == 0) return(-1);
  if (compar != NULL)
    qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), compar);
    
  return(i);
}

#endif /* HAVE_LIBUTIL */

void
anjuta_util_help_display (GtkWidget   *parent,
						  const gchar *doc_id,
						  const gchar *file_name)
{

	GError *error = NULL;
	GdkScreen *screen;
	gchar *command;
	const gchar *lang;
	const gchar * const *langs;
	gchar *uri = NULL;
	gint i;
	
	g_return_if_fail (file_name != NULL);

	langs = g_get_language_names ();
	for (i = 0; langs[i]; i++)
	{
		lang = langs[i];
		if (strchr (lang, '.'))
			continue;

		uri = g_build_filename (DATADIR, "/gnome/help/", doc_id,
							    lang, file_name, NULL);

		if (g_file_test (uri, G_FILE_TEST_EXISTS)) {
			break;
		}
		g_free (uri);
		uri = NULL;
	}

	if (uri == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (parent), _("Unable to display help. Please make sure Anjuta "
								  "documentation package is install. It can be downloaded "
								  "from http://anjuta.org"));

		return;
	}
	
	command = g_strconcat ("gnome-help ghelp://", uri,  NULL);
	g_free (uri);

	screen = gtk_widget_get_screen (GTK_WIDGET (parent));
	gdk_spawn_command_line_on_screen (screen, command, &error);
	if (error != NULL)
	{
		g_warning ("Error executing help application: %s",
				   error->message);
		g_error_free (error);
		
		return;
	}
	g_free (command);
}

/* The following functions are taken from gedit */

/* Note that this function replace home dir with ~ */
gchar *
anjuta_util_uri_get_dirname (const gchar *uri)
{
	gchar *res;
	gchar *str;

	// CHECK: does it work with uri chaining? - Paolo
	str = g_path_get_dirname (uri);
	g_return_val_if_fail (str != NULL, ".");

	if ((strlen (str) == 1) && (*str == '.'))
	{
		g_free (str);
		
		return NULL;
	}

	res = anjuta_util_replace_home_dir_with_tilde (str);

	g_free (str);
	
	return res;
}

gchar*
anjuta_util_replace_home_dir_with_tilde (const gchar *uri)
{
	gchar *tmp;
	gchar *home;

	g_return_val_if_fail (uri != NULL, NULL);

	/* Note that g_get_home_dir returns a const string */
	tmp = (gchar *)g_get_home_dir ();

	if (tmp == NULL)
		return g_strdup (uri);

	home = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
	if (home == NULL)
		return g_strdup (uri);

	if (strcmp (uri, home) == 0)
	{
		g_free (home);
		
		return g_strdup ("~");
	}

	tmp = home;
	home = g_strdup_printf ("%s/", tmp);
	g_free (tmp);

	if (g_str_has_prefix (uri, home))
	{
		gchar *res;

		res = g_strdup_printf ("~/%s", uri + strlen (home));

		g_free (home);
		
		return res;		
	}

	g_free (home);

	return g_strdup (uri);
}

/**
 * anjuta_util_shell_expand:
 * @string: input string
 *
 * Expand environment variables $(var_name) and tilde (~) in the input string.
 *
 * Returns: a newly-allocated string that must be freed with g_free().
 */
gchar*
anjuta_util_shell_expand (const gchar *string)
{
	GString* expand;
	
	if (string == NULL) return NULL;
	
	expand = g_string_sized_new (strlen (string));
	
	for (; *string != '\0'; string++)
	{
		switch (*string)
		{
			case '$':
			{
				/* Variable expansion */
				const gchar *end;
				gint var_name_len;

				end = string + 1;
				while (isalnum (*end) || (*end == '_')) end++;
				var_name_len = end - string - 1;
				if (var_name_len > 0)
				{
					const gchar *value;
					
					g_string_append_len (expand, string + 1, var_name_len);
					value = g_getenv (expand->str + expand->len - var_name_len);
					g_string_truncate (expand, expand->len - var_name_len);
					g_string_append (expand, value);
					string = end - 1;
					continue;
				}
				break;
			}
			case '~':
			{
				/* User home directory expansion */
				if (isspace(string[1]) || (string[1] == G_DIR_SEPARATOR) || (string[1] == '\0'))
				{
					g_string_append (expand, g_get_home_dir());
					continue;
				}
				break;
			}
			default:
				break;
		}
		g_string_append_c (expand, *string);
	}
	
	return g_string_free (expand, FALSE);
}

gchar *
anjuta_util_str_middle_truncate (const gchar *string,
								  guint        truncate_length)
{
	GString     *truncated;
	guint        length;
	guint        n_chars;
	guint        num_left_chars;
	guint        right_offset;
	guint        delimiter_length;
	const gchar *delimiter = "\342\200\246";

	g_return_val_if_fail (string != NULL, NULL);

	length = strlen (string);

	g_return_val_if_fail (g_utf8_validate (string, length, NULL), NULL);

	/* It doesnt make sense to truncate strings to less than
	 * the size of the delimiter plus 2 characters (one on each
	 * side)
	 */
	delimiter_length = g_utf8_strlen (delimiter, -1);
	if (truncate_length < (delimiter_length + 2)) {
		return g_strdup (string);
	}

	n_chars = g_utf8_strlen (string, length);

	/* Make sure the string is not already small enough. */
	if (n_chars <= truncate_length) {
		return g_strdup (string);
	}

	/* Find the 'middle' where the truncation will occur. */
	num_left_chars = (truncate_length - delimiter_length) / 2;
	right_offset = n_chars - truncate_length + num_left_chars + delimiter_length;

	truncated = g_string_new_len (string,
				      g_utf8_offset_to_pointer (string, num_left_chars) - string);
	g_string_append (truncated, delimiter);
	g_string_append (truncated, g_utf8_offset_to_pointer (string, right_offset));
		
	return g_string_free (truncated, FALSE);
}

/* 
 * Functions to implement XDG Base Directory Specification
 * http://standards.freedesktop.org/basedir-spec/latest/index.html
 * Use this to save any config/cache/data files
 *
 */

static gchar*
anjuta_util_construct_pathv (const gchar* str, va_list str_list) 
{
	GPtrArray *str_arr;
	const gchar* tmp_str;
	gchar* path;

	str_arr = g_ptr_array_new();
	g_ptr_array_add (str_arr, (gpointer) str);

	/* Extract elements from va_list */
	if (str != NULL)
	{
		while ((tmp_str = va_arg (str_list, const gchar*)) != NULL) 
		{
			g_ptr_array_add (str_arr, (gpointer)tmp_str);
		}
		va_end (str_list);
	}
	
	/* Terminate the list */
	g_ptr_array_add (str_arr, NULL);

	path = g_build_filenamev ((gchar **)str_arr->pdata);
	g_ptr_array_free (str_arr, TRUE);
	
	return path;
}

static GFile*
anjuta_util_get_user_cache_filev (const gchar* path, va_list list)
{
	gchar *uri_str, *base_path, *dir;
	GFile *uri;
	const gchar anjuta_prefix[] = "anjuta";
	base_path = g_build_filename (g_get_user_cache_dir(), anjuta_prefix, path, NULL);

	uri_str = anjuta_util_construct_pathv (base_path, list);
	g_free (base_path);

	uri = g_file_new_for_path (uri_str);
	dir = g_path_get_dirname (uri_str);
	g_free(uri_str);
	if (!anjuta_util_create_dir (dir)) return NULL;

	return uri;
}

GFile*
anjuta_util_get_user_cache_file (const gchar* path, ...)
{
	va_list list;
	va_start (list, path);
	return anjuta_util_get_user_cache_filev (path, list);
}

static GFile*
anjuta_util_get_user_config_filev (const gchar* path, va_list list)
{
	gchar *uri_str, *base_path, *dir;
	GFile *uri;
	const gchar anjuta_prefix[] = "anjuta";
	base_path = g_build_filename (g_get_user_config_dir(), anjuta_prefix, path, NULL);

	uri_str = anjuta_util_construct_pathv (base_path, list);
	g_free (base_path);

	uri = g_file_new_for_path (uri_str);
	dir = g_path_get_dirname (uri_str);
	g_free(uri_str);
	if (!anjuta_util_create_dir (dir)) return NULL;

	return uri;
}

GFile*
anjuta_util_get_user_config_file (const gchar* path, ...)
{
	va_list list;
	va_start (list, path);
	return anjuta_util_get_user_config_filev (path, list);
}

static GFile*
anjuta_util_get_user_data_filev (const gchar* path, va_list list)
{
	gchar *uri_str, *base_path, *dir;
	GFile *uri;
	const gchar anjuta_prefix[] = "anjuta";
	base_path = g_build_filename (g_get_user_data_dir(), anjuta_prefix, path, NULL);

	uri_str = anjuta_util_construct_pathv (base_path, list);
	g_free (base_path);

	uri = g_file_new_for_path (uri_str);
	dir = g_path_get_dirname (uri_str);
	g_free(uri_str);
	if (!anjuta_util_create_dir (dir)) return NULL;

	return uri;
}

GFile*
anjuta_util_get_user_data_file (const gchar* path, ...)
{
	va_list list;
	va_start (list, path);
	return anjuta_util_get_user_data_filev (path, list);
}

gchar*
anjuta_util_get_user_cache_file_path (const gchar* path, ...)
{
	va_list list;
	GFile *file;
	gchar *file_path;
	va_start (list, path);
	file = anjuta_util_get_user_cache_filev (path, list);
	file_path = g_file_get_path (file);
	g_object_unref (file);

	return file_path;
}

gchar*
anjuta_util_get_user_config_file_path (const gchar* path, ...)
{
	va_list list;
	GFile *file;
	gchar *file_path; 
	va_start (list, path);
	file = anjuta_util_get_user_config_filev (path, list);
	file_path = g_file_get_path (file);
	g_object_unref (file);

	return file_path;
}

gchar*
anjuta_util_get_user_data_file_path (const gchar* path, ...)
{
	va_list list;
	GFile *file;
	gchar *file_path;;
	va_start (list, path);
	file = anjuta_util_get_user_data_filev (path, list);
	file_path = g_file_get_path (file);
	g_object_unref (file);

	return file_path;
}

GList *
anjuta_util_convert_gfile_list_to_path_list (GList *list)
{
	GList *path_list;
	GList *current_file;
	gchar *path;
	
	path_list = NULL;
	
	for (current_file = list; current_file != NULL; current_file = g_list_next (current_file))
	{
		path = g_file_get_path (current_file->data);
		
		/* Ignore files with invalid paths */
		if (path)
			path_list = g_list_append (path_list, path);
	}
	
	return path_list;
}

GList *
anjuta_util_convert_gfile_list_to_relative_path_list (GList *list, 
                                                      const gchar *parent)
{
	GFile *parent_file;
	GList *path_list;
	GList *current_file;
	gchar *path;
	
	parent_file = g_file_new_for_path (parent);
	path_list = NULL;
	
	if (parent_file)
	{		
		for (current_file = list; current_file != NULL; current_file = g_list_next (current_file))
		{
			path = g_file_get_relative_path (parent_file, current_file->data);
			
			/* Ignore files with invalid paths */
			if (path)
				path_list = g_list_append (path_list, path);
		}
		
		g_object_unref (parent_file);
	}
	
	return path_list;
}

