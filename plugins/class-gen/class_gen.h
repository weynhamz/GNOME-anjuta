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

#define SAFE_FREE(x) { if(x != NULL) g_free(x); }

GtkWidget* class_gen_create_dialog_class (AnjutaClassGenPlugin* plugin);
void class_gen_create_code_class (AnjutaClassGenPlugin* plugin);
void class_gen_init (AnjutaClassGenPlugin* plugin);
void class_gen_del (AnjutaClassGenPlugin* plugin);
void class_gen_show (AnjutaClassGenPlugin* plugin);
void class_gen_get_strings (AnjutaClassGenPlugin* plugin);
void class_gen_generate (AnjutaClassGenPlugin *plugin);
void class_gen_set_root (AnjutaClassGenPlugin* plugin, gchar* root_dir);
void class_gen_message_box(const char* szMsg);

gboolean is_legal_class_name (const char* szClassName); 
gboolean is_legal_file_name (const char* szFileName);



#endif
