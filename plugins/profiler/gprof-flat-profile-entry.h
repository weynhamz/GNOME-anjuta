/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-flat-profile-entry.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-flat-profile-entry.h is free software.
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

#ifndef _GPROF_FLAT_PROFILE_ENTRY
#define _GPROF_FLAT_PROFILE_ENTRY

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>

G_BEGIN_DECLS

typedef struct _GProfFlatProfileEntry         GProfFlatProfileEntry;
typedef struct _GProfFlatProfileEntryClass    GProfFlatProfileEntryClass;
typedef struct _GProfFlatProfileEntryPriv     GProfFlatProfileEntryPriv;

#define GPROF_FLAT_PROFILE_ENTRY_TYPE            (gprof_flat_profile_entry_get_type ())
#define GPROF_FLAT_PROFILE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_FLAT_PROFILE_ENTRY_TYPE, GProfFlatProfileEntry))
#define GPROF_FLAT_PROFILE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_FLAT_PROFILE_ENTRY_TYPE, GProfFlatProfileEntryClass))
#define IS_GPROF_FLAT_PROFILE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_FLAT_PROFILE_ENTRY_TYPE))
#define IS_GPROF_FLAT_PROFILE_CLASS_ENTRY(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_FLAT_PROFILE_ENTRY_TYPE))

struct  _GProfFlatProfileEntry
{
	GObject parent;
	GProfFlatProfileEntryPriv *priv;
};

struct _GProfFlatProfileEntryClass
{
	GObjectClass parent_class;
};

GType gprof_flat_profile_entry_get_type (void);

GProfFlatProfileEntry *gprof_flat_profile_entry_new (gchar **fields);
void gprof_flat_profile_entry_free (GProfFlatProfileEntry *self);

gfloat gprof_flat_profile_entry_get_time_perc (GProfFlatProfileEntry *self);
gfloat gprof_flat_profile_entry_get_cum_sec (GProfFlatProfileEntry *self);
gfloat gprof_flat_profile_entry_get_self_sec (GProfFlatProfileEntry *self);
guint gprof_flat_profile_entry_get_calls (GProfFlatProfileEntry *self);
gfloat gprof_flat_profile_entry_get_avg_ms (GProfFlatProfileEntry *self);
gfloat gprof_flat_profile_entry_get_total_ms (GProfFlatProfileEntry *self);
gchar *gprof_flat_profile_entry_get_name (GProfFlatProfileEntry *self);

GProfFlatProfileEntry *gprof_flat_profile_entry_get_next (GList *current_iter,
														  GList **next_iter);

G_END_DECLS

#endif
