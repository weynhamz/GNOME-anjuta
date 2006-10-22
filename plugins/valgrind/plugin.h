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
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#ifndef _VALGRIND_PLUGIN_H_
#define _VALGRIND_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include "preferences.h"


G_BEGIN_DECLS

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
