/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vgtoolprefs.h"


static void vg_tool_prefs_class_init (VgToolPrefsClass *klass);
static void vg_tool_prefs_init (VgToolPrefs *prefs);
static void vg_tool_prefs_destroy (GtkObject *obj);
static void vg_tool_prefs_finalize (GObject *obj);

static void tool_prefs_apply (VgToolPrefs *prefs);
static void tool_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);


static GtkVBoxClass *parent_class = NULL;


GType
vg_tool_prefs_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgToolPrefsClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_tool_prefs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgToolPrefs),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_tool_prefs_init,
		};
		
		type = g_type_register_static (GTK_TYPE_VBOX, "VgToolPrefs", &info, 0);
	}
	
	return type;
}

static void
vg_tool_prefs_class_init (VgToolPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_ref (GTK_TYPE_VBOX);
	
	object_class->finalize = vg_tool_prefs_finalize;
	gtk_object_class->destroy = vg_tool_prefs_destroy;
	
	/* virtual methods */
	klass->apply = tool_prefs_apply;
	klass->get_argv = tool_prefs_get_argv;
}

static void
vg_tool_prefs_init (VgToolPrefs *prefs)
{
	;
}

static void
vg_tool_prefs_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_tool_prefs_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
tool_prefs_apply (VgToolPrefs *prefs)
{
	;
}


void
vg_tool_prefs_apply (VgToolPrefs *prefs)
{
	g_return_if_fail (VG_IS_TOOL_PREFS (prefs));
	
	VG_TOOL_PREFS_GET_CLASS (prefs)->apply (prefs);
}


static void
tool_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv)
{
	;
}


void
vg_tool_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv)
{
	g_return_if_fail (VG_IS_TOOL_PREFS (prefs));
	g_return_if_fail (argv != NULL);
	
	VG_TOOL_PREFS_GET_CLASS (prefs)->get_argv (prefs, tool, argv);
}
