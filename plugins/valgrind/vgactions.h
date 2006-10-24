/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Massimo Cora' 2005 <maxcvs@gmail.com>
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

#ifndef __VGACTIONS_H
#define __VGACTIONS_H

#include <glib.h>
#include <glib-object.h>

#include "vgerror.h"
#include "process.h"
#include "symtab.h"
#include "preferences.h"

G_BEGIN_DECLS

#define VG_TYPE_ACTIONS         (vg_actions_get_type ())
#define VG_ACTIONS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), VG_TYPE_ACTIONS, VgActions))
#define VG_ACTIONS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), VG_TYPE_ACTIONS, VgActionsClass))
#define VG_IS_ACTIONS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), VG_TYPE_ACTIONS))
#define VG_IS_ACTIONS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), VG_TYPE_ACTIONS))
#define VG_ACTIONS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), VG_TYPE_ACTIONS, VgActionsClass))


typedef struct _VgActionsClass VgActionsClass;
typedef struct _VgActions VgActions;
typedef struct _VgActionsPriv VgActionsPriv;

#include "plugin.h"


struct _VgActionsClass {
	GObjectClass parent_class;

};

struct _VgActions {
	GObject parent;
	VgActionsPriv *priv;
};



GType vg_actions_get_type (void);
VgActions *vg_actions_new (AnjutaValgrindPlugin *anjuta_plugin, 
					ValgrindPluginPrefs **prefs, GtkWidget *vg_default_view);
void vg_actions_run (VgActions *actions, gchar* prg_to_debug, gchar* tool, 
					GError **err);
void vg_actions_kill (VgActions *actions);
void vg_actions_set_pid (VgActions *actions, pid_t pid);
void vg_actions_set_giochan (VgActions *actions, GIOChannel*gio);
void vg_actions_set_watch_id (VgActions *actions, guint watch_id);


G_END_DECLS

#endif /* __VGACTIONS_H */
