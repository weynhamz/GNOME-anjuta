/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model.h
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

#ifndef _SYMBOL_DB_MODEL_H_
#define _SYMBOL_DB_MODEL_H_

#include <glib-object.h>
#include <gtk/gtktreemodel.h>
#include <libgda/gda-data-model.h>
#include "symbol-db-engine.h"

G_BEGIN_DECLS

#define SYMBOL_DB_TYPE_MODEL             (sdb_model_get_type ())
#define SYMBOL_DB_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_DB_TYPE_MODEL, SymbolDBModel))
#define SYMBOL_DB_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_DB_TYPE_MODEL, SymbolDBModelClass))
#define SYMBOL_DB_IS_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_DB_TYPE_MODEL))
#define SYMBOL_DB_IS_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_DB_TYPE_MODEL))
#define SYMBOL_DB_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_DB_TYPE_MODEL, SymbolDBModelClass))

typedef struct _SymbolDBModelClass SymbolDBModelClass;
typedef struct _SymbolDBModel SymbolDBModel;

struct _SymbolDBModelClass
{
	GObjectClass parent_class;

	/* Virtual methods */
	gboolean (*get_query_value) (SymbolDBModel *model,
	                             GdaDataModel *data_model,
	                             GdaDataModelIter *iter, gint column,
	                             GValue *value);
	
	gboolean (*get_query_value_at) (SymbolDBModel *model,
	                                GdaDataModel *data_model, gint position,
	                                gint column, GValue *value);

	/* Pure virtual methods; alternatives to signals */
	
	gboolean (*get_has_child) (SymbolDBModel *model, gint tree_level,
	                           GValue column_values[]);

	gint (*get_n_children) (SymbolDBModel *model, gint tree_level,
	                        GValue column_values[]);

	GdaDataModel* (*get_children) (SymbolDBModel *model, gint tree_level,
	                               GValue column_values[],
	                               gint offset, gint limit);
};

struct _SymbolDBModel
{
	GObject parent_instance;
};

GType sdb_model_get_type (void) G_GNUC_CONST;

/* Use this to create the model normally. The "..." part is alternatively GType
 * and gint for column type and corresponding GdaDataModel column.
 */
GtkTreeModel* symbol_db_model_new (gint n_columns, ...);

/* Normally used by bindings */
GtkTreeModel* symbol_db_model_newv (gint n_columns, GType *types,
                                    gint *data_cols);

/* Used by derived classes */
void symbol_db_model_set_columns (SymbolDBModel *model, gint n_columns,
                                  GType *types, gint *data_cols);

void symbol_db_model_update (SymbolDBModel *model);
void symbol_db_model_freeze (SymbolDBModel *model);
void symbol_db_model_thaw (SymbolDBModel *model);

G_END_DECLS

#endif /* _SYMBOL_DB_MODEL_H_ */
