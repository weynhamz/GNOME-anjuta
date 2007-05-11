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

#include "style-editor.h"

extern GType editor_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_EDITOR         (editor_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_EDITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_EDITOR, EditorPlugin))
#define ANJUTA_PLUGIN_EDITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_EDITOR, EditorPluginClass))
#define ANJUTA_IS_PLUGIN_EDITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_EDITOR))
#define ANJUTA_IS_PLUGIN_EDITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_EDITOR))
#define ANJUTA_PLUGIN_EDITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_EDITOR, EditorPluginClass))

typedef struct _EditorPlugin EditorPlugin;
typedef struct _EditorPluginClass EditorPluginClass;

struct _EditorPlugin{
	AnjutaPlugin parent;
	
	GtkWidget* style_button;
};

struct _EditorPluginClass{
	AnjutaPluginClass parent_class;
};
