/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-view-manager.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-view-manager.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#ifndef _GPROF_VIEW_MANAGER_H
#define _GPROF_VIEW_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gprof-profile-data.h"
#include "gprof-view.h"

G_BEGIN_DECLS

typedef struct _GProfViewManager         GProfViewManager;
typedef struct _GProfViewManagerClass    GProfViewManagerClass;
typedef struct _GProfViewManagerPriv     GProfViewManagerPriv;

#define GPROF_VIEW_MANAGER_TYPE            (gprof_view_manager_get_type ())
#define GPROF_VIEW_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_VIEW_MANAGER_TYPE, GProfViewManager))
#define GPROF_VIEW_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_VIEW_MANAGER_TYPE, GProfViewManagerClass))
#define IS_GPROF_VIEW_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_VIEW_MANAGER_TYPE))
#define IS_GPROF_VIEW_CLASS_MANAGER(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_VIEW_MANAGER_TYPE))

struct  _GProfViewManager
{
	GObject parent;
	GProfViewManagerPriv *priv;
};

struct _GProfViewManagerClass
{
	GObjectClass parent_class;
};

GType gprof_view_manager_get_type (void);

GProfViewManager *gprof_view_manager_new (void);
void gprof_view_manager_free (GProfViewManager *self);

void gprof_view_manager_add_view (GProfViewManager *self, GProfView *view, 
								  const gchar *label);
void gprof_view_manager_refresh_views (GProfViewManager *self);
GtkWidget *gprof_view_manager_get_notebook (GProfViewManager *self);

G_END_DECLS

#endif
