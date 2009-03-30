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


#ifndef __VG_MEMCHECK_PREFS_H__
#define __VG_MEMCHECK_PREFS_H__

#include "vgtoolprefs.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_MEMCHECK_PREFS            (vg_memcheck_prefs_get_type ())
#define VG_MEMCHECK_PREFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_MEMCHECK_PREFS, VgMemcheckPrefs))
#define VG_MEMCHECK_PREFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_MEMCHECK_PREFS, VgMemcheckPrefsClass))
#define VG_IS_MEMCHECK_PREFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_MEMCHECK_PREFS))
#define VG_IS_MEMCHECK_PREFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_MEMCHECK_PREFS))
#define VG_MEMCHECK_PREFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_MEMCHECK_PREFS, VgMemcheckPrefsClass))

typedef struct _VgMemcheckPrefs VgMemcheckPrefs;
typedef struct _VgMemcheckPrefsClass VgMemcheckPrefsClass;

struct _VgMemcheckPrefs {
	VgToolPrefs parent_object;
	
	GtkComboBox *leak_check;
	GtkToggleButton *show_reachable;
	GtkComboBox *leak_resolution;
	GtkSpinButton *freelist_vol;
	GtkToggleButton *workaround_gcc296_bugs;
	GtkToggleButton *avoid_strlen_errors;
};

struct _VgMemcheckPrefsClass {
	VgToolPrefsClass parent_class;
	
};


GType vg_memcheck_prefs_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_MEMCHECK_PREFS_H__ */
