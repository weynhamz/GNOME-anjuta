/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-flat-profile-view.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-flat-profile-view.h is free software.
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

#ifndef _GPROF_FLAT_PROFILE_VIEW_H
#define _GPROF_FLAT_PROFILE_VIEW_H

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "gprof-view.h"

G_BEGIN_DECLS

typedef struct _GProfFlatProfileView         GProfFlatProfileView;
typedef struct _GProfFlatProfileViewClass    GProfFlatProfileViewClass;
typedef struct _GProfFlatProfileViewPriv     GProfFlatProfileViewPriv;

#define GPROF_FLAT_PROFILE_VIEW_TYPE            (gprof_flat_profile_view_get_type ())
#define GPROF_FLAT_PROFILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_FLAT_PROFILE_VIEW_TYPE, GProfFlatProfileView))
#define GPROF_FLAT_PROFILE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_FLAT_PROFILE_VIEW_TYPE, GProfFlatProfileViewClass))
#define IS_GPROF_FLAT_PROFILE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_FLAT_PROFILE_VIEW_TYPE))
#define IS_GPROF_FLAT_PROFILE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_FLAT_PROFILE_VIEW_TYPE))

struct  _GProfFlatProfileView
{
	GProfView parent;
	GProfFlatProfileViewPriv *priv;
};

struct _GProfFlatProfileViewClass
{
	GProfViewClass parent_class;
};

GType gprof_flat_profile_view_get_type (void);

GProfFlatProfileView *gprof_flat_profile_view_new (GProfProfileData *profile_data,
												   IAnjutaSymbolManager *symbol_manager,
												   IAnjutaDocumentManager *document_manager);

void gprof_flat_profile_view_refresh (GProfView *view);
GtkWidget *gprof_flat_profile_view_get_widget (GProfView *view);

#endif
