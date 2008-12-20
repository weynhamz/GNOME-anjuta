/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  validator.h
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

#ifndef __CLASSGEN_VALIDATOR_H__
#define __CLASSGEN_VALIDATOR_H__

#include <gtk/gtk.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define CG_TYPE_VALIDATOR             (cg_validator_get_type ())
#define CG_VALIDATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CG_TYPE_VALIDATOR, CgValidator))
#define CG_VALIDATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CG_TYPE_VALIDATOR, CgValidatorClass))
#define CG_IS_VALIDATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CG_TYPE_VALIDATOR))
#define CG_IS_VALIDATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CG_TYPE_VALIDATOR))
#define CG_VALIDATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CG_TYPE_VALIDATOR, CgValidatorClass))

typedef struct _CgValidatorClass CgValidatorClass;
typedef struct _CgValidator CgValidator;

/* Object that sets the a given widget to insensitive if a
 * variable amount of GtkEntrys are empty. */

struct _CgValidatorClass
{
	GObjectClass parent_class;
};

struct _CgValidator
{
	GObject parent_instance;
};

GType cg_validator_get_type (void) G_GNUC_CONST;

CgValidator *cg_validator_new (GtkWidget *widget,
                               ...);
void cg_validator_revalidate (CgValidator *validator);

G_END_DECLS

#endif /* __CLASSGEN_VALIDATOR_H__ */
