/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007-2008 <maxcvs@email.it>
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

#ifndef _SYMBOL_DB_VIEW_SEARCH_H_
#define _SYMBOL_DB_VIEW_SEARCH_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include "symbol-db-engine.h"

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_VIEW_SEARCH             (sdb_view_search_get_type ())
#define SYMBOL_DB_VIEW_SEARCH(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_VIEW_SEARCH, SymbolDBViewSearch))
#define SYMBOL_DB_VIEW_SEARCH_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_VIEW_SEARCH, SymbolDBViewSearchClass))
#define SYMBOL_IS_DB_VIEW_SEARCH(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_VIEW_SEARCH))
#define SYMBOL_IS_DB_VIEW_SEARCH_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_VIEW_SEARCH))
#define SYMBOL_DB_VIEW_SEARCH_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_VIEW_SEARCH, SymbolDBViewSearchClass))

typedef struct _SymbolDBViewSearchPriv SymbolDBViewSearchPriv;
typedef struct _SymbolDBViewSearchClass SymbolDBViewSearchClass;
typedef struct _SymbolDBViewSearch SymbolDBViewSearch;

struct _SymbolDBViewSearch
{
    GtkVBox parent;
	
	SymbolDBViewSearchPriv *priv;
};

struct _SymbolDBViewSearchClass
{
    GtkVBoxClass   parent_class;	
	
	/* Signals */
    void (*symbol_selected) (SymbolDBViewSearch *search, 
							 gint line, gchar *file);
};


GType sdb_view_search_get_type (void) G_GNUC_CONST;

void 
symbol_db_view_search_clear (SymbolDBViewSearch *search);

GtkWidget *
symbol_db_view_search_new (SymbolDBEngine *dbe);


G_END_DECLS

#endif /* _SYMBOL_DB_VIEW_SEARCH_H_ */
