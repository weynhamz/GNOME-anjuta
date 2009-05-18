/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) James Liggett 2008

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#ifndef _PLUGIN_H
#define _PLUGIN_H

#include <config.h>
#include <gio/gio.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>

extern GType git_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_GIT         (git_get_type (NULL))
#define ANJUTA_PLUGIN_GIT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_GIT, Git))
#define ANJUTA_PLUGIN_GIT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_GIT, GitClass))
#define ANJUTA_IS_PLUGIN_GIT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_GIT))
#define ANJUTA_IS_PLUGIN_GIT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_GIT))
#define ANJUTA_PLUGIN_GIT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_GIT, GitClass))

typedef struct _Git Git;
typedef struct _GitClass GitClass;

#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/anjuta-git.ui"
#define ICON_FILE "anjuta-git-plugin-48.png"

struct _Git
{
	AnjutaPlugin parent;
	gint uiid;
	gchar *project_root_directory;
	gchar *current_editor_filename;
	gchar *current_fm_filename;
	IAnjutaMessageView *message_view;
	
	/* Watches */
	gint project_root_watch_id;
	gint editor_watch_id;
	gint fm_watch_id;
	
	GtkWidget *log_viewer;
	GtkWidget *log_popup_menu;
	
	/* File monitors */
	GFileMonitor *bisect_file_monitor;
};

struct _GitClass
{
	AnjutaPluginClass parent_class;
};

#endif
