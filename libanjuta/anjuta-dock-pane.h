/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
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

#ifndef _ANJUTA_DOCK_PANE_H_
#define _ANJUTA_DOCK_PANE_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-plugin.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_DOCK_PANE             (anjuta_dock_pane_get_type ())
#define ANJUTA_DOCK_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_DOCK_PANE, AnjutaDockPane))
#define ANJUTA_DOCK_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_DOCK_PANE, AnjutaDockPaneClass))
#define ANJUTA_IS_DOCK_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_DOCK_PANE))
#define ANJUTA_IS_DOCK_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_DOCK_PANE))
#define ANJUTA_DOCK_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_DOCK_PANE, AnjutaDockPaneClass))

typedef struct _AnjutaDockPaneClass AnjutaDockPaneClass;
typedef struct _AnjutaDockPane AnjutaDockPane;
typedef struct _AnjutaDockPanePriv AnjutaDockPanePriv;

struct _AnjutaDockPaneClass
{
	GObjectClass parent_class;

	/* Virtual methods */
	void (*refresh) (AnjutaDockPane *self);
	GtkWidget * (*get_widget) (AnjutaDockPane *self);
};

struct _AnjutaDockPane
{
	GObject parent_instance;

	AnjutaDockPanePriv *priv;
};

GType anjuta_dock_pane_get_type (void) G_GNUC_CONST;
void anjuta_dock_pane_refresh (AnjutaDockPane *self);
GtkWidget *anjuta_dock_pane_get_widget (AnjutaDockPane *self);
AnjutaPlugin *anjuta_dock_pane_get_plugin (AnjutaDockPane *self);

G_END_DECLS

#endif /* _ANJUTA_DOCK_PANE_H_ */
