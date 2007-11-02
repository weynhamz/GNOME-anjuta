/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Massimo Cora' 2005 <maxcvs@gmail.com>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

#ifndef _VALGRIND_PLUGIN_H_
#define _VALGRIND_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include "preferences.h"


G_BEGIN_DECLS

extern GType anjuta_valgrind_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_VALGRIND         (anjuta_valgrind_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_VALGRIND(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_VALGRIND, AnjutaValgrindPlugin))
#define ANJUTA_PLUGIN_VALGRIND_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_VALGRIND, AnjutaValgrindPluginClass))
#define ANJUTA_IS_PLUGIN_VALGRIND(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_VALGRIND))
#define ANJUTA_IS_PLUGIN_VALGRIND_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_VALGRIND))
#define ANJUTA_PLUGIN_VALGRIND_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_VALGRIND, AnjutaValgrindPluginClass))

typedef struct _AnjutaValgrindPlugin AnjutaValgrindPlugin;
typedef struct _AnjutaValgrindPluginClass AnjutaValgrindPluginClass;

#include "vgactions.h"

struct _AnjutaValgrindPlugin{
	AnjutaPlugin parent;

	gboolean is_busy;
	
	gchar *project_root_uri;
	gint root_watch_id;
	
	GtkWidget *valgrind_widget;			/* a VgToolView object */
	gboolean valgrind_displayed;	
	GtkWidget *general_prefs;
	VgActions *val_actions;
	
	ValgrindPluginPrefs * val_prefs;
	
	gint uiid;
	GtkActionGroup *action_group;
};

struct _AnjutaValgrindPluginClass{
	AnjutaPluginClass parent_class;
};


void valgrind_set_busy_status (AnjutaValgrindPlugin *plugin, gboolean status);
void valgrind_update_ui (AnjutaValgrindPlugin *plugin);

G_END_DECLS

#endif /* _VALGRIND_PLUGIN_H_ */
