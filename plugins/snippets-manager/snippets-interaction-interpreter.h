/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-interaction-interpreter.h
    Copyright (C) Dragos Dena 2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#ifndef __SNIPPETS_INTERACTION_H__
#define __SNIPPETS_INTERACTION_H__

#include <glib.h>
#include <glib-object.h>
#include "snippet.h"
#include "snippets-db.h"
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

G_BEGIN_DECLS

typedef struct _SnippetsInteraction SnippetsInteraction;
typedef struct _SnippetsInteractionPrivate SnippetsInteractionPrivate;
typedef struct _SnippetsInteractionClass SnippetsInteractionClass;

#define ANJUTA_TYPE_SNIPPETS_INTERACTION            (snippets_interaction_get_type ())
#define ANJUTA_SNIPPETS_INTERACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPETS_INTERACTION, SnippetsInteraction))
#define ANJUTA_SNIPPETS_INTERACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPETS_INTERACTION, SnippetsInteractionClass))
#define ANJUTA_IS_SNIPPETS_INTERACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPETS_INTERACTION))
#define ANJUTA_IS_SNIPPETS_INTERACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPETS_INTERACTION))

struct _SnippetsInteraction
{
	GObject object;

};

struct _SnippetsInteractionClass
{
	GObjectClass klass;
	
};

GType                snippets_interaction_get_type               (void) G_GNUC_CONST;
SnippetsInteraction* snippets_interaction_new                    (void);

void                 snippets_interaction_start                  (SnippetsInteraction *snippets_interaction,
                                                                  AnjutaShell *shell);
void                 snippets_interaction_destroy                (SnippetsInteraction *snippets_interaction);
void                 snippets_interaction_insert_snippet         (SnippetsInteraction *snippets_interaction,
                                                                  SnippetsDB *snippets_db,
                                                                  AnjutaSnippet *snippet);
void                 snippets_interaction_trigger_insert_request (SnippetsInteraction *snippets_interaction,
                                                                  SnippetsDB *snippets_db);
void                 snippets_interaction_set_editor              (SnippetsInteraction *snippets_interaction,
                                                                   IAnjutaEditor *editor);

G_END_DECLS

#endif /* __SNIPPETS_INTERACTION_H__ */
