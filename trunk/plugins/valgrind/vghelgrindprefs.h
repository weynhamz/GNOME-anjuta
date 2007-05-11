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


#ifndef __VG_HELGRIND_PREFS_H__
#define __VG_HELGRIND_PREFS_H__

#include "vgtoolprefs.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_HELGRIND_PREFS            (vg_helgrind_prefs_get_type ())
#define VG_HELGRIND_PREFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_HELGRIND_PREFS, VgHelgrindPrefs))
#define VG_HELGRIND_PREFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_HELGRIND_PREFS, VgHelgrindPrefsClass))
#define VG_IS_HELGRIND_PREFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_HELGRIND_PREFS))
#define VG_IS_HELGRIND_PREFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_HELGRIND_PREFS))
#define VG_HELGRIND_PREFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_HELGRIND_PREFS, VgHelgrindPrefsClass))

typedef struct _VgHelgrindPrefs VgHelgrindPrefs;
typedef struct _VgHelgrindPrefsClass VgHelgrindPrefsClass;

struct _VgHelgrindPrefs {
	VgToolPrefs parent_object;
	
	GtkToggleButton *private_stacks;
	GtkOptionMenu *show_last_access;
};

struct _VgHelgrindPrefsClass {
	VgToolPrefsClass parent_class;
	
};


GType vg_helgrind_prefs_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_HELGRIND_PREFS_H__ */
