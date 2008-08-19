/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-view.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-view.h is free software.
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
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

#ifndef _GPROF_VIEW_H
#define _GPROF_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include "gprof-profile-data.h"

G_BEGIN_DECLS

typedef struct _GProfView         GProfView;
typedef struct _GProfViewClass    GProfViewClass;
typedef struct _GProfViewPriv     GProfViewPriv;

#define GPROF_VIEW_TYPE            (gprof_view_get_type ())
#define GPROF_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_VIEW_TYPE, GProfView))
#define GPROF_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_VIEW_TYPE, GProfViewClass))
#define GPROF_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPROF_VIEW_TYPE, GProfViewClass))
#define IS_GPROF_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_VIEW_TYPE))
#define IS_GPROF_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_VIEW_TYPE))

struct  _GProfView
{
	GObject parent;
	GProfViewPriv *priv;
};

struct _GProfViewClass
{
	GObjectClass parent_class;
	
	/* virtual methods */
	
	void (*refresh) (GProfView *self);
	GtkWidget * (*get_widget) (GProfView *self);
};

GType gprof_view_get_type (void);

void gprof_view_set_data (GProfView *self, GProfProfileData *profile_data);
void gprof_view_set_symbol_manager (GProfView *self, 
									IAnjutaSymbolManager *symbol_manager);
void gprof_view_set_document_manager (GProfView *self, 
									  IAnjutaDocumentManager *document_manager);
void gprof_view_show_symbol_in_editor (GProfView *self, 
									   const gchar *symbol_name);
GProfProfileData *gprof_view_get_data (GProfView *self);

void gprof_view_refresh (GProfView *self);
GtkWidget *gprof_view_get_widget (GProfView *self);

/* Static methods */
void gprof_view_format_float (GtkTreeViewColumn *col, 
							  GtkCellRenderer *renderer,
							  GtkTreeModel *model,
							  GtkTreeIter *iter,
							  gpointer column_number);

G_END_DECLS

#endif
