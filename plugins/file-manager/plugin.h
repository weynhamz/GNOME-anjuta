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
#include "file-model.h"

#define ANJUTA_TYPE_FILE_MANAGER             (file_manager_get_type (NULL))
#define ANJUTA_FILE_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_FILE_MANAGER, AnjutaFileManager))
#define ANJUTA_FILE_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_FILE_MANAGER, AnjutaFileManagerClass))
#define ANJUTA_IS_FILE_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_FILE_MANAGER))
#define ANJUTA_IS_FILE_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_FILE_MANAGER))
#define ANJUTA_FILE_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_FILE_MANAGER, AnjutaFileManagerClass))

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
	GSettings* settings;

	guint current_document_watch_id;
};

struct _AnjutaFileManagerClass {
	AnjutaPluginClass parent_class;
};

GType file_manager_get_type (GTypeModule *module);

#endif
