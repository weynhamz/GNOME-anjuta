/*
    resources.h
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
#ifndef _RESOURCES_H_
#define _RESOURCES_H_

#ifdef __cplusplus
extern "C"
{
#endif

GtkWidget* anjuta_res_lookup_widget (GtkWidget       *widget, const gchar     *widget_name);
GtkWidget* anjuta_res_get_pixmap_widget (GtkWidget* widget, const gchar* pixfile, gboolean gnome_pixmap);
GdkImlibImage* anjuta_res_get_image  (const gchar     *filename);

/* All the return strings MUST be freed */
gchar* anjuta_res_get_pixmap_dir (void);
gchar* anjuta_res_get_data_dir (void);
gchar* anjuta_res_get_help_dir (void);
gchar* anjuta_res_get_help_dir_locale (const gchar* locale);
gchar* anjuta_res_get_doc_dir (void);

/* All the return strings MUST be freed */
gchar* anjuta_res_get_pixmap_file (const gchar* pixfile);
gchar* anjuta_res_get_data_file (const gchar* pixfile);
gchar* anjuta_res_get_help_file (const gchar* helpfile);
gchar* anjuta_res_get_help_file_locale (const gchar* helpfile, const gchar* locale);
gchar* anjuta_res_get_doc_file (const gchar* docfile);

/* System Help */
void anjuta_res_help_search (const gchar * word);

/* URI handler -- wrapped over gnome_url_show() */
void anjuta_res_url_show(const char *url);

#ifdef __cplusplus
}
#endif

#endif /* _RESOURCES_H_ */
