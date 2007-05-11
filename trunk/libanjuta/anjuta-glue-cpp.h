/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-glue-cpp.h (c) 2006 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#ifndef ANJUTA_GLUE_CPP_H
#define ANJUTA_GLUE_CPP_H

#include "anjuta-glue-factory.h"

G_BEGIN_DECLS

GObject*
anjuta_glue_cpp_load_plugin(AnjutaGlueFactory* factory, const gchar* component_name, const gchar* type_name);

G_END_DECLS

#endif
