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

#define GPL_HEADING "/*\n" \
" *  This program is free software; you can redistribute it and/or modify\n" \
" *  it under the terms of the GNU General Public License as published by\n" \
" *  the Free Software Foundation; either version 2 of the License, or\n" \
" *  (at your option) any later version.\n" \
" *\n" \
" *  This program is distributed in the hope that it will be useful,\n" \
" *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
" *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
" *  GNU Library General Public License for more details.\n" \
" *\n" \
" *  You should have received a copy of the GNU General Public License\n" \
" *  along with this program; if not, write to the Free Software\n" \
" *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n" \
" */\n\n" 

#define LGPL_HEADING "/*\n" \
" * This program is free software; you can redistribute it and/or\n" \
" * modify it under the terms of the GNU Lesser General Public\n" \
" * License as published by the Free Software Foundation; either\n" \
" * version 2.1 of the License, or (at your option) any later version.\n" \
" * \n" \
" * This program is distributed in the hope that it will be useful,\n" \
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n" \
" * Lesser General Public License for more details.\n" \
" * \n" \
" * You should have received a copy of the GNU Lesser General Public\n" \
" * License along with main.c; if not, write to:\n" \
" *            The Free Software Foundation, Inc.,\n" \
" *            59 Temple Place - Suite 330,\n" \
" *            Boston,  MA  02111-1307, USA.\n" \
" */\n\n"

#define SAFE_FREE(x) { \
	if(x != NULL) \
		g_free(x); \
	}

#define FETCH_STRING(gxml, str) \
		gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (gxml, str)));
		
#define FETCH_BOOLEAN(gxml, str) \
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, str)));

/* note: this must follow the order as it is in "License" combo-box */
enum {
	GPL,
	LGPL,
	NO_LICENSE
};

/* inheritance items */
enum {
	INHERIT_PUBLIC,
	INHERIT_PROTECTED,
	INHERIT_PRIVATE
};


gchar *browse_for_file (gchar *title);	
	
gboolean gobject_class_create_code (ClassGenData* data);
gboolean transform_file (const gchar *input_file, const gchar *output_file, 
					gchar **replace_table, const gchar *author, const gchar *email, 
					gboolean date_output, gboolean gpl_output);
gchar *cstr_replace_all (gchar *search, const gchar *find, const gchar *replace);

gboolean generic_cpp_class_create_code (ClassGenData *data);


#endif
