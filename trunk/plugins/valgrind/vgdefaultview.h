/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@gmail.com>  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __VG_DEFAULT_VIEW_H__
#define __VG_DEFAULT_VIEW_H__

#include <gtk/gtk.h>

#include <gconf/gconf-client.h>

#include <sys/types.h>
#include <regex.h>

#include "vgerror.h"
#include "symtab.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_DEFAULT_VIEW            (vg_default_view_get_type ())
#define VG_DEFAULT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_DEFAULT_VIEW, VgDefaultView))
#define VG_DEFAULT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_DEFAULT_VIEW, VgDefaultViewClass))
#define VG_IS_DEFAULT_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_DEFAULT_VIEW))
#define VG_IS_DEFAULT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_DEFAULT_VIEW))
#define VG_DEFAULT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_DEFAULT_VIEW, VgDefaultViewClass))

typedef struct _VgDefaultView VgDefaultView;
typedef struct _VgDefaultViewClass VgDefaultViewClass;

#include "vgtoolview.h"
#include "plugin.h"

struct _VgDefaultView {
	VgToolView parent_object;
	
	GConfClient *gconf;
	
	GtkWidget *table;
	GtkWidget *rule_list;
	
	GPtrArray *errors;
	VgErrorParser *parser;
	
	GPtrArray *suppressions;
	
	int search_id;
	regex_t search_regex;
	
	guint rules_id;
	
	int srclines;
	guint lines_id;
	
	AnjutaValgrindPlugin *valgrind_plugin;
};

struct _VgDefaultViewClass {
	VgToolViewClass parent_class;
	
};


GType vg_default_view_get_type (void);

GtkWidget *vg_default_view_new (AnjutaValgrindPlugin *valgrind_plugin);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_DEFAULT_VIEW_H__ */
