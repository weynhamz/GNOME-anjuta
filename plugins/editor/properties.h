/*
 * properties.h Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_


#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef gint PropsID;

PropsID prop_set_new (void);
void prop_set_destroy (PropsID p);
gpointer prop_get_pointer (PropsID p);

void prop_set_with_key (PropsID p, const gchar *key, const gchar *val);
void prop_set_int_with_key (PropsID p, const gchar *key, int val);
void prop_set (PropsID p, const gchar *keyval);
gchar* prop_get (PropsID p, const gchar *key);
gchar* prop_get_expanded (PropsID p, const gchar *key);
gchar* prop_expand (PropsID p, const gchar *withvars);
int prop_get_int (PropsID p, const gchar *key, gint defaultValue);
gchar* prop_get_wild (PropsID p, const gchar *keybase, const gchar *filename);
gchar* prop_get_new_expand (PropsID p, const gchar *keybase, const gchar *filename);
void prop_clear (PropsID p);
void prop_read_from_memory (PropsID p, const gchar *data,
							gint len, const gchar *directoryForImports);
void prop_read (PropsID p, const gchar *filename, const gchar *directoryForImports);
void prop_set_parent (PropsID p1, PropsID p2);
GList* prop_glist_from_data (guint props, const gchar* id);

#ifdef __cplusplus
}
#endif

#endif /* _PROPERTIES_H_ */
