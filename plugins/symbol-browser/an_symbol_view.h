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

GType anjuta_symbol_view_get_type (void);
GtkWidget *anjuta_symbol_view_new (void);

void   anjuta_symbol_view_open (AnjutaSymbolView *sv, const gchar *dir);
void   anjuta_symbol_view_clear (AnjutaSymbolView *sv);
void   anjuta_symbol_view_update (AnjutaSymbolView *sv, GList *source_files);
void   anjuta_symbol_view_save (AnjutaSymbolView *sv);

GList* anjuta_symbol_view_get_node_expansion_states (AnjutaSymbolView *sv);
void   anjuta_symbol_view_set_node_expansion_states (AnjutaSymbolView *sv,
													 GList *expansion_states);

G_CONST_RETURN gchar* anjuta_symbol_view_get_current_symbol (AnjutaSymbolView *sv);

void anjuta_symbol_view_add_source (AnjutaSymbolView *sv, const gchar *filename);
void anjuta_symbol_view_remove_source (AnjutaSymbolView *sv, const gchar *filename);

/* Returns TRUE if file and line are updated */
gboolean anjuta_symbol_view_get_current_symbol_def (AnjutaSymbolView *sv,
													const gchar** const file,
													gint *line);
gboolean anjuta_symbol_view_get_current_symbol_decl (AnjutaSymbolView *sv,
													 const gchar** const file,
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
/* Caller does not get a reference */
GdkPixbuf *anjuta_symbol_view_get_pixbuf (AnjutaSymbolView *sv,
										  SVNodeType node_type);

G_END_DECLS

#endif /* AN_SYMBOL_VIEW_H */
