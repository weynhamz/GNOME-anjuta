/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Naba Kumar 2010 <naba@gnome.org>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYMBOL_DB_QUERY_H_
#define _SYMBOL_DB_QUERY_H_

#include <glib-object.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>

G_BEGIN_DECLS

#define SYMBOL_DB_TYPE_QUERY             (sdb_query_get_type ())
#define SYMBOL_DB_QUERY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_DB_TYPE_QUERY, SymbolDBQuery))
#define SYMBOL_DB_QUERY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_DB_TYPE_QUERY, SymbolDBQueryClass))
#define SYMBOL_DB_IS_QUERY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_DB_TYPE_QUERY))
#define SYMBOL_DB_IS_QUERY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_DB_TYPE_QUERY))
#define SYMBOL_DB_QUERY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_DB_TYPE_QUERY, SymbolDBQueryClass))

typedef struct _SymbolDBQueryClass SymbolDBQueryClass;
typedef struct _SymbolDBQuery SymbolDBQuery;
typedef struct _SymbolDBQueryPriv SymbolDBQueryPriv;

struct _SymbolDBQueryClass
{
	GObjectClass parent_class;
};

struct _SymbolDBQuery
{
	GObject parent_instance;
	SymbolDBQueryPriv *priv;
};

GType sdb_query_get_type (void) G_GNUC_CONST;
SymbolDBQuery* symbol_db_query_new (SymbolDBEngine *system_db_engine,
                                    SymbolDBEngine *project_db_engine,
                                    IAnjutaSymbolQueryName name);

G_END_DECLS

#endif /* _SYMBOL_DB_QUERY_H_ */
