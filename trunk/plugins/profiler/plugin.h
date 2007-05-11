/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
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

#ifndef _PROFILER_H_
#define _PROFILER_H_

#include <config.h>
#include <glib/gstdio.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs.h>
#include "gprof-view-manager.h"
#include "gprof-profile-data.h"
#include "gprof-flat-profile-view.h"
#include "gprof-call-graph-view.h"
#include "gprof-function-call-tree-view.h"

#ifdef HAVE_GRAPHVIZ
#include "gprof-function-call-chart-view.h"
#endif

extern GType profiler_get_type (AnjutaGluePlugin *plugin);
#define TYPE_PROFILER         (profiler_get_type (NULL))
#define PROFILER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_PROFILER, Profiler))
#define PROFILER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), TYPE_PROFILER, ProfilerClass))
#define IS_PROFILER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_PROFILER))
#define IS_PROFILER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_PROFILER))
#define PROFILER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_PROFILER, ProfilerClass))

typedef struct _Profiler Profiler;
typedef struct _ProfilerClass ProfilerClass;

struct _Profiler
{
	AnjutaPlugin parent;
	
	gint uiid;
	GtkActionGroup *action_group;
	AnjutaPreferences *prefs;
	GladeXML *prefs_gxml;
	GProfViewManager *view_manager;
	GProfProfileData *profile_data;
	gint project_watch_id;
	gchar *project_root_uri;
	gchar *profile_target_path;
	GnomeVFSMonitorHandle *profile_data_monitor;
};

struct _ProfilerClass
{
	AnjutaPluginClass parent_class;
};

#endif
