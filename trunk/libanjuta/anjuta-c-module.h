/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-c-module.h
    Copyright (C) 2007 Sébastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _ANJUTA_C_MODULE_H_
#define _ANJUTA_C_MODULE_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_C_MODULE			(anjuta_c_module_get_type ())
#define ANJUTA_C_MODULE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_C_MODULE, AnjutaCModule))
#define ANJUTA_C_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_C_MODULE, AnjutaCModuleClass))
#define ANJUTA_IS_C_MODULE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_C_MODULE))
#define ANJUTA_IS_C_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), ANJUTA_TYPE_C_MODULE))
#define ANJUTA_C_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_TYPE_C_MODULE, AnjutaCModuleClass))

typedef struct _AnjutaCModule AnjutaCModule;
typedef struct _AnjutaCModuleClass AnjutaCModuleClass;

GType anjuta_c_module_get_type (void) G_GNUC_CONST;

AnjutaCModule *anjuta_c_module_new (const gchar *path, const char *name);
gboolean anjuta_c_module_get_last_error (AnjutaCModule *module, GError** err);

G_END_DECLS

#endif /* _ANJUTA_C_MODULE_H_ */
