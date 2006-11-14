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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
 
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-ui.h>

#include <libanjuta/interfaces/ianjuta-document-manager.h>

extern GType search_plugin_type;
#define SEARCH_PLUGIN_TYPE (search_plugin_type)
#define SEARCH_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), SEARCH_PLUGIN_TYPE, SearchPlugin))

typedef struct _SearchPlugin SearchPlugin;
typedef struct _SearchPluginClass SearchPluginClass;

struct _SearchPlugin{
	AnjutaPlugin parent;
	
	gint uiid;
	IAnjutaDocumentManager* docman;
};

struct _SearchPluginClass{
	AnjutaPluginClass parent_class;
};
