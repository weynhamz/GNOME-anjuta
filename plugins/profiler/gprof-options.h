/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-options.h
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 * 
 * gprof-options.h is free software.
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

#ifndef _GPROF_OPTIONS_H
#define _GPROF_OPTIONS_H

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

G_BEGIN_DECLS

typedef struct _GProfOptions         GProfOptions;
typedef struct _GProfOptionsClass    GProfOptionsClass;
typedef struct _GProfOptionsPriv     GProfOptionsPriv;

#define GPROF_OPTIONS_TYPE            (gprof_options_get_type ())
#define GPROF_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_OPTIONS_TYPE, GProfOptions))
#define GPROF_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_OPTIONS_TYPE, GProfOptionsClass))
#define GPROF_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPROF_OPTIONS_TYPE, GProfOptionsClass))
#define IS_GPROF_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_OPTIONS_TYPE))
#define IS_GPROF_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_OPTIONS_TYPE))

struct  _GProfOptions
{
	GObject parent;
	GProfOptionsPriv *priv;
};

struct _GProfOptionsClass
{
	GObjectClass parent_class;
};

/* Option types (used for widget synchronization */
typedef enum
{
	OPTION_TYPE_TOGGLE,
	OPTION_TYPE_TEXT_ENTRY,
	OPTION_TYPE_ENTRY
} OptionWidgetType;

GType gprof_options_get_type (void);

GProfOptions *gprof_options_new ();
void gprof_options_destroy (GProfOptions *self);

void gprof_options_set_target (GProfOptions *self, gchar *target);

gchar *gprof_options_get_string (GProfOptions *self, const gchar *key);
gint gprof_options_get_int (GProfOptions *self, const gchar *key);


void gprof_options_set_string (GProfOptions *self, gchar *key, gchar *value);
void gprof_options_set_int (GProfOptions *self, gchar *key, gint value);

void gprof_options_register_key (GProfOptions *self, gchar *key_name, 
								 const gchar *default_value, 
								 const gchar *widget_name, 
								 OptionWidgetType widget_type);


void gprof_options_create_window (GProfOptions *self, GladeXML *gxml);

void gprof_options_load (GProfOptions *self, const gchar *path);
void gprof_options_save (GProfOptions *self, const gchar *path);



G_END_DECLS

#endif /* _GPROF_OPTIONS_H */

 
