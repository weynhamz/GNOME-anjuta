/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ANJUTA_DOCK_H_
#define _ANJUTA_DOCK_H_

#include <glib-object.h>
#include <gdl/gdl-dock.h>
#include <gdl/gdl-dock-object.h>
#include "anjuta-dock-pane.h"
#include "anjuta-command-bar.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_DOCK             (anjuta_dock_get_type ())
#define ANJUTA_DOCK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_DOCK, AnjutaDock))
#define ANJUTA_DOCK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_DOCK, AnjutaDockClass))
#define ANJUTA_IS_DOCK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_DOCK))
#define ANJUTA_IS_DOCK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_DOCK))
#define ANJUTA_DOCK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_DOCK, AnjutaDockClass))

typedef struct _AnjutaDockClass AnjutaDockClass;
typedef struct _AnjutaDock AnjutaDock;
typedef struct _AnjutaDockPriv AnjutaDockPriv;

struct _AnjutaDockClass
{
	GdlDockClass parent_class;
};

struct _AnjutaDock
{
	GdlDock parent_instance;

	AnjutaDockPriv *priv;
};

GType anjuta_dock_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_dock_new (void);
gboolean anjuta_dock_add_pane (AnjutaDock *self, const gchar *pane_name,
                               const gchar *pane_label, const gchar *stock_icon,
                               AnjutaDockPane *pane, GdlDockPlacement placement, 
                               AnjutaCommandBarEntry *entries, int num_entries,
                               gpointer user_data);
gboolean anjuta_dock_add_pane_full (AnjutaDock *self, const gchar *pane_name,
                                    const gchar *pane_label, 
                                    const gchar *stock_icon,  AnjutaDockPane *pane, 
                                    GdlDockPlacement placement,
                                    AnjutaCommandBarEntry *entries, int num_entries,
                                    gpointer user_data, 
                                    GdlDockItemBehavior behavior);
void anjuta_dock_replace_command_pane (AnjutaDock *self, const gchar *pane_name,
                                       const gchar *pane_label, const gchar *stock_icon,
                                       AnjutaDockPane *pane, GdlDockPlacement placement, 
                                       AnjutaCommandBarEntry *entries, int num_entries,
                                       gpointer user_data);
void anjuta_dock_remove_pane (AnjutaDock *self, AnjutaDockPane *pane);
void anjuta_dock_show_pane (AnjutaDock *self, AnjutaDockPane *pane);
void anjuta_dock_hide_pane (AnjutaDock *self, AnjutaDockPane *pane);
void anjuta_dock_present_pane (AnjutaDock *self, AnjutaDockPane *pane);
void anjuta_dock_set_command_bar (AnjutaDock *self, 
                                  AnjutaCommandBar *command_bar);
AnjutaCommandBar* anjuta_dock_get_command_bar (AnjutaDock *self);

G_END_DECLS

#endif /* _ANJUTA_DOCK_H_ */
