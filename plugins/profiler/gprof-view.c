/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-view.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-view.c is free software.
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

#include "gprof-view.h"

struct _GProfViewPriv
{
	GProfProfileData *profile_data;
};

static void
gprof_view_init (GProfView *self)
{
	self->priv = g_new0 (GProfViewPriv, 1);
}

static void
gprof_view_finalize (GObject *obj)
{
	GProfView *self;
	
	self = (GProfView *) obj;
	
	gprof_profile_data_free (self->priv->profile_data);
	g_free(self->priv);
}

static void 
gprof_view_class_init (GProfViewClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	object_class->finalize = gprof_view_finalize;
	
	klass->refresh = NULL;
	klass->get_widget = NULL;
}

GType 
gprof_view_get_type ()
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_view_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfView", &obj_info, 0);
	}
	return obj_type;
}

void
gprof_view_set_data (GProfView *self, GProfProfileData *profile_data)
{
	self->priv->profile_data = g_object_ref (profile_data);
}

GProfProfileData *
gprof_view_get_data (GProfView *self)
{
	return self->priv->profile_data;
}

void 
gprof_view_refresh (GProfView *self)
{
	/* Don't refresh views if we don't have any data to work with */
	if (gprof_profile_data_has_data (self->priv->profile_data))
		GPROF_VIEW_GET_CLASS (self)->refresh (self);
}

GtkWidget *
gprof_view_get_widget (GProfView *self)
{
	return GPROF_VIEW_GET_CLASS (self)->get_widget (self);
}
