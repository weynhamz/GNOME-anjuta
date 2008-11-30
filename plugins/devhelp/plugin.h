/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Johannes Schmid 2006 <jhs@cvs.gnome.org>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _DEVHELP_H_
#define _DEVHELP_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#ifndef DISABLE_EMBEDDED_DEVHELP
#include <devhelp/dh-base.h>
#endif /* DISABLE_EMBEDDED_DEVHELP */

extern GType devhelp_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_DEVHELP         (devhelp_get_type (NULL))
#define ANJUTA_PLUGIN_DEVHELP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_DEVHELP, AnjutaDevhelp))
#define ANJUTA_PLUGIN_DEVHELP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_DEVHELP, AnjutaDevhelpClass))
#define ANJUTA_IS_PLUGIN_DEVHELP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_DEVHELP))
#define ANJUTA_IS_PLUGIN_DEVHELP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_DEVHELP))
#define ANJUTA_PLUGIN_DEVHELP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_DEVHELP, AnjutaDevhelpClass))

typedef struct _AnjutaDevhelp AnjutaDevhelp;
typedef struct _AnjutaDevhelpClass AnjutaDevhelpClass;

struct _AnjutaDevhelp{
	AnjutaPlugin parent;
	
#ifndef DISABLE_EMBEDDED_DEVHELP
	DhBase         *base;
#endif /* DISABLE_EMBEDDED_DEVHELP */
	GtkWidget      *view;
	GtkWidget      *view_sw;
	GtkWidget      *control_notebook; 
	GtkWidget      *main_vbox;
	GtkWidget      *book_tree;
	GtkWidget      *search;
	GtkWidget      *go_back;
	GtkWidget      *go_forward;
	GtkWidget      *online;
	
	IAnjutaEditor	*editor;
	guint editor_watch_id;
	
	GtkActionGroup* action_group;
	gint uiid;
};

struct _AnjutaDevhelpClass{
	AnjutaPluginClass parent_class;
};

void anjuta_devhelp_check_history(AnjutaDevhelp* devhelp);

#endif
