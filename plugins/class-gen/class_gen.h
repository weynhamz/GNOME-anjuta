/*
 *  Copyright (C) 2005 Massimo Cora'
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef __CLASS_GEN_H__
#define __CLASS_GEN_H__

#include "plugin.h"
#include "action-callbacks.h"

#define SAFE_FREE(x) { \
	if(x != NULL) \
		g_free(x); \
	}

#define FETCH_STRING(gxml, str) \
		gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (gxml, str)));
		
#define FETCH_BOOLEAN(gxml, str) \
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, str)));


gchar *browse_for_file (gchar *title);	
	
gboolean gobject_class_create_code (ClassGenData* data);
gboolean transform_file (const gchar *input_file, const gchar *output_file, 
					gchar **replace_table, const gchar *author, const gchar *email, 
					gboolean date_output, gboolean gpl_output);
gchar *cstr_replace_all (gchar *search, const gchar *find, const gchar *replace);

gboolean generic_cpp_class_create_code (ClassGenData *data);


#endif
