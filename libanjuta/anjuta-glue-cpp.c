/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-glue-cpp.c (c) 2006 Johannes Schmid
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

/**
 * SECTION:anjuta-glue-cpp
 * @title: AnjutaAnjutaGlueCpp
 * @short_description: C++ Anjuta glue code
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-glue-cpp.h
 * 
 */

#include "anjuta-glue-cpp.h"

#include <string.h>
#include <gmodule.h>

typedef GObject* (*AnjutaGlue_constructor)();

GObject*
anjuta_glue_cpp_load_plugin(AnjutaGlueFactory* factory, const gchar* component_name, const gchar* type_name)
{
	GList* p = anjuta_glue_factory_get_path(factory);
	gchar *plugin_name;
  
	plugin_name = g_module_build_path (NULL, component_name);
  
  	AnjutaGlue_constructor constructor;
  
	while (p)
    {
		const gchar *file_name;
 		PathEntry *entry = p->data;
		GDir *dir;
		GObject* plugin;
      
		dir = g_dir_open (entry->path, 0, NULL);

		if (dir == NULL)
			continue;
      
		do {
			file_name = g_dir_read_name (dir);
	
			if (file_name && strcmp (file_name, plugin_name) == 0) {
				GModule *module;
	  			gchar *plugin_path;
	  
	  			/* We have found a matching module */
	  			plugin_path = g_module_build_path (entry->path, plugin_name);
	  			module = g_module_open (plugin_path, 0);
	  			if (module == NULL)
	    		{
	      			g_warning ("Could not open module: %s\n", g_module_error ());
	     		 	goto move_to_next_dir;
	    		}

	  			if (!g_module_symbol (module, "anjuta_glue_constructor", (gpointer *)(gpointer)&constructor))
	    		{
	      			g_module_close (module);
	      			goto move_to_next_dir;
	    		}
				/* Create the object */
				plugin =  (*constructor)();
				return plugin;
			}
		} while (file_name != NULL);
move_to_next_dir:	  
      g_dir_close (dir);
      
      p = p->next;
    }

  return NULL;
}
