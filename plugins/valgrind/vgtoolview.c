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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "vgtoolview.h"


static void vg_tool_view_class_init (VgToolViewClass *klass);
static void vg_tool_view_init (VgToolView *view);
static void vg_tool_view_destroy (GtkObject *obj);
static void vg_tool_view_finalize (GObject *obj);

static void tool_view_clear (VgToolView *view);
static void tool_view_reset (VgToolView *view);
static void tool_view_connect (VgToolView *view, int sockfd);
static int  tool_view_step (VgToolView *view);
static void tool_view_disconnect (VgToolView *view);
static int  tool_view_save_log (VgToolView *view, gchar* uri);
static int  tool_view_load_log (VgToolView *view, VgActions *actions, gchar* uri);
static void tool_view_cut (VgToolView *view);
static void tool_view_copy (VgToolView *view);
static void tool_view_paste (VgToolView *view);
static void tool_view_show_rules (VgToolView *view);


static GtkVBoxClass *parent_class = NULL;


GType
vg_tool_view_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgToolViewClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_tool_view_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgToolView),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_tool_view_init,
		};
		
		type = g_type_register_static (GTK_TYPE_VBOX, "VgToolView", &info, 0);
	}
	
	return type;
}

static void
vg_tool_view_class_init (VgToolViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_ref (GTK_TYPE_VBOX);
	
	object_class->finalize = vg_tool_view_finalize;
	gtk_object_class->destroy = vg_tool_view_destroy;
	
	/* virtual methods */
	klass->clear = tool_view_clear;
	klass->reset = tool_view_reset;
	klass->connect = tool_view_connect;
	klass->step = tool_view_step;
	klass->disconnect = tool_view_disconnect;
	klass->save_log = tool_view_save_log;
	klass->load_log = tool_view_load_log;
	klass->cut = tool_view_cut;
	klass->copy = tool_view_copy;
	klass->paste = tool_view_paste;
	klass->show_rules = tool_view_show_rules;
}

static void
vg_tool_view_init (VgToolView *view)
{
	view->argv = NULL;
	view->symtab = NULL;
	view->srcdir = NULL;
	view->rules = NULL;
}

static void
vg_tool_view_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_tool_view_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


void
vg_tool_view_set_argv (VgToolView *view, const char **argv)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	view->argv = argv;
}


void
vg_tool_view_set_srcdir (VgToolView *view, const char **srcdir)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	view->srcdir = srcdir;
}


void
vg_tool_view_set_symtab (VgToolView *view, SymTab *symtab)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	view->symtab = symtab;
}


static void
tool_view_clear (VgToolView *view)
{
	;
}


void
vg_tool_view_clear (VgToolView *view)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->clear (view);
}


static void
tool_view_reset (VgToolView *view)
{
	;
}


void
vg_tool_view_reset (VgToolView *view)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->reset (view);
}


static void
tool_view_connect (VgToolView *view, int sockfd)
{
	;
}


void
vg_tool_view_connect (VgToolView *view, int sockfd)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->connect (view, sockfd);
}


static int
tool_view_step (VgToolView *view)
{
	return -1;
}


int
vg_tool_view_step (VgToolView *view)
{
	g_return_val_if_fail (VG_IS_TOOL_VIEW (view), -1);
	
	return VG_TOOL_VIEW_GET_CLASS (view)->step (view);
}


static void
tool_view_disconnect (VgToolView *view)
{
	;
}


void
vg_tool_view_disconnect (VgToolView *view)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->disconnect (view);
}


static int
tool_view_save_log (VgToolView *view, gchar* uri)
{
	errno = ENOTSUP;
	return -1;
}


int
vg_tool_view_save_log (VgToolView *view, gchar *uri)
{
	g_return_val_if_fail (VG_IS_TOOL_VIEW (view), -1);
	
	return VG_TOOL_VIEW_GET_CLASS (view)->save_log (view, uri);
}


static int
tool_view_load_log (VgToolView *view, VgActions *actions, gchar* uri)
{
	errno = ENOTSUP;
	return -1;
}


int
vg_tool_view_load_log (VgToolView *view, VgActions *actions, gchar *uri)
{
	g_return_val_if_fail (VG_IS_TOOL_VIEW (view), -1);
	
	return VG_TOOL_VIEW_GET_CLASS (view)->load_log (view, actions, uri);
}


static void
tool_view_cut (VgToolView *view)
{
	;
}


void
vg_tool_view_cut (VgToolView *view)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->cut (view);
}


static void
tool_view_copy (VgToolView *view)
{
	;
}


void
vg_tool_view_copy (VgToolView *view)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->copy (view);
}


static void
tool_view_paste (VgToolView *view)
{
	;
}


void
vg_tool_view_paste (VgToolView *view)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->paste (view);
}


static void
tool_view_show_rules (VgToolView *view)
{
	if (view->rules == NULL)
		return;
	
	if (GTK_WIDGET_MAPPED (view->rules))
		gdk_window_raise (view->rules->window);
	else
		gtk_widget_show (view->rules);
}


void
vg_tool_view_show_rules (VgToolView *view)
{
	g_return_if_fail (VG_IS_TOOL_VIEW (view));
	
	VG_TOOL_VIEW_GET_CLASS (view)->show_rules (view);
}



static char *
path_concat (const char *dirname, int dirlen, const char *basename, int baselen)
{
	char *path, *p;
	
	p = path = g_malloc (dirlen + baselen + 2);
	memcpy (path, dirname, dirlen);
	p += dirlen;
	*p++ = '/';
	memcpy (p, basename, baselen);
	p[baselen] = '\0';
	
	return path;
}

static gboolean
path_is_rx (const char *path)
{
	struct stat st;
	
	if (stat (path, &st) != -1 && S_ISREG (st.st_mode)) {
		if (access (path, R_OK | X_OK) != -1)
			return TRUE;
	}
	
	return FALSE;
}


char *
vg_tool_view_scan_path (const char *program)
{
	const char *pathenv, *path, *p;
	char *filename;
	int len;
	
	if (program[0] == '/') {
		if (path_is_rx (program))
			return g_strdup (program);
		
		return NULL;
	}
	
	if (!(pathenv = getenv ("PATH")))
		return NULL;
	
	path = pathenv;
	len = strlen (program);
	while ((p = strchr (path, ':'))) {
		if (p > path) {
			filename = path_concat (path, (p - path), program, len);
			if (path_is_rx (filename))
				return filename;
			
			g_free (filename);
		}
		
		path = p + 1;
	}
	
	if (path[0] != '\0') {
		filename = g_strdup_printf ("%s/%s", path, program);
		if (path_is_rx (filename))
			return filename;
		
		g_free (filename);
	}
	
	return NULL;
}
