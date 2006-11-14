/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2004 Naba Kumar, Johannes Schmid

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PLUGIN_H
#define PLUGIN_H

#include <config.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-cvs-plugin.glade"

extern GType cvs_plugin_type;
#define CVS_PLUGIN_TYPE (cvs_plugin_type)
#define CVS_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), CVS_PLUGIN_TYPE, CVSPlugin))

typedef struct _CVSPlugin CVSPlugin;
typedef struct _CVSPluginClass CVSPluginClass;

struct _CVSPlugin{
	AnjutaPlugin parent;
	
	IAnjutaMessageView* mesg_view;
	IAnjutaEditor* diff_editor;
	AnjutaLauncher* launcher;
	gboolean executing_command;
	
	/* UI merge id */
	gint uiid;
	
	/* Action groups */
	GtkActionGroup *cvs_action_group;
	GtkActionGroup *cvs_popup_action_group;
	
	/* Watch IDs */
	gint fm_watch_id;
	gint project_watch_id;
	gint editor_watch_id;

	/* Watched values */
	gchar *fm_current_filename;
	gchar *project_root_dir;
	gchar *current_editor_filename;
};

struct _CVSPluginClass{
	AnjutaPluginClass parent_class;
};

#endif
