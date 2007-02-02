/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-function-call-tree-view.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-function-call-tree-view.h is free software.
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

#ifndef _GPROF_FUNCTION_CALL_TREE_VIEW_H
#define _GPROF_FUNCTION_CALL_TREE_VIEW_H

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "gprof-view.h"

G_BEGIN_DECLS

typedef struct _GProfFunctionCallTreeView         GProfFunctionCallTreeView;
typedef struct _GProfFunctionCallTreeViewClass    GProfFunctionCallTreeViewClass;
typedef struct _GProfFunctionCallTreeViewPriv     GProfFunctionCallTreeViewPriv;

#define GPROF_FUNCTION_CALL_TREE_VIEW_TYPE            (gprof_function_call_tree_view_get_type ())
#define GPROF_FUNCTION_CALL_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_FUNCTION_CALL_TREE_VIEW_TYPE, GProfFunctionCallTreeView))
#define GPROF_FUNCTION_CALL_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_FUNCTION_CALL_TREE_VIEW_TYPE, GProfFunctionCallTreeViewClass))
#define IS_GPROF_FUNCTION_CALL_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_FUNCTION_CALL_TREE_VIEW_TYPE))
#define IS_GPROF_FUNCTION_CALL_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_FUNCTION_CALL_TREE_VIEW_TYPE))

struct  _GProfFunctionCallTreeView
{
	GProfView parent;
	GProfFunctionCallTreeViewPriv *priv;
};

struct _GProfFunctionCallTreeViewClass
{
	GProfViewClass parent_class;
};

GType gprof_flat_profile_view_get_type ();

GProfFunctionCallTreeView * gprof_function_call_tree_view_new (GProfProfileData *profile_data);

void gprof_function_call_tree_view_refresh (GProfView *view);
GtkWidget *gprof_function_call_tree_view_get_widget (GProfView *view);

#endif
