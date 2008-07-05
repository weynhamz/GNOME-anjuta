/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-prefs.h
 * Copyright (C) Massimo Cora' 2007-2008 <maxcvs@email.it>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef __SYMBOL_DB_PREFS_H__
#define __SYMBOL_DB_PREFS_H__

#include "plugin.h"

void symbol_db_prefs_init (SymbolDBPlugin *plugin, AnjutaPreferences *prefs);
void symbol_db_prefs_finalize (SymbolDBPlugin *plugin, AnjutaPreferences *prefs);

/*
void symbol_db_load_global_tags (gpointer plugin);
*/
#endif
