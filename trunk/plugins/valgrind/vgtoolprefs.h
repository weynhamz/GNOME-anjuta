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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef __VG_TOOL_PREFS_H__
#define __VG_TOOL_PREFS_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_TOOL_PREFS            (vg_tool_prefs_get_type ())
#define VG_TOOL_PREFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_TOOL_PREFS, VgToolPrefs))
#define VG_TOOL_PREFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_TOOL_PREFS, VgToolPrefsClass))
#define VG_IS_TOOL_PREFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_TOOL_PREFS))
#define VG_IS_TOOL_PREFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_TOOL_PREFS))
#define VG_TOOL_PREFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_TOOL_PREFS, VgToolPrefsClass))

typedef struct _VgToolPrefs VgToolPrefs;
typedef struct _VgToolPrefsClass VgToolPrefsClass;

struct _VgToolPrefs {
	GtkVBox parent_object;
	
	const char *label;
};

struct _VgToolPrefsClass {
	GtkVBoxClass parent_class;
	
	/* virtual methods */
	void (* apply) (VgToolPrefs *prefs);
	void (* get_argv) (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);
};


GType vg_tool_prefs_get_type (void);

void vg_tool_prefs_apply (VgToolPrefs *prefs);

void vg_tool_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_TOOL_PREFS_H__ */
