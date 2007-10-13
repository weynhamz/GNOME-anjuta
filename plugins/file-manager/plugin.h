/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _FILE_MANAGER_H_
#define _FILE_MANAGER_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>

#include "file-view.h"

typedef struct _AnjutaFileManager AnjutaFileManager;
typedef struct _AnjutaFileManagerClass AnjutaFileManagerClass;

struct _AnjutaFileManager {
	AnjutaPlugin parent;
	AnjutaFileView* fv;
	GtkWidget* sw;
	guint root_watch_id;
	gboolean have_project;
	
	gint uiid;
	GtkActionGroup *action_group;
	GList* gconf_notify_ids;
	AnjutaPreferences* prefs;
};

struct _AnjutaFileManagerClass {
	AnjutaPluginClass parent_class;
};

#endif
