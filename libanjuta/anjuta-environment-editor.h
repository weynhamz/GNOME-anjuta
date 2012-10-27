/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-environment-editor.h
 * Copyright (C) 2011 Sebastien Granjoux
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _ANJUTA_ENVIRONMENT_EDITOR_H_
#define _ANJUTA_ENVIRONMENT_EDITOR_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_ENVIRONMENT_EDITOR             (anjuta_environment_editor_get_type ())
#define ANJUTA_ENVIRONMENT_EDITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_ENVIRONMENT_EDITOR, AnjutaEnvironmentEditor))
#define ANJUTA_ENVIRONMENT_EDITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_ENVIRONMENT_EDITOR, AnjutaEnvironmentEditorClass))
#define ANJUTA_IS_ENVIRONMENT_EDITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_ENVIRONMENT_EDITOR))
#define ANJUTA_IS_ENVIRONMENT_EDITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_ENVIRONMENT_EDITOR))
#define ANJUTA_ENVIRONMENT_EDITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_ENVIRONMENT_EDITOR, AnjutaEnvironmentEditorClass))

typedef struct _AnjutaEnvironmentEditorClass AnjutaEnvironmentEditorClass;
typedef struct _AnjutaEnvironmentEditor AnjutaEnvironmentEditor;

struct _AnjutaEnvironmentEditorClass
{
	GtkBinClass parent_class;
	
	/* Signals */
	void(* changed) (AnjutaEnvironmentEditor *self);
};

GType anjuta_environment_editor_get_type (void) G_GNUC_CONST;

GtkWidget* anjuta_environment_editor_new (void);
void anjuta_environment_editor_set_variable (AnjutaEnvironmentEditor *editor, const gchar *variable);
gchar** anjuta_environment_editor_get_all_variables (AnjutaEnvironmentEditor *editor);
gchar** anjuta_environment_editor_get_modified_variables (AnjutaEnvironmentEditor *editor);
void anjuta_environment_editor_reset (AnjutaEnvironmentEditor *editor);

G_END_DECLS

#endif /* _ANJUTA_ENVIRONMENT_EDITOR_H_ */
