/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-function-call-chart-view.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-function-call-chart-view.h is free software.
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

#ifndef _GPROF_FUNCTION_CALL_CHART_VIEW_H
#define _GPROF_FUNCTION_CALL_CHART_VIEW_H

#include <config.h>
#include <graphviz/gvc.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "gprof-view.h"

G_BEGIN_DECLS

typedef struct _GProfFunctionCallChartView         GProfFunctionCallChartView;
typedef struct _GProfFunctionCallChartViewClass    GProfFunctionCallChartViewClass;
typedef struct _GProfFunctionCallChartViewPriv     GProfFunctionCallChartViewPriv;

#define GPROF_FUNCTION_CALL_CHART_VIEW_TYPE            (gprof_function_call_chart_view_get_type ())
#define GPROF_FUNCTION_CALL_CHART_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_FUNCTION_CALL_CHART_VIEW_TYPE, GProfFunctionCallChartView))
#define GPROF_FUNCTION_CALL_CHART_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_FUNCTION_CALL_CHART_VIEW_TYPE, GProfFunctionCallChartViewClass))
#define IS_GPROF_FUNCTION_CALL_CHART_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_FUNCTION_CALL_CHART_VIEW_TYPE))
#define IS_GPROF_FUNCTION_CALL_CHART_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_FUNCTION_CALL_CHART_VIEW_TYPE))

struct  _GProfFunctionCallChartView
{
	GProfView parent;
	GProfFunctionCallChartViewPriv *priv;
};

struct _GProfFunctionCallChartViewClass
{
	GProfViewClass parent_class;
};

GType gprof_function_call_chart_view_get_type (void);

GProfFunctionCallChartView *gprof_function_call_chart_view_new (GProfProfileData *profile_data,
																IAnjutaSymbolManager *symbol_manager,
																IAnjutaDocumentManager *document_manager);

void gprof_function_call_chart_view_refresh (GProfView *view);
GtkWidget *gprof_function_call_chart_view_get_widget (GProfView *view);

#endif
