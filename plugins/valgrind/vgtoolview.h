/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
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


#ifndef __VG_TOOL_VIEW_H__
#define __VG_TOOL_VIEW_H__

#include <gtk/gtk.h>

#include "symtab.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_TOOL_VIEW            (vg_tool_view_get_type ())
#define VG_TOOL_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_TOOL_VIEW, VgToolView))
#define VG_TOOL_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_TOOL_VIEW, VgToolViewClass))
#define VG_IS_TOOL_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_TOOL_VIEW))
#define VG_IS_TOOL_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_TOOL_VIEW))
#define VG_TOOL_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_TOOL_VIEW, VgToolViewClass))

typedef struct _VgToolView VgToolView;
typedef struct _VgToolViewClass VgToolViewClass;

#include "vgactions.h"

struct _VgToolView {
	GtkVBox parent_object;
	
	const char **argv;    /* args of program to debug */
	const char **srcdir;  /* list src dir to check for src files */

	GPtrArray *argv_array;
	GPtrArray *srcdir_array;
	
	SymTab *symtab;
	
	GtkWidget *rules;
};

struct _VgToolViewClass {
	GtkVBoxClass parent_class;
	
	/* virtual methods */
	
	/* state methods */
	void (* clear)   (VgToolView *view);               /* clears the display (implies a disconnect if needed) */
	void (* reset)   (VgToolView *view);               /* resets the state */
	void (* connect) (VgToolView *view, int sockfd);   /* connect to valgrind's --logfile-fd */
	int  (* step)    (VgToolView *view);               /* take 1 parse step over valgrind's output stream */
	void (* disconnect) (VgToolView *view);            /* disconnect from valgrind's --logfile-fd */
	
	int (* save_log) (VgToolView *view, gchar* uri);
	int (* load_log) (VgToolView *view, VgActions *actions, gchar* uri);
	
	/* cut/copy/paste methods */
	void (* cut) (VgToolView *view);
	void (* copy) (VgToolView *view);
	void (* paste) (VgToolView *view);
	
	/* other methods */
	void (* show_rules) (VgToolView *view);            /* show suppression rules dialog */
};


GType vg_tool_view_get_type (void);

void vg_tool_view_set_argv (VgToolView *view, char *arg0, ...);
void vg_tool_view_set_srcdir (VgToolView *view, char *srcdir0, ...);
void vg_tool_view_set_symtab (VgToolView *view, SymTab *symtab);

void vg_tool_view_clear (VgToolView *view);
void vg_tool_view_reset (VgToolView *view);
void vg_tool_view_connect (VgToolView *view, int sockfd);
int  vg_tool_view_step (VgToolView *view);
void vg_tool_view_disconnect (VgToolView *view);

int vg_tool_view_save_log (VgToolView *view, gchar*uri);
int vg_tool_view_load_log (VgToolView *view, VgActions *actions, gchar *uri);

void vg_tool_view_cut (VgToolView *view);
void vg_tool_view_copy (VgToolView *view);
void vg_tool_view_paste (VgToolView *view);

void vg_tool_view_show_rules (VgToolView *view);

char *vg_tool_view_scan_path (const char *program);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_TOOL_VIEW_H__ */
