/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-provider.h
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

#ifndef __SNIPPETS_PROVIDER_H__
#define __SNIPPETS_PROVIDER_H__

#include <glib.h>
#include <glib-object.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include "snippets-db.h"
#include "snippets-interaction-interpreter.h"

G_BEGIN_DECLS

typedef struct _SnippetsProvider SnippetsProvider;
typedef struct _SnippetsProviderPrivate SnippetsProviderPrivate;
typedef struct _SnippetsProviderClass SnippetsProviderClass;

#define ANJUTA_TYPE_SNIPPETS_PROVIDER            (snippets_provider_get_type ())
#define ANJUTA_SNIPPETS_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPETS_PROVIDER, SnippetsProvider))
#define ANJUTA_SNIPPETS_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPETS_PROVIDER, SnippetsProviderClass))
#define ANJUTA_IS_SNIPPETS_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPETS_PROVIDER))
#define ANJUTA_IS_SNIPPETS_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPETS_PROVIDER))

struct _SnippetsProvider
{
	GObject parent;

	AnjutaShell *anjuta_shell;

};

struct _SnippetsProviderClass
{
	GObjectClass parent_class;

};


GType                snippets_provider_get_type (void) G_GNUC_CONST;
SnippetsProvider*    snippets_provider_new      (SnippetsDB *snippets_db,
                                                 SnippetsInteraction *snippets_interaction);
void                 snippets_provider_load     (SnippetsProvider *snippets_provider,
                                                 IAnjutaEditorAssist *editor_assist);
void                 snippets_provider_unload   (SnippetsProvider *snippets_provider);
void                 snippets_provider_request  (SnippetsProvider *snippets_provider);

G_END_DECLS

#endif /* __SNIPPETS_PROVIDER_H__ */

