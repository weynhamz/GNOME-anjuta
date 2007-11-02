/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-c-plugin-factory.h
    Copyright (C) 2007 Sebastien Granjoux  <seb.sfo@free.fr>

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

#ifndef _ANJUTA_C_PLUGIN_FACTORY_H_
#define _ANJUTA_C_PLUGIN_FACTORY_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_C_PLUGIN_FACTORY			(anjuta_c_plugin_factory_get_type ())
#define ANJUTA_C_PLUGIN_FACTORY(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_C_PLUGIN_FACTORY, AnjutaCPluginFactory))
#define ANJUTA_C_PLUGIN_FACTORY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_C_PLUGIN_FACTORY, AnjutaCPluginFactoryClass))
#define ANJUTA_IS_C_PLUGIN_FACTORY(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_C_PLUGIN_FACTORY))
#define ANJUTA_IS_C_PLUGIN_FACTORY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), ANJUTA_TYPE_C_PLUGIN_FACTORY))
#define ANJUTA_C_PLUGIN_FACTORY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_TYPE_C_PLUGIN_FACTORY, AnjutaCPluginFactoryClass))

typedef struct _AnjutaCPluginFactory AnjutaCPluginFactory;
typedef struct _AnjutaCPluginFactoryClass AnjutaCPluginFactoryClass;

GType anjuta_c_plugin_factory_get_type (void) G_GNUC_CONST;

AnjutaCPluginFactory *anjuta_c_plugin_factory_new (void);
void anjuta_c_plugin_factory_free (AnjutaCPluginFactory *factory);

G_END_DECLS
#endif /* _ANJUTA_C_PLUGIN_FACTORY_H_ */
