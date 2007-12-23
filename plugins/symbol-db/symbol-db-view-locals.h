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

#ifndef _SYMBOL_DB_VIEW_LOCALS_H_
#define _SYMBOL_DB_VIEW_LOCALS_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include "symbol-db-engine.h"

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_VIEW_LOCALS             (symbol_db_view_locals_get_type ())
#define SYMBOL_DB_VIEW_LOCALS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_VIEW_LOCALS, SymbolDBViewLocals))
#define SYMBOL_DB_VIEW_LOCALS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_VIEW_LOCALS, SymbolDBViewLocalsClass))
#define SYMBOL_IS_DB_VIEW_LOCALS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_VIEW_LOCALS))
#define SYMBOL_IS_DB_VIEW_LOCALS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_VIEW_LOCALS))
#define SYMBOL_DB_VIEW_LOCALS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_VIEW_LOCALS, SymbolDBViewLocalsClass))

typedef struct _SymbolDBViewLocalsClass SymbolDBViewLocalsClass;
typedef struct _SymbolDBViewLocals SymbolDBViewLocals;
typedef struct _SymbolDBViewLocalsPriv SymbolDBViewLocalsPriv;

struct _SymbolDBViewLocalsClass
{
	GtkTreeViewClass parent_class;
};

struct _SymbolDBViewLocals
{
	GtkTreeView parent_instance;
	SymbolDBViewLocalsPriv *priv;
};

GType symbol_db_view_locals_get_type (void) G_GNUC_CONST;

GtkWidget *
symbol_db_view_locals_new (void);

/*
 * filepath: db-relative file path, e.g. /src/file.c
 */
void
symbol_db_view_locals_update_list (SymbolDBViewLocals *dbvl, SymbolDBEngine *dbe,
							  const gchar* db_filepath);
gint
symbol_db_view_locals_get_line (SymbolDBViewLocals *dbvl,
								SymbolDBEngine *dbe,
								GtkTreeIter * iter);

/**
 * Enable or disable receiving signals from engine. Disabling this at project-load
 * time can avoid to receive hundreds (thousands?) of useless symbol-inserted.
 */
void
symbol_db_view_locals_recv_signals_from_engine (SymbolDBViewLocals *dbvl, 
							SymbolDBEngine *dbe, gboolean enable_status);


/**
 * Clear cache like GTree(s) and GtkTreeStore(s).
 */
void 
symbol_db_view_locals_clear_cache (SymbolDBViewLocals *dbvl);

G_END_DECLS

#endif /* _SYMBOL_DB_VIEW_LOCALS_H_ */
