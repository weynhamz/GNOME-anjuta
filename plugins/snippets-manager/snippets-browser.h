/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-browser.h
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

#ifndef __SNIPPETS_BROWSER_H__
#define __SNIPPETS_BROWSER_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "snippets-db.h"
#include "snippet.h"
#include "snippets-interaction-interpreter.h"
#include <libanjuta/anjuta-shell.h>

G_BEGIN_DECLS

typedef struct _SnippetsBrowser SnippetsBrowser;
typedef struct _SnippetsBrowserPrivate SnippetsBrowserPrivate;
typedef struct _SnippetsBrowserClass SnippetsBrowserClass;

#define ANJUTA_TYPE_SNIPPETS_BROWSER            (snippets_browser_get_type ())
#define ANJUTA_SNIPPETS_BROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPETS_BROWSER, SnippetsBrowser))
#define ANJUTA_SNIPPETS_BROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPETS_BROWSER, SnippetsBrowserClass))
#define ANJUTA_IS_SNIPPETS_BROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPETS_BROWSER))
#define ANJUTA_IS_SNIPPETS_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPETS_BROWSER))

struct _SnippetsBrowser
{
	GtkHBox parent;

	gboolean show_only_document_language_snippets;
	AnjutaShell *anjuta_shell;

	/*< private >*/
	SnippetsBrowserPrivate *priv;
};

struct _SnippetsBrowserClass
{
	GtkHBoxClass parent_class;

	/* Signals */
	void (*maximize_request)          (SnippetsBrowser *snippets_browser);
	void (*unmaximize_request)        (SnippetsBrowser *snippets_browser);
};

G_END_DECLS


GType                  snippets_browser_get_type               (void) G_GNUC_CONST;
SnippetsBrowser*       snippets_browser_new	                   (void);

void                   snippets_browser_load                   (SnippetsBrowser *snippets_browser,
                                                                SnippetsDB *snippets_db,
                                                                SnippetsInteraction *snippets_interaction);
void                   snippets_browser_unload                 (SnippetsBrowser *snippets_browser);

void                   snippets_browser_show_editor            (SnippetsBrowser *snippets_browser);
void                   snippets_browser_hide_editor            (SnippetsBrowser *snippets_browser);
void                   snippets_browser_refilter_snippets_view (SnippetsBrowser *snippets_browser);

#endif /* __SNIPPETS_BROWSER_H__ */

