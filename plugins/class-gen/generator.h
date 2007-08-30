/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  generator.h
 *  Copyright (C) 2006 Armin Burgmeier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __CLASSGEN_GENERATOR_H__
#define __CLASSGEN_GENERATOR_H__

#include <plugins/project-wizard/values.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define CG_TYPE_GENERATOR             (cg_generator_get_type ())
#define CG_GENERATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CG_TYPE_GENERATOR, CgGenerator))
#define CG_GENERATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CG_TYPE_GENERATOR, CgGeneratorClass))
#define CG_IS_GENERATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CG_TYPE_GENERATOR))
#define CG_IS_GENERATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CG_TYPE_GENERATOR))
#define CG_GENERATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CG_TYPE_GENERATOR, CgGeneratorClass))

typedef struct _CgGeneratorClass CgGeneratorClass;
typedef struct _CgGenerator CgGenerator;

/* Actual Code generation using autogen */

typedef enum
{
	CG_GENERATOR_ERROR_DEFFILE,
	CG_GENERATOR_ERROR_NOT_GENERATED,

	CG_GENERATOR_ERROR_FAILED
} _CgGeneratorError;

struct _CgGeneratorClass
{
	GObjectClass parent_class;
};

struct _CgGenerator
{
	GObject parent_instance;
};

GType cg_generator_get_type (void) G_GNUC_CONST;

CgGenerator *cg_generator_new (const gchar *header_template,
                               const gchar *source_template,
                               const gchar *header_destination,
                               const gchar *source_destination);
gboolean cg_generator_run (CgGenerator *generator,
                           NPWValueHeap *values,
                           GError **error);

const gchar *cg_generator_get_header_template (CgGenerator *generator);
const gchar *cg_generator_get_source_template (CgGenerator *generator);
const gchar *cg_generator_get_header_destination (CgGenerator *generator);
const gchar *cg_generator_get_source_destination (CgGenerator *generator);

G_END_DECLS

#endif /* __CLASSGEN_GENERATOR_H__ */
