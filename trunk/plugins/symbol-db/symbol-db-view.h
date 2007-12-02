/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007 <maxcvs@email.it>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _SYMBOL_DB_VIEW_H_
#define _SYMBOL_DB_VIEW_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include "symbol-db-engine.h"
#include "plugin.h"

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_VIEW             (symbol_db_view_get_type ())
#define SYMBOL_DB_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_VIEW, SymbolDBView))
#define SYMBOL_DB_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_VIEW, SymbolDBViewClass))
#define SYMBOL_IS_DB_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_VIEW))
#define SYMBOL_IS_DB_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_VIEW))
#define SYMBOL_DB_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_VIEW, SymbolDBViewClass))

typedef struct _SymbolDBViewClass SymbolDBViewClass;
typedef struct _SymbolDBView SymbolDBView;
typedef struct _SymbolDBViewPriv SymbolDBViewPriv;

struct _SymbolDBViewClass
{
	GtkTreeViewClass parent_class;
};

struct _SymbolDBView
{
	GtkTreeView parent_instance;
	SymbolDBViewPriv *priv;
};

GType symbol_db_view_get_type (void) G_GNUC_CONST;

GtkWidget* 
symbol_db_view_new (void);

/* return the pixbufs. It will initialize pixbufs first if they weren't before
 * node_access: can be NULL.
 */
const GdkPixbuf*
symbol_db_view_get_pixbuf  (const gchar *node_type, const gchar *node_access);

void 
symbol_db_view_open (SymbolDBView *dbv, SymbolDBEngine *dbe);

void
symbol_db_view_row_expanded (SymbolDBView *dbv, SymbolDBEngine *dbe, 
							 GtkTreeIter *iter);

gboolean
symbol_db_view_get_file_and_line (SymbolDBView *dbv, SymbolDBEngine *dbe,
							GtkTreeIter * iter, gint *OUT_line, gchar **OUT_file);

G_END_DECLS

#endif /* _SYMBOL_DB_VIEW_H_ */
