/*
 *   resources.c
 *   Copyright (C) 2000  Kh. Naba Kumar Singh
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <gnome.h>

#include "utilities.h"
#include "resources.h"

GtkWidget *
anjuta_res_lookup_widget (GtkWidget * widget, const gchar * widget_name)
{
	GtkWidget *parent, *found_widget;

	for (;;)
	{
		if (GTK_IS_MENU (widget))
			parent =
				gtk_menu_get_attach_widget (GTK_MENU
							    (widget));
		else
			parent = widget->parent;
		if (parent == NULL)
			break;
		widget = parent;
	}

	found_widget = (GtkWidget *) gtk_object_get_data (GTK_OBJECT (widget),
							  widget_name);
	if (!found_widget)
		g_warning (_("Widget not found: %s"), widget_name);
	return found_widget;
}

GdkPixbuf *
anjuta_res_get_pixbuf (const gchar * filename)
{
	GdkPixbuf *image;
	gchar *pathname;
	GError *error = NULL;

	pathname = anjuta_res_get_pixmap_file (filename);
	if (!pathname)
	{
		g_warning (_("Could not find application image file: %s"), filename);
		return NULL;
	}

	image = gdk_pixbuf_new_from_file (pathname, &error);
	g_free (pathname);
	return image;
}

GtkWidget *
anjuta_res_get_image (const gchar * pixfile)
{
	GtkWidget *pixmap;
	gchar *pathname;
	
	if (!pixfile || !pixfile[0])
		return gtk_image_new ();

	pathname = anjuta_res_get_pixmap_file (pixfile);
	if (!pathname)
	{
		g_warning (_("Could not find application pixmap file: %s"),
			   pixfile);
		return gtk_image_new ();
	}
	pixmap = gtk_image_new_from_file (pathname);
	g_free (pathname);
	return pixmap;
}


/* All the return strings MUST be freed */
gchar *
anjuta_res_get_pixmap_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_PIXMAPS_DIR);
	if (file_is_directory(path))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_data_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_DATA_DIR);
	if (file_is_directory(path))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_help_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_HELP_DIR);
	if (file_is_directory(path))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_help_dir_locale (const gchar * locale)
{
	gchar* path;
	if (locale)
		path = g_strconcat (PACKAGE_HELP_DIR, "/", locale, NULL);
	else
		path = g_strdup (PACKAGE_HELP_DIR);
	if (file_is_directory(path))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_doc_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_DOC_DIR);
	if (file_is_directory(path))
		return path;
	g_free (path);
	return NULL;
}

/* All the return strings MUST be freed */
gchar *
anjuta_res_get_pixmap_file (const gchar * pixfile)
{
	gchar* path;
	g_return_val_if_fail (pixfile != NULL, NULL);
	path = g_strconcat (PACKAGE_PIXMAPS_DIR, "/", pixfile, NULL);
	if (file_is_regular(path))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_data_file (const gchar * datafile)
{
	gchar* path;
	g_return_val_if_fail (datafile != NULL, NULL);
	path = g_strconcat (PACKAGE_DATA_DIR, "/", datafile, NULL);
	if (file_is_regular(path))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_help_file (const gchar * helpfile)
{
	gchar* path;
	g_return_val_if_fail (helpfile != NULL, NULL);
	path = g_strconcat (PACKAGE_HELP_DIR, "/", helpfile, NULL);
	if (file_is_regular(path))
		return path;
	g_free (path);
	return NULL;
}
	
gchar *
anjuta_res_get_help_file_locale (const gchar * helpfile, const gchar * locale)
{
	gchar* path;
	g_return_val_if_fail (helpfile != NULL, NULL);
	if (locale)
		path = g_strconcat (PACKAGE_HELP_DIR, "/", locale, "/",
				    helpfile, NULL);
	else
		path = g_strconcat (PACKAGE_HELP_DIR, "/", helpfile, NULL);
	if (file_is_regular(path))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_doc_file (const gchar * docfile)
{
	gchar* path;
	g_return_val_if_fail (docfile != NULL, NULL);
	path = g_strconcat (PACKAGE_DOC_DIR, "/", docfile, NULL);
	if (file_is_regular(path))
		return path;
	g_free (path);
	return NULL;
}

void
anjuta_res_help_search (const gchar * word)
{
	if(word)
	{
		if(fork()==0)
		{
			execlp("devhelp", "devhelp", "-s", word, NULL);
			g_error (_("Cannot execute Devhelp. Make sure it is installed"));
		}
	}
	else
	{
		if(fork()==0)
		{
			execlp("devhelp", "devhelp", NULL);
			g_error (_("Cannot execute Devhelp. Make sure it is installed"));
		}
	}
}

void anjuta_res_url_show (const char *url)
{
	GError *error = NULL;
	gnome_url_show (url, &error);
}
