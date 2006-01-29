/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
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


#ifndef __VG_RULE_LIST_H__
#define __VG_RULE_LIST_H__

#include <gtk/gtk.h>

#include "vgrule-editor.h"

#include "list.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_RULE_LIST            (vg_rule_list_get_type ())
#define VG_RULE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_RULE_LIST, VgRuleList))
#define VG_RULE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_RULE_LIST, VgRuleListClass))
#define IS_VG_RULE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_RULE_LIST))
#define IS_VG_RULE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_RULE_LIST))
#define VG_RULE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_RULE_LIST, VgRuleListClass))

typedef struct _VgRuleList VgRuleList;
typedef struct _VgRuleListClass VgRuleListClass;

struct _VgRuleList {
	GtkVBox parent_object;
	
	GtkTreeModel *model;
	GtkWidget *list;
	
	List rules;
	
	GtkWidget *add;
	GtkWidget *edit;
	GtkWidget *remove;
	
	char *filename;
	VgRuleParser *parser;
	GIOChannel *gio;
	guint show_id;
	guint load_id;
	
	gboolean changed;
};

struct _VgRuleListClass {
	GtkVBoxClass parent_class;
	
	/* signals */
	
	void (* rule_added) (VgRuleList *list, VgRule *rule);
};

GType vg_rule_list_get_type (void);
GType vg_rule_list_get_class (void);

GtkWidget *vg_rule_list_new (const char *filename);

void vg_rule_list_set_filename (VgRuleList *list, const char *filename);

int vg_rule_list_save (VgRuleList *list);


/* interface to get add a rule from another window */
void vg_rule_list_add_rule (VgRuleList *list, const char *title, GtkWindow *parent,
			    VgErrorSummary *summary);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_RULE_LIST__ */
