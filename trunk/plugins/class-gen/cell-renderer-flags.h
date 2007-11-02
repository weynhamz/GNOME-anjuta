/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  cell-renderer-flags.h
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

#ifndef __CLASSGEN_CELL_RENDERER_FLAGS_H__
#define __CLASSGEN_CELL_RENDERER_FLAGS_H__

#include <gtk/gtkcellrenderertext.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define CG_TYPE_CELL_RENDERER_FLAGS             (cg_cell_renderer_flags_get_type ())
#define CG_CELL_RENDERER_FLAGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CG_TYPE_CELL_RENDERER_FLAGS, CgCellRendererFlags))
#define CG_CELL_RENDERER_FLAGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CG_TYPE_CELL_RENDERER_FLAGS, CgCellRendererFlagsClass))
#define CG_IS_CELL_RENDERER_FLAGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CG_TYPE_CELL_RENDERER_FLAGS))
#define CG_IS_CELL_RENDERER_FLAGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CG_TYPE_CELL_RENDERER_FLAGS))
#define CG_CELL_RENDERER_FLAGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CG_TYPE_CELL_RENDERER_FLAGS, CgCellRendererFlagsClass))

typedef struct _CgCellRendererFlagsClass CgCellRendererFlagsClass;
typedef struct _CgCellRendererFlags CgCellRendererFlags;

struct _CgCellRendererFlagsClass {
	GtkCellRendererTextClass parent_class;
};

struct _CgCellRendererFlags {
	GtkCellRendererText parent_instance;
};

GType cg_cell_renderer_flags_get_type (void) G_GNUC_CONST;

GtkCellRenderer *cg_cell_renderer_flags_new (void);

G_END_DECLS

#endif /* __CLASSGEN_CELL_RENDERER_FLAGS_H__ */
