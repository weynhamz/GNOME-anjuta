/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2009 <maxcvs@email.it>
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

#ifndef _SYMBOL_DB_SEARCH_COMMAND_H_
#define _SYMBOL_DB_SEARCH_COMMAND_H_

#include <glib-object.h>
#include <libanjuta/anjuta-async-command.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

#include "symbol-db-engine.h"

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_SEARCH_COMMAND             (sdb_search_command_get_type ())
#define SYMBOL_DB_SEARCH_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_SEARCH_COMMAND, SymbolDBSearchCommand))
#define SYMBOL_DB_SEARCH_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_SEARCH_COMMAND, SymbolDBSearchCommandClass))
#define SYMBOL_IS_DB_SEARCH_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_SEARCH_COMMAND))
#define SYMBOL_IS_DB_SEARCH_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_SEARCH_COMMAND))
#define SYMBOL_DB_SEARCH_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_SEARCH_COMMAND, SymbolDBSearchCommandClass))

typedef struct _SymbolDBSearchCommandClass SymbolDBSearchCommandClass;
typedef struct _SymbolDBSearchCommand SymbolDBSearchCommand;
typedef struct _SymbolDBSearchCommandPriv SymbolDBSearchCommandPriv;

struct _SymbolDBSearchCommandClass
{
	AnjutaAsyncCommandClass parent_class;
};

struct _SymbolDBSearchCommand
{
	AnjutaAsyncCommand parent_instance;

	SymbolDBSearchCommandPriv *priv;
};

typedef enum {

	CMD_SEARCH_FILE,
	CMD_SEARCH_PROJECT,
	CMD_SEARCH_SYSTEM
	
} CmdSearchType;

GType sdb_search_command_get_type (void) G_GNUC_CONST;

SymbolDBSearchCommand* 
symbol_db_search_command_new (SymbolDBEngine *dbe, CmdSearchType cmd_search_type, 
                              IAnjutaSymbolType match_types, gboolean include_types,  
                              IAnjutaSymbolField info_fields, const gchar *pattern, 
			 				  gint results_limit, gint results_offset);

/** set a gfile in case of a CMD_SEARCH_FILE command */
void
symbol_db_search_command_set_file (SymbolDBSearchCommand* sdbsc, const GFile *gfile);

/** set a GList for session packages. It should be NULL if client wants just a project scan */ 
void
symbol_db_search_command_set_session_packages (SymbolDBSearchCommand* sdbsc, 
                                               const GList *session_packages);


/** get result iterator when command is finished */
SymbolDBEngineIterator *
symbol_db_search_command_get_iterator_result (SymbolDBSearchCommand* sdbsc);


G_END_DECLS

#endif /* _SYMBOL_DB_SEARCH_COMMAND_H_ */
