/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-profile-data.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-profile-data.h is free software.
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

#ifndef _GPROF_PROFILE_DATA_H
#define _GPROF_PROFILE_DATA_H

#include <glib-object.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgnomevfs/gnome-vfs.h>
#include "gprof-flat-profile.h"
#include "gprof-call-graph.h"

G_BEGIN_DECLS

typedef struct _GProfProfileData         GProfProfileData;
typedef struct _GProfProfileDataClass    GProfProfileDataClass;
typedef struct _GProfProfileDataPriv     GProfProfileDataPriv;

#define GPROF_PROFILE_DATA_TYPE            (gprof_profile_data_get_type ())
#define GPROF_PROFILE_DATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_PROFILE_DATA_TYPE, GProfProfileData))
#define GPROF_PROFILE_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_PROFILE_DATA_TYPE, GProfProfileDataClass))
#define IS_GPROF_PROFILE_DATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_PROFILE_DATA_TYPE))
#define IS_GPROF_PROFILE_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_PROFILE_DATA_TYPE))

struct  _GProfProfileData
{
	GObject parent;
	GProfProfileDataPriv *priv;
};

struct _GProfProfileDataClass
{
	GObjectClass parent_class;
};

GType gprof_profile_data_get_type (void);
GProfProfileData *gprof_profile_data_new (void);
void gprof_profile_data_free (GProfProfileData *self);

gboolean gprof_profile_data_init_profile (GProfProfileData *self, gchar *path,
										  gchar *alternate_profile_data_path,
									  	  GPtrArray *options);

GProfFlatProfile *gprof_profile_data_get_flat_profile (GProfProfileData *self);
GProfCallGraph *gprof_profile_data_get_call_graph (GProfProfileData *self);
gboolean gprof_profile_data_has_data (GProfProfileData *self);

G_END_DECLS

#endif
