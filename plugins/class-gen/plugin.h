/*
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
 
#ifndef __CLASS_GEN_PLUGIN_H__
#define __CLASS_GEN_PLUGIN_H__

#include <config.h>
#include <libanjuta/anjuta-plugin.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-class-gen-plugin.glade"
#define CLASS_TEMPLATE PACKAGE_DATA_DIR"/class-templates/"

// templates filenames
#define CLASS_GOC_HEADER_TEMPLATE	"goc_template_header"
#define CLASS_GOC_SOURCE_TEMPLATE	"goc_template_source"

extern GType class_gen_type;
#define ANJUTA_CLASS_GEN_PLUGIN_TYPE (class_gen_type)
#define ANJUTA_CLASS_GEN_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_CLASS_GEN_PLUGIN_TYPE, AnjutaClassGenPlugin))

typedef struct _AnjutaClassGenPlugin AnjutaClassGenPlugin;
typedef struct _AnjutaClassGenPluginClass AnjutaClassGenPluginClass;
	

struct _AnjutaClassGenPlugin {
	AnjutaPlugin parent;

	AnjutaPreferences *prefs;	
	gchar *top_dir;
	guint root_watch_id;
};

struct _AnjutaClassGenPluginClass {
	AnjutaPluginClass parent_class;
};


#endif /* __CLASS_GEN_PLUGIN_H__ */
