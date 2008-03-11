/***************************************************************************
 *            build-options.h
 *
 *  Sat Mar  1 20:47:23 2008
 *  Copyright  2008  Johannes Schmid
 *  <jhs@gnome.org>
 ****************************************************************************/

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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

#include <gtk/gtk.h>

gboolean build_dialog_configure (GtkWindow* parent, const gchar* dialog_title,
																 GHashTable** build_options,
																 const gchar* default_args, gchar** args);


#endif /* BUILD_OPTIONS_H */
