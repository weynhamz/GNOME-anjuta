/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model-file.h
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

#ifndef _SYMBOL_DB_MODEL_FILE_H_
#define _SYMBOL_DB_MODEL_FILE_H_

#include <glib-object.h>
#include "symbol-db-model-project.h"

G_BEGIN_DECLS

#define SYMBOL_DB_TYPE_MODEL_FILE             (symbol_db_model_file_get_type ())
#define SYMBOL_DB_MODEL_FILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_DB_TYPE_MODEL_FILE, SymbolDBModelFile))
#define SYMBOL_DB_MODEL_FILE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_DB_TYPE_MODEL_FILE, SymbolDBModelFileClass))
#define SYMBOL_DB_IS_MODEL_FILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_DB_TYPE_MODEL_FILE))
#define SYMBOL_DB_IS_MODEL_FILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_DB_TYPE_MODEL_FILE))
#define SYMBOL_DB_MODEL_FILE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_DB_TYPE_MODEL_FILE, SymbolDBModelFileClass))

typedef struct _SymbolDBModelFileClass SymbolDBModelFileClass;
typedef struct _SymbolDBModelFile SymbolDBModelFile;

enum {
	SYMBOL_DB_MODEL_FILE_COL_SYMBOL_ID,
	SYMBOL_DB_MODEL_FILE_COL_PIXBUF,
	SYMBOL_DB_MODEL_FILE_COL_LABEL,
	SYMBOL_DB_MODEL_FILE_COL_FILE,
	SYMBOL_DB_MODEL_FILE_COL_LINE,
	SYMBOL_DB_MODEL_FILE_COL_N_COLS
};

struct _SymbolDBModelFileClass
{
	SymbolDBModelProjectClass parent_class;
};

struct _SymbolDBModelFile
{
	SymbolDBModel parent_instance;
};

GType symbol_db_model_file_get_type (void) G_GNUC_CONST;
GtkTreeModel* symbol_db_model_file_new (SymbolDBEngine* dbe);

G_END_DECLS

#endif /* _SYMBOL_DB_MODEL_FILE_H_ */
