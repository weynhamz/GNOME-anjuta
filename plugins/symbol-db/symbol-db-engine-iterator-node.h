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

#ifndef _SYMBOL_DB_ENGINE_ITERATOR_NODE_H_
#define _SYMBOL_DB_ENGINE_ITERATOR_NODE_H_

#include <glib-object.h>
#include <libgda/libgda.h>

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE             (sdb_engine_iterator_node_get_type ())
#define SYMBOL_DB_ENGINE_ITERATOR_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE, SymbolDBEngineIteratorNode))
#define SYMBOL_DB_ENGINE_ITERATOR_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE, SymbolDBEngineIteratorNodeClass))
#define SYMBOL_IS_DB_ENGINE_ITERATOR_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE))
#define SYMBOL_IS_DB_ENGINE_ITERATOR_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE))
#define SYMBOL_DB_ENGINE_ITERATOR_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE, SymbolDBEngineIteratorNodeClass))

typedef struct _SymbolDBEngineIteratorNodeClass SymbolDBEngineIteratorNodeClass;
typedef struct _SymbolDBEngineIteratorNode SymbolDBEngineIteratorNode;
typedef struct _SymbolDBEngineIteratorNodePriv SymbolDBEngineIteratorNodePriv;

struct _SymbolDBEngineIteratorNode
{
	GObject parent_instance;
	
	SymbolDBEngineIteratorNodePriv *priv;
};

struct _SymbolDBEngineIteratorNodeClass
{
	GObjectClass parent_class;
};


GType sdb_engine_iterator_node_get_type (void) G_GNUC_CONST;

gint
symbol_db_engine_iterator_node_get_symbol_id (SymbolDBEngineIteratorNode *dbin);

const gchar* 
symbol_db_engine_iterator_node_get_symbol_name (SymbolDBEngineIteratorNode *dbin);

gint
symbol_db_engine_iterator_node_get_symbol_file_pos (SymbolDBEngineIteratorNode *dbin);

gboolean
symbol_db_engine_iterator_node_get_symbol_is_file_scope (SymbolDBEngineIteratorNode *dbin);

const gchar* 
symbol_db_engine_iterator_node_get_symbol_signature (SymbolDBEngineIteratorNode *dbin);

/* just one SYMINFO_* per time. It is NOT possible to pass something like
 * SYMINFO_1 | SYMINFO_2 | ...
 */
const gchar*
symbol_db_engine_iterator_node_get_symbol_extra_string (SymbolDBEngineIteratorNode *dbin,
												   gint sym_info);

void
symbol_db_engine_iterator_node_set_data (SymbolDBEngineIteratorNode *dbin,
										 const GdaDataModelIter *data);

void
symbol_db_engine_iterator_node_set_conversion_hash (SymbolDBEngineIteratorNode *dbin,
										 const GHashTable *sym_type_conversion_hash);

SymbolDBEngineIteratorNode *
symbol_db_engine_iterator_node_new (const GdaDataModelIter *data);

G_END_DECLS

#endif /* _SYMBOL_DB_ENGINE_ITERATOR_NODE_H_ */

