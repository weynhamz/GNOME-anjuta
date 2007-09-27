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

#ifndef _SYMBOL_DB_ENGINE_ITERATOR_H_
#define _SYMBOL_DB_ENGINE_ITERATOR_H_

#include <glib-object.h>
#include <libgda/libgda.h>
#include "symbol-db-engine-iterator-node.h"

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_ENGINE_ITERATOR             (sdb_engine_iterator_get_type ())
#define SYMBOL_DB_ENGINE_ITERATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_ENGINE_ITERATOR, SymbolDBEngineIterator))
#define SYMBOL_DB_ENGINE_ITERATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_ENGINE_ITERATOR, SymbolDBEngineIteratorClass))
#define SYMBOL_IS_DB_ENGINE_ITERATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_ENGINE_ITERATOR))
#define SYMBOL_IS_DB_ENGINE_ITERATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_ENGINE_ITERATOR))
#define SYMBOL_DB_ENGINE_ITERATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_ENGINE_ITERATOR, SymbolDBEngineIteratorClass))

typedef struct _SymbolDBEngineIteratorClass SymbolDBEngineIteratorClass;
typedef struct _SymbolDBEngineIterator SymbolDBEngineIterator;
typedef struct _SymbolDBEngineIteratorPriv SymbolDBEngineIteratorPriv;

struct _SymbolDBEngineIteratorClass
{
	SymbolDBEngineIteratorNodeClass parent_class;
};

struct _SymbolDBEngineIterator
{
	SymbolDBEngineIteratorNode parent_instance;
	SymbolDBEngineIteratorPriv *priv;
};

GType sdb_engine_iterator_get_type (void) /*G_GNUC_CONST*/;


SymbolDBEngineIterator *
symbol_db_engine_iterator_new (GdaDataModel *model);

gboolean
symbol_db_engine_iterator_first (SymbolDBEngineIterator *dbi);

gboolean
symbol_db_engine_iterator_move_next (SymbolDBEngineIterator *dbi);

gboolean
symbol_db_engine_iterator_move_prev (SymbolDBEngineIterator *dbi);

gint 
symbol_db_engine_iterator_get_n_items (SymbolDBEngineIterator *dbi);

gboolean
symbol_db_engine_iterator_last (SymbolDBEngineIterator *dbi);

gboolean
symbol_db_engine_iterator_set_position (SymbolDBEngineIterator *dbi, gint pos);

gint
symbol_db_engine_iterator_get_position (SymbolDBEngineIterator *dbi);

void
symbol_db_engine_iterator_foreach (SymbolDBEngineIterator *dbi, GFunc callback, 
								   gpointer user_data);

G_END_DECLS

#endif /* _SYMBOL_DB_ENGINE_ITERATOR_H_ */
