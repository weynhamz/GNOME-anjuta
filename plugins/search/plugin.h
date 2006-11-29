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

extern GType search_plugin_get_type(GluePlugin *klass);
#define ANJUTA_TYPE_PLUGIN_SEARCH         (search_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_SEARCH(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_SEARCH, SearchPlugin))
#define ANJUTA_PLUGIN_SEARCH_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_SEARCH, SearchPluginClass))
#define ANJUTA_IS_PLUGIN_SEARCH(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_SEARCH))
#define ANJUTA_IS_PLUGIN_SEARCH_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_SEARCH))
#define ANJUTA_PLUGIN_SEARCH_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_SEARCH, SearchPluginClass))

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
