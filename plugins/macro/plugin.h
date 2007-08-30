/* 
 * 	plugin.h (c) 2004 Johannes Schmid
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include <config.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

//#include "macro-db.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-macro.glade"

extern GType macro_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_MACRO         (macro_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_MACRO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_MACRO, MacroPlugin))
#define ANJUTA_PLUGIN_MACRO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_MACRO, MacroPluginClass))
#define ANJUTA_IS_PLUGIN_MACRO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_MACRO))
#define ANJUTA_IS_PLUGIN_MACRO_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_MACRO))
#define ANJUTA_PLUGIN_MACRO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_MACRO, MacroPluginClass))

typedef struct _MacroPlugin MacroPlugin;
typedef struct _MacroPluginClass MacroPluginClass;
#include "macro-db.h"
struct _MacroPlugin
{
	AnjutaPlugin parent;
	
	/* Merge id */
	gint uiid;

	/* Action group */
	GtkActionGroup *action_group;
	
	/* Watch IDs */
	gint editor_watch_id;

	/* Watched values */
	GObject *current_editor;

	GtkWidget *macro_dialog;
	MacroDB *macro_db;
};

struct _MacroPluginClass
{
	AnjutaPluginClass parent_class;
};

gboolean
macro_insert (MacroPlugin * plugin, const gchar *keyword);

#endif
