/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  plugin.h
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
 
#ifndef __CLASS_GEN_PLUGIN_H__
#define __CLASS_GEN_PLUGIN_H__

#include "window.h"
#include "generator.h"

#include <config.h>
#include <libanjuta/anjuta-plugin.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-class-gen-plugin.glade"
#define CLASS_TEMPLATE PACKAGE_DATA_DIR"/class-templates/"

extern GType class_gen_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_CLASS_GEN         (class_gen_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_CLASS_GEN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_CLASS_GEN, AnjutaClassGenPlugin))
#define ANJUTA_PLUGIN_CLASS_GEN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_CLASS_GEN, AnjutaClassGenPluginClass))
#define ANJUTA_IS_PLUGIN_CLASS_GEN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_CLASS_GEN))
#define ANJUTA_IS_PLUGIN_CLASS_GEN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_CLASS_GEN))
#define ANJUTA_PLUGIN_CLASS_GEN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_CLASS_GEN, AnjutaClassGenPluginClass))

typedef struct _AnjutaClassGenPlugin AnjutaClassGenPlugin;
typedef struct _AnjutaClassGenPluginClass AnjutaClassGenPluginClass;
	

struct _AnjutaClassGenPlugin
{
	AnjutaPlugin parent;

	AnjutaPreferences *prefs;	
	gchar *top_dir;
	guint root_watch_id;

	CgWindow* window;
	CgGenerator* generator;
};

struct _AnjutaClassGenPluginClass
{
	AnjutaPluginClass parent_class;
};


#endif /* __CLASS_GEN_PLUGIN_H__ */
