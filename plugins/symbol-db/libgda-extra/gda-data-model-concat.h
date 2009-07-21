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

#ifndef _GDA_DATA_MODEL_CONCAT_H_
#define _GDA_DATA_MODEL_CONCAT_H_

#include <libgda/gda-data-model.h>
#include <libgda/gda-data-select.h>
#include <libgda/gda-data-model-iter.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GDA_TYPE_DATA_MODEL_CONCAT             (gdm_concat_get_type ())
#define GDA_DATA_MODEL_CONCAT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDA_TYPE_DATA_MODEL_CONCAT, GdaDataModelConcat))
#define GDA_DATA_MODEL_CONCAT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDA_TYPE_DATA_MODEL_CONCAT, GdaDataModelConcatClass))
#define GDA_IS_DATA_MODEL_CONCAT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDA_TYPE_DATA_MODEL_CONCAT))
#define GDA_IS_DATA_MODEL_CONCAT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDA_TYPE_DATA_MODEL_CONCAT))
#define GDA_DATA_MODEL_CONCAT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDA_TYPE_DATA_MODEL_CONCAT, GdaDataModelConcatClass))

typedef struct _GdaDataModelConcat 			GdaDataModelConcat;
typedef struct _GdaDataModelConcatClass 	GdaDataModelConcatClass;
typedef struct _GdaDataModelConcatPrivate 	GdaDataModelConcatPrivate;

struct _GdaDataModelConcat
{
	GObject parent_instance;
	GdaDataModelConcatPrivate *priv;
};

struct _GdaDataModelConcatClass
{
	GObjectClass parent_class;	
};


GType gdm_concat_get_type (void) G_GNUC_CONST;

GdaDataModel *gda_data_model_concat_new (void);

void gda_data_model_concat_append_model (GdaDataModelConcat *mconcat, 
    										GdaDataModel *model);

G_END_DECLS

#endif /* _GDA_DATA_MODEL_CONCAT_H_ */
