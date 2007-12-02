/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-flat-profile.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-flat-profile.h is free software.
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

#ifndef _GPROF_FLAT_PROFILE_H
#define _GPROF_FLAT_PROFILE_H

#include <glib.h>
#include <glib-object.h>
#include <stdio.h>
#include <string.h>
#include "gprof-flat-profile-entry.h"
#include "string-utils.h"

G_BEGIN_DECLS

typedef struct _GProfFlatProfile         GProfFlatProfile;
typedef struct _GProfFlatProfileClass    GProfFlatProfileClass;
typedef struct _GProfFlatProfilePriv     GProfFlatProfilePriv;

#define GPROF_FLAT_PROFILE_TYPE            (gprof_flat_profile_get_type ())
#define GPROF_FLAT_PROFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_FLAT_PROFILE_TYPE, GProfFlatProfile))
#define GPROF_FLAT_PROFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_FLAT_PROFILE_TYPE, GProfFlatProfileClass))
#define IS_GPROF_FLAT_PROFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_FLAT_PROFILE_TYPE))
#define IS_GPROF_FLAT_PROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_FLAT_PROFILE_TYPE))

struct  _GProfFlatProfile
{
	GObject parent;
	GProfFlatProfilePriv *priv;
};

struct _GProfFlatProfileClass
{
	GObjectClass parent_class;
};

GType gprof_flat_profile_get_type (void);
GProfFlatProfile *gprof_flat_profile_new (FILE *stream);
void gprof_flat_profile_free (GProfFlatProfile *self);

GProfFlatProfileEntry *gprof_flat_profile_get_first_entry (GProfFlatProfile *self,
														   GList **iter);
GProfFlatProfileEntry *gprof_flat_profile_find_entry (GProfFlatProfile* self,
													  const gchar *name);
void gprof_flat_profile_dump (GProfFlatProfile *self, FILE *stream);

G_END_DECLS

#endif
