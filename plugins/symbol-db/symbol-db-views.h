/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-views.h
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

#ifndef _SYMBOL_DB_VIEWS_H_
#define _SYMBOL_DB_VIEWS_H_

#include "symbol-db-engine.h"
#include "plugin.h"

G_BEGIN_DECLS

typedef enum {
	SYMBOL_DB_VIEW_PROJECT,
	SYMBOL_DB_VIEW_FILE,
	SYMBOL_DB_VIEW_SEARCH
} SymbolViewType;

GtkWidget* symbol_db_view_new (SymbolViewType view_type,
                               SymbolDBEngine *dbe, SymbolDBPlugin *plugin);

G_END_DECLS

#endif /* _SYMBOL_DB_VIEWS_H_ */
