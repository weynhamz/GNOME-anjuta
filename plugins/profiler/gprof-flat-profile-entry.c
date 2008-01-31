/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-flat-profile-entry.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-flat-profile-entry.c is free software.
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

#include "gprof-flat-profile-entry.h"

struct _GProfFlatProfileEntryPriv
{
	gfloat time_perc;  /* Percent of total execution */
	gfloat cum_sec;    /* Cumulative seconds spent on function and children */
	gfloat self_sec;   /* Time spent on this function only */
	guint calls;  /* Number of times called */
	gfloat avg_ms;     /* Average number of milliseconds spent by this call */
	gfloat total_ms;   /* Spent by function and children */
	gchar *name; /* Function name */
};

static void
gprof_flat_profile_entry_init (GProfFlatProfileEntry *self)
{
	self->priv = g_new0 (GProfFlatProfileEntryPriv, 1);
}

static void
gprof_flat_profile_entry_finalize (GObject *obj)
{
	GProfFlatProfileEntry *self;
	
	self = (GProfFlatProfileEntry *) obj;
	
	g_free (self->priv->name);
	g_free (self->priv);
}

static void 
gprof_flat_profile_entry_class_init (GProfFlatProfileEntryClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	
	object_class->finalize = gprof_flat_profile_entry_finalize;
}

GType
gprof_flat_profile_entry_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfFlatProfileEntryClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_flat_profile_entry_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfFlatProfileEntry),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_flat_profile_entry_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfFlatProfileEntry", &obj_info, 
											0);
	}
	return obj_type;
}

GProfFlatProfileEntry *
gprof_flat_profile_entry_new (gchar **fields)
{
	GProfFlatProfileEntry *entry;
	
	entry = g_object_new (GPROF_FLAT_PROFILE_ENTRY_TYPE, NULL);
	
	entry->priv->time_perc = g_ascii_strtod (fields[0], NULL);
	entry->priv->cum_sec = g_ascii_strtod (fields[1], NULL);
	entry->priv->self_sec = g_ascii_strtod (fields[2], NULL);
	entry->priv->calls = (unsigned) atoi (fields[3]);
	entry->priv->avg_ms = g_ascii_strtod (fields[4], NULL);
	entry->priv->total_ms = g_ascii_strtod (fields[5], NULL);
	entry->priv->name = g_strdup (fields[6]);

	return entry;	
}

void 
gprof_flat_profile_entry_free (GProfFlatProfileEntry *self)
{
	g_object_unref (self);
}

gfloat
gprof_flat_profile_entry_get_time_perc (GProfFlatProfileEntry *self)
{
	return self->priv->time_perc;
}

gfloat
gprof_flat_profile_entry_get_cum_sec (GProfFlatProfileEntry *self)
{
	return self->priv->cum_sec;
}

gfloat
gprof_flat_profile_entry_get_self_sec (GProfFlatProfileEntry *self)
{
	return self->priv->self_sec;
}

guint
gprof_flat_profile_entry_get_calls (GProfFlatProfileEntry *self)
{
	return self->priv->calls;
}

gfloat
gprof_flat_profile_entry_get_avg_ms (GProfFlatProfileEntry *self)
{
	return self->priv->avg_ms;
}

gfloat
gprof_flat_profile_entry_get_total_ms (GProfFlatProfileEntry *self)
{
	return self->priv->total_ms;
}

gchar *
gprof_flat_profile_entry_get_name (GProfFlatProfileEntry *self)
{
	return self->priv->name;
}

GProfFlatProfileEntry *
gprof_flat_profile_entry_get_next (GList *current_iter, GList **next_iter)
{
	*next_iter = g_list_next (current_iter);
	
	if (*next_iter)
		return GPROF_FLAT_PROFILE_ENTRY ((*next_iter)->data);
	else 
		return NULL;
}
