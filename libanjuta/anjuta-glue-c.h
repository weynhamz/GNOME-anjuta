/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-glue-c.h
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

#ifndef __ANJUTA_GLUE_C_H__
#define __ANJUTA_GLUE_C_H__

#include "anjuta-glue-plugin.h"

#include <gmodule.h>

G_BEGIN_DECLS

#define ANJUTA_GLUE_TYPE_C_PLUGIN			(anjuta_glue_c_plugin_get_type ())
#define ANJUTA_GLUE_C_PLUGIN(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_GLUE_TYPE_C_PLUGIN, AnjutaGlueCPlugin))
#define ANJUTA_GLUE_C_PLUGIN_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_GLUE_TYPE_C_PLUGIN, AnjutaGlueCPluginClass))
#define ANJUTA_GLUE_IS_C_PLUGIN(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_GLUE_TYPE_C_PLUGIN))
#define ANJUTA_GLUE_IS_C_PLUGIN_CLASS(klass)(G_TYPE_CHECK_CLASS_TYPE ((obj), ANJUTA_GLUE_TYPE_C_PLUGIN))
#define ANJUTA_GLUE_C_PLUGIN_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_GLUE_TYPE_C_PLUGIN, AnjutaGlueCPluginClass))

typedef struct _AnjutaGlueCPlugin      AnjutaGlueCPlugin;
typedef struct _AnjutaGlueCPluginClass AnjutaGlueCPluginClass;

struct _AnjutaGlueCPlugin
{
  AnjutaGluePlugin parent;
  GModule *module;
};

struct _AnjutaGlueCPluginClass
{
  AnjutaGluePluginClass parent_class;
};

GType       anjuta_glue_c_plugin_get_type      (void) G_GNUC_CONST;

AnjutaGluePlugin *anjuta_glue_c_plugin_new     (void);

G_END_DECLS

#endif /* __ANJUTA_GLUE_C_H__ */
