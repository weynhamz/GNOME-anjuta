/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model-project.h
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

#ifndef _SYMBOL_DB_MODEL_PROJECT_H_
#define _SYMBOL_DB_MODEL_PROJECT_H_

#include <glib-object.h>
#include "symbol-db-model.h"

G_BEGIN_DECLS

#define SYMBOL_DB_TYPE_MODEL_PROJECT             (sdb_model_project_get_type ())
#define SYMBOL_DB_MODEL_PROJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_DB_TYPE_MODEL_PROJECT, SymbolDBModelProject))
#define SYMBOL_DB_MODEL_PROJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_DB_TYPE_MODEL_PROJECT, SymbolDBModelProjectClass))
#define SYMBOL_DB_IS_MODEL_PROJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_DB_TYPE_MODEL_PROJECT))
#define SYMBOL_DB_IS_MODEL_PROJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_DB_TYPE_MODEL_PROJECT))
#define SYMBOL_DB_MODEL_PROJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_DB_TYPE_MODEL_PROJECT, SymbolDBModelProjectClass))

typedef struct _SymbolDBModelProjectClass SymbolDBModelProjectClass;
typedef struct _SymbolDBModelProject SymbolDBModelProject;

enum {
	SYMBOL_DB_MODEL_PROJECT_COL_SYMBOL_ID,
	SYMBOL_DB_MODEL_PROJECT_COL_PIXBUF,
	SYMBOL_DB_MODEL_PROJECT_COL_LABEL,
	SYMBOL_DB_MODEL_PROJECT_COL_FILE,
	SYMBOL_DB_MODEL_PROJECT_COL_LINE,
	SYMBOL_DB_MODEL_PROJECT_COL_ARGS,
	SYMBOL_DB_MODEL_PROJECT_COL_N_COLS
};

struct _SymbolDBModelProjectClass
{
	SymbolDBModelClass parent_class;
};

struct _SymbolDBModelProject
{
	SymbolDBModel parent_instance;
};

GType sdb_model_project_get_type (void) G_GNUC_CONST;
GtkTreeModel* symbol_db_model_project_new (SymbolDBEngine* dbe);

G_END_DECLS

#endif /* _SYMBOL_DB_MODEL_PROJECT_H_ */
