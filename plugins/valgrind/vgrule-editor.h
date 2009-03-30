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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __VG_RULE_EDITOR_H__
#define __VG_RULE_EDITOR_H__

#include <gtk/gtk.h>

#include "vgerror.h"
#include "vgrule.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_RULE_EDITOR            (vg_rule_editor_get_type ())
#define VG_RULE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_RULE_EDITOR, VgRuleEditor))
#define VG_RULE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_RULE_EDITOR, VgRuleEditorClass))
#define IS_VG_RULE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_RULE_EDITOR))
#define IS_VG_RULE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_RULE_EDITOR))
#define VG_RULE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_RULE_EDITOR, VgRuleEditorClass))

typedef struct _VgRuleEditor VgRuleEditor;
typedef struct _VgRuleEditorClass VgRuleEditorClass;

struct _VgRuleEditor {
	GtkVBox parent_object;
	
	GtkEntry *name;
	GtkComboBox *type;
	GtkEntry *syscall;
	
	GtkToggleButton *addrcheck;
	GtkToggleButton *memcheck;
	
	GPtrArray *callers;
	GtkBox *call_stack;
};

struct _VgRuleEditorClass {
	GtkVBoxClass parent_class;
	
};


GType vg_rule_editor_get_type (void);

GtkWidget *vg_rule_editor_new (void);
GtkWidget *vg_rule_editor_new_from_rule (VgRule *rule);
GtkWidget *vg_rule_editor_new_from_summary (VgErrorSummary *summary);

const char *vg_rule_editor_get_name (VgRuleEditor *editor);
void vg_rule_editor_set_name (VgRuleEditor *editor, const char *name);

void vg_rule_editor_set_type (VgRuleEditor *editor, vgrule_t type);
void vg_rule_editor_set_syscall (VgRuleEditor *editor, const char *syscall);
void vg_rule_editor_add_caller (VgRuleEditor *editor, vgcaller_t type, const char *name);

VgRule *vg_rule_editor_get_rule (VgRuleEditor *editor);

void vg_rule_editor_save (VgRuleEditor *editor, const char *filename);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_RULE_EDITOR_H__ */
