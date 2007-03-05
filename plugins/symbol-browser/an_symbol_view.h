/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_symbol_view.h
 * Copyright (C) 2004 Naba Kumar
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef AN_SYMBOL_VIEW_H
#define AN_SYMBOL_VIEW_H

#include <gtk/gtk.h>
#include "an_symbol_info.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_SYMBOL_VIEW        (anjuta_symbol_view_get_type ())
#define ANJUTA_SYMBOL_VIEW(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_SYMBOL_VIEW, AnjutaSymbolView))
#define ANJUTA_SYMBOL_VIEW_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_SYMBOL_VIEW, AnjutaSymbolViewClass))
#define ANJUTA_IS_SYMBOL_VIEW(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_SYMBOL_VIEW))
#define ANJUTA_IS_SYMBOL_VIEW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_SYMBOL_VIEW))

typedef struct _AnjutaSymbolViewPriv AnjutaSymbolViewPriv;

typedef struct _AnjutaSymbolView
{
	GtkTreeView parent;
	AnjutaSymbolViewPriv *priv;
} AnjutaSymbolView;

typedef struct _AnjutaSymbolViewClass
{
	GtkTreeViewClass parent_class;
} AnjutaSymbolViewClass;

enum {
	COMPLETION_ACCESS_NONE,
	COMPLETION_ACCESS_DIRECT,
	COMPLETION_ACCESS_POINTER,
	COMPLETION_ACCESS_STATIC
};

enum
{
	COL_PIX, COL_NAME, COL_LINE, N_COLS
};

GType anjuta_symbol_view_get_type (void);
GtkWidget *anjuta_symbol_view_new (void);

void   anjuta_symbol_view_open (AnjutaSymbolView *sv, const gchar *dir);
void   anjuta_symbol_view_clear (AnjutaSymbolView *sv);
void   anjuta_symbol_view_update (AnjutaSymbolView *sv, GList *source_files);
void   anjuta_symbol_view_save (AnjutaSymbolView *sv);

GList* anjuta_symbol_view_get_node_expansion_states (AnjutaSymbolView *sv);
void   anjuta_symbol_view_set_node_expansion_states (AnjutaSymbolView *sv,
													 GList *expansion_states);

gchar* anjuta_symbol_view_get_current_symbol (AnjutaSymbolView *sv);

void anjuta_symbol_view_add_source (AnjutaSymbolView *sv, const gchar *filename);
void anjuta_symbol_view_remove_source (AnjutaSymbolView *sv, const gchar *filename);
void anjuta_symbol_view_update_source_from_buffer (AnjutaSymbolView *sv, const gchar *uri,
													gchar* text_buffer, gint buffer_size);
TMSourceFile *anjuta_symbol_view_get_tm_file (AnjutaSymbolView * sv, const gchar * uri);

/* expr: the expression you want to complete.
 * len: expr_len
 * func_scope_tag: the TMTag of the line into expr, which define the scope of the tag to return.
 *
 * returns the TMTag type of expression you want to complete. Usually it is a struct
 * a class or a union. 
 * returns the access_method detected which you wanna complete. 
 */
TMTag* anjuta_symbol_view_get_type_of_expression(AnjutaSymbolView * sv,
		const gchar* expr, int expr_len, const TMTag *func_scope_tag, gint *access_method);

/* TMTag* klass_tag: a tag for a struct/union/class
 * include_parents_tags: in the final array do you want to include the parent classes? Works
 *                       only with classes. 
 *
 * returns the public/protected members of parent classes within the ones of the class you
 *         passed as param.
 */
GPtrArray* anjuta_symbol_view_get_completable_members (TMTag* klass_tag, gboolean include_parents_tags);

/* Returns TRUE if file and line are updated */
gboolean anjuta_symbol_view_get_current_symbol_def (AnjutaSymbolView *sv,
													gchar** file,
													gint *line);
gboolean anjuta_symbol_view_get_current_symbol_decl (AnjutaSymbolView *sv,
													 gchar** file,
													 gint *line);
GtkTreeModel *anjuta_symbol_view_get_file_symbol_model (AnjutaSymbolView *sv);

void anjuta_symbol_view_workspace_add_file (AnjutaSymbolView *sv,
											const gchar *uri); 
void anjuta_symbol_view_workspace_remove_file (AnjutaSymbolView *sv,
											   const gchar *uri);
void anjuta_symbol_view_workspace_update_file (AnjutaSymbolView *sv,
											   const gchar *old_file_uri,
											   const gchar *new_file_uri);
							   
gint anjuta_symbol_view_workspace_get_line (AnjutaSymbolView *sv,
											GtkTreeIter *iter);
gboolean anjuta_symbol_view_get_file_symbol (AnjutaSymbolView *sv,
											 const gchar *symbol,
											 gboolean prefer_definition,
											 const gchar** const filename,
											 gint *line);

G_END_DECLS

#endif /* AN_SYMBOL_VIEW_H */
