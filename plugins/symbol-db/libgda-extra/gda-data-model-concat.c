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

#include "gda-data-model-concat.h"

/**
 *
 * An external user won't recognize that the GdaDataModel is composed
 * by more than a GdaDataModel with the same structure (n_rows, n_cols etc)
 *
 * |------------------- GdaDataModel -------------------|    (external vision)
 * |<----- ModelSlice 1 -----><----- ModelSlice 2 ----->|    (internal vision)
 */

/*
 * I would rather present it as the following because each data model concatenated
 * need to have the same number an types of columns, but can have any number of rows.
 *
 * col0 | col1  | ...   
 * -----+-------+---------
 * ...
 * Rows from Slice 0
 * ...
 * -----+-------+---------
 * ...
 * Rows from Slice 1
 * ...
 * -----+-------+---------
 * ...
 * Rows from Slice N
 * ...
 *
 */
typedef struct _ModelSlice {
	GdaDataModel *model;

	/* In a general way these number represents the boundaries 
	 * mapped on the GdaDataModel as seen from an external user. See ascii above.
	 */
	gint lower_bound;
	gint upper_bound;
	
} ModelSlice;

struct _GdaDataModelConcatPrivate {
	
	/* a list of ModelSlice(s) */
	GList *slices;
	ModelSlice *curr_model_slice;

	/* num of records which is the total of the different models appended */
	gint tot_items;
};


static GObjectClass *parent_class = NULL;


static void
gdm_concat_iface_impl_init (GdaDataModelIface *iface);

static void
gdm_concat_model_slice_destroy (ModelSlice *slice)
{
	g_object_unref (slice->model);
	g_free (slice);
}

static void
gdm_concat_init (GdaDataModelConcat *mconcat)
{
	g_return_if_fail (GDA_IS_DATA_MODEL_CONCAT (mconcat));
	mconcat->priv = g_new0 (GdaDataModelConcatPrivate, 1);
	mconcat->priv->slices = NULL;
	mconcat->priv->curr_model_slice = NULL;
	mconcat->priv->tot_items = 0;
}

static void
gdm_concat_dispose (GObject *object)
{
	GdaDataModelConcat *mconcat = GDA_DATA_MODEL_CONCAT (object);
	GdaDataModelConcatPrivate *priv;

	priv = mconcat->priv;
	
	/* we'll just point curr_moodel_slice to NULL because it's already
	 * in the slices list
	 */
	priv->curr_model_slice = NULL;
	
	GList *node = priv->slices;
	while (node != NULL)
	{
		gdm_concat_model_slice_destroy ((ModelSlice*) node->data);

		node = node->next;
	}
	
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gdm_concat_finalize (GObject *object)
{
	GdaDataModelConcat *mconcat = (GdaDataModelConcat *) object;
	
	/* free memory */
	if (mconcat->priv) 
	{
		g_free (mconcat->priv);
		mconcat->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gdm_concat_class_init (GdaDataModelConcatClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = gdm_concat_dispose;	
	object_class->finalize = gdm_concat_finalize;
}

/**
 * gdm_concat_get_type
 *
 * Returns: the #GType of GdaDataModelConcat.
 */
GType
gdm_concat_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		static GStaticMutex registering = G_STATIC_MUTEX_INIT;
		static const GTypeInfo info = {
			sizeof (GdaDataModelConcatClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gdm_concat_class_init,
			NULL,
			NULL,
			sizeof (GdaDataModelConcat),
			0,
			(GInstanceInitFunc) gdm_concat_init
		};

		static const GInterfaceInfo data_model_info = {
			(GInterfaceInitFunc) gdm_concat_iface_impl_init,
			NULL,
			NULL
		};

		g_static_mutex_lock (&registering);
		if (type == 0) {
			type = g_type_register_static (G_TYPE_OBJECT, "GdaDataModelConcat", &info, 0);
			g_type_add_interface_static (type, GDA_TYPE_DATA_MODEL, &data_model_info);
		}
		g_static_mutex_unlock (&registering);
	}
	return type;
}

static gint
gdm_concat_get_n_rows (GdaDataModel *model)
{
	GdaDataModelConcat *mconcat = GDA_DATA_MODEL_CONCAT (model);
	GdaDataModelConcatPrivate *priv;

	g_return_val_if_fail (model != NULL, -1);

	priv = mconcat->priv;

	return priv->tot_items;	
}

static gint
gdm_concat_get_n_columns (GdaDataModel *model)
{
	GdaDataModelConcat *mconcat = GDA_DATA_MODEL_CONCAT (model);
	GdaDataModelConcatPrivate *priv;

	g_return_val_if_fail (model != NULL, -1);

	priv = mconcat->priv;

	if (priv->curr_model_slice == NULL)
	{
		g_warning ("No model has been appended yet");
		return -1;
	}

	return gda_data_model_get_n_columns (priv->curr_model_slice->model);
}

static GdaColumn *
gdm_concat_describe_column (GdaDataModel *model, gint col)
{
	GdaDataModelConcat *mconcat = GDA_DATA_MODEL_CONCAT (model);
	GdaDataModelConcatPrivate *priv;

	g_return_val_if_fail (model != NULL, NULL);

	priv = mconcat->priv;

	if (priv->curr_model_slice == NULL)
	{
		g_warning ("No model has been appended yet");
		return NULL;
	}

	return gda_data_model_describe_column (priv->curr_model_slice->model, col);
}


static gint
gdm_concat_set_curr_slice_by_row_num (GdaDataModelConcat *mconcat, gint row)
{	
	GdaDataModelConcatPrivate *priv;
	ModelSlice *found_slice = NULL;
	
	priv = mconcat->priv;

	/* iterate and search for the correct slice (e.g. the one that suits the bounds) */
	GList *node = priv->slices;
	
    while (node) 
	{
		ModelSlice *curr_slice = node->data;
		
    	if (curr_slice && row >= curr_slice->lower_bound && 
		    row <= curr_slice->upper_bound)
		{
			found_slice = curr_slice;
			break;
		}
      	node = g_list_next (node);
    }

	/* if not found it'll be set to NULL */
	priv->curr_model_slice = found_slice;

	/* if NULL return -1, otherwise return the relative row value */	 
	return priv->curr_model_slice == NULL ? -1 : 
		row - priv->curr_model_slice->lower_bound;
}

static const GValue * 
gdm_concat_get_value_at (GdaDataModel *model, gint col, gint row, 
    GError **error)
{
	GdaDataModelConcat *mconcat = GDA_DATA_MODEL_CONCAT (model);
	GdaDataModelConcatPrivate *priv;
	gint relative_row;

	g_return_val_if_fail (model != NULL, NULL);

	priv = mconcat->priv;
	
	if ( (relative_row = gdm_concat_set_curr_slice_by_row_num (mconcat, row)) < 0) 
	{
		/* maybe an array out of bound error occurred */
		return NULL;
	}
	
	return gda_data_model_get_value_at (priv->curr_model_slice->model, col, relative_row, 
	    error);
}

static GdaDataModelAccessFlags
gda_concat_get_access_flags (GdaDataModel *model)
{
	return GDA_DATA_MODEL_ACCESS_RANDOM;
}

static void
gdm_concat_iface_impl_init (GdaDataModelIface *iface)
{
	iface->i_get_n_rows = gdm_concat_get_n_rows;
	iface->i_get_n_columns = gdm_concat_get_n_columns;
	iface->i_create_iter = NULL;	
    
	iface->i_get_value_at = gdm_concat_get_value_at;
	iface->i_iter_at_row = NULL;
	iface->i_iter_next = NULL;
	iface->i_iter_prev = NULL;

	iface->i_get_attributes_at = NULL;
	iface->i_get_access_flags = gda_concat_get_access_flags;
	iface->i_describe_column = gdm_concat_describe_column;
	
	iface->i_set_value_at = NULL;
	iface->i_iter_set_value = NULL;
	iface->i_set_values = NULL;
    iface->i_append_values = NULL;
	iface->i_append_row = NULL;
	iface->i_remove_row = NULL;
	iface->i_find_row = NULL;
	
	iface->i_set_notify = NULL;
	iface->i_get_notify = NULL;
	iface->i_send_hint = NULL;
}

GdaDataModel *
gda_data_model_concat_new (void)
{
	GdaDataModelConcat *mconcat;

	mconcat = g_object_new (GDA_TYPE_DATA_MODEL_CONCAT, NULL);

	return GDA_DATA_MODEL (mconcat);
}

void 
gda_data_model_concat_append_model (GdaDataModelConcat *mconcat, GdaDataModel *model)
{
	GdaDataModelConcatPrivate *priv;
	gint new_model_rows;

	g_return_if_fail (GDA_IS_DATA_MODEL_CONCAT (mconcat));
	g_return_if_fail (GDA_IS_DATA_MODEL (model));

	/* we want random access data models */
	g_return_if_fail (gda_data_model_get_access_flags (model) & GDA_DATA_MODEL_ACCESS_RANDOM);

	priv = mconcat->priv;

	/* get the n rows of the new model to be attached */
	new_model_rows = gda_data_model_get_n_rows (model);
	
	ModelSlice *slice = g_new0 (ModelSlice, 1);
	slice->model = g_object_ref (model);

	slice->lower_bound = priv->tot_items;
	slice->upper_bound = priv->tot_items + new_model_rows -1;
	
	/* append the new object to the list */
	priv->slices = g_list_append (priv->slices, slice);

	priv->tot_items += new_model_rows;

	if (priv->curr_model_slice == NULL)
		priv->curr_model_slice = slice;
}    

