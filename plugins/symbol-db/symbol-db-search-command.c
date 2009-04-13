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

#include "symbol-db-search-command.h"

#include <libanjuta/anjuta-debug.h>

struct _SymbolDBSearchCommandPriv {
	/* may be set or not. Initial value (at init time) is NULL 
	 * it shouldn't be freed. */
	GFile *gfile;	

	/* may be set or not. Initial value (at init time) is NULL 
	 * it shouldn't be freed. */
	GList *session_packages;

	SymbolDBEngine *dbe;
	CmdSearchType cmd_search_type;
	IAnjutaSymbolType match_types;
	gboolean include_types;
	IAnjutaSymbolField info_fields;
	const gchar *pattern;
	gint results_limit;
	gint results_offset;
	

	/* store the pointer to an iterator. The object does not have to be
	 * freed 
	 */
	SymbolDBEngineIterator *iterator_result;
};


G_DEFINE_TYPE (SymbolDBSearchCommand, sdb_search_command, ANJUTA_TYPE_ASYNC_COMMAND);

static void
sdb_search_command_init (SymbolDBSearchCommand *object)
{
	object->priv = g_new0 (SymbolDBSearchCommandPriv, 1);	

	object->priv->gfile = NULL;
	object->priv->session_packages = NULL;
}

static void
sdb_search_command_finalize (GObject *object)
{
	SymbolDBSearchCommand *sdbsc;	
	sdbsc = SYMBOL_DB_SEARCH_COMMAND (object);
	
	g_free (sdbsc->priv);
	
	G_OBJECT_CLASS (sdb_search_command_parent_class)->finalize (object);
}

static SymbolDBEngineIterator *
do_search_file (SymbolDBSearchCommand *sdbsc)
{
	SymbolDBSearchCommandPriv *priv;
	SymbolDBEngineIterator *iterator;
	GPtrArray *filter_array;
	gchar *abs_file_path;	

	priv = sdbsc->priv;
	
	abs_file_path = g_file_get_path (priv->gfile);

	if (abs_file_path == NULL)
	{		
		return NULL;
	}
	
	if (priv->match_types & IANJUTA_SYMBOL_TYPE_UNDEF)
	{
		filter_array = NULL;
	}
	else 
	{
		filter_array = symbol_db_util_fill_type_array (priv->match_types);
	}
	
	iterator = 
		symbol_db_engine_find_symbol_by_name_pattern_on_file (priv->dbe,
				    priv->pattern,
					abs_file_path,
					filter_array,
					priv->include_types,
					priv->results_limit,
					priv->results_offset,
					priv->info_fields);
	
	g_free (abs_file_path);	

	return iterator;
}

static SymbolDBEngineIterator *
do_search_prj_glb (SymbolDBSearchCommand *sdbsc)
{
	SymbolDBEngineIterator *iterator;
	gboolean exact_match;
	GPtrArray *filter_array;
	SymbolDBSearchCommandPriv *priv;
	

	priv = sdbsc->priv;
	
	exact_match = symbol_db_util_is_pattern_exact_match (priv->pattern);

	if (priv->match_types & IANJUTA_SYMBOL_TYPE_UNDEF)
	{
		filter_array = NULL;
	}
	else 
	{
		filter_array = symbol_db_util_fill_type_array (priv->match_types);
	}
	
	iterator = 		
		symbol_db_engine_find_symbol_by_name_pattern_filtered (priv->dbe,
					priv->pattern,
					exact_match,
					filter_array,
					priv->include_types,
					1,
					priv->session_packages,
					priv->results_limit,
					priv->results_offset,
					priv->info_fields);	
	
	return iterator;
}

/**
 * Main method that'll run the task assigned with the command.
 */
static guint
sdb_search_command_run (AnjutaCommand *command)
{
	SymbolDBSearchCommand *sdbsc;
	SymbolDBSearchCommandPriv *priv;

	sdbsc = SYMBOL_DB_SEARCH_COMMAND (command);

	priv = sdbsc->priv;

	switch (priv->cmd_search_type)
	{
		case CMD_SEARCH_FILE:
			priv->iterator_result = do_search_file (sdbsc);
			break;

		case CMD_SEARCH_PROJECT:
		case CMD_SEARCH_SYSTEM:
			priv->iterator_result = do_search_prj_glb (sdbsc);
			break;			
	}

	if (priv->iterator_result == NULL)
	{
		/* 1 is for error occurred */
		return 1;
	}

	anjuta_command_notify_data_arrived (command);
	
	/* 0 should be for no error */
	return 0;
}

static void
sdb_search_command_class_init (SymbolDBSearchCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);
	
	object_class->finalize = sdb_search_command_finalize;
	command_class->run = sdb_search_command_run;
}

/**
 * ctor.
 */
SymbolDBSearchCommand* 
symbol_db_search_command_new (SymbolDBEngine *dbe, CmdSearchType cmd_search_type, 
                              IAnjutaSymbolType match_types, gboolean include_types,  
                              IAnjutaSymbolField info_fields, const gchar *pattern, 
                              gint results_limit, gint results_offset)
{
	SymbolDBSearchCommand *sdb_search_cmd;
	SymbolDBSearchCommandPriv *priv;
	
	sdb_search_cmd = g_object_new (SYMBOL_TYPE_DB_SEARCH_COMMAND, NULL);

	priv = sdb_search_cmd->priv;

	/* set some priv data. Nothing should be freed later */
	priv->cmd_search_type = cmd_search_type;
	priv->match_types = match_types;
	priv->include_types = include_types;
	priv->info_fields = info_fields;
	priv->pattern = pattern;
	priv->results_limit = results_limit;
	priv->results_offset = results_offset;	
	priv->dbe = dbe;
	priv->iterator_result = NULL;
	
	return sdb_search_cmd;
}

void
symbol_db_search_command_set_file (SymbolDBSearchCommand* sdbsc, const GFile *gfile)
{
	SymbolDBSearchCommandPriv *priv;

	g_return_if_fail (sdbsc != NULL);
	g_return_if_fail (gfile != NULL);
	
	priv = sdbsc->priv;
	
	priv->gfile = (GFile*)gfile;
}	

void
symbol_db_search_command_set_session_packages (SymbolDBSearchCommand* sdbsc, 
                                               const GList *session_packages)
{
	SymbolDBSearchCommandPriv *priv;

	g_return_if_fail (sdbsc != NULL);
	
	priv = sdbsc->priv;
	
	priv->session_packages = (GList*)session_packages;
}	

SymbolDBEngineIterator *
symbol_db_search_command_get_iterator_result (SymbolDBSearchCommand* sdbsc)
{
	SymbolDBSearchCommandPriv *priv;
	g_return_val_if_fail (sdbsc != NULL, NULL);
	
	priv = sdbsc->priv;

	return priv->iterator_result;
}
