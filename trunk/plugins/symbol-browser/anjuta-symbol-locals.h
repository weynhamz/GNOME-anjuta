/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-symbol-locals.h
 * Copyright (C) Naba Kumar  <naba@gnome.org>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _ANJUTA_SYMBOL_LOCALS_H_
#define _ANJUTA_SYMBOL_LOCALS_H_

#include <gtk/gtktreeview.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_SYMBOL_LOCALS             (anjuta_symbol_locals_get_type ())
#define ANJUTA_SYMBOL_LOCALS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SYMBOL_LOCALS, AnjutaSymbolLocals))
#define ANJUTA_SYMBOL_LOCALS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SYMBOL_LOCALS, AnjutaSymbolLocalsClass))
#define ANJUTA_IS_SYMBOL_LOCALS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SYMBOL_LOCALS))
#define ANJUTA_IS_SYMBOL_LOCALS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SYMBOL_LOCALS))
#define ANJUTA_SYMBOL_LOCALS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_SYMBOL_LOCALS, AnjutaSymbolLocalsClass))

typedef struct _AnjutaSymbolLocalsClass AnjutaSymbolLocalsClass;
typedef struct _AnjutaSymbolLocals AnjutaSymbolLocals;

struct _AnjutaSymbolLocalsClass
{
	GtkTreeViewClass parent_class;
};

struct _AnjutaSymbolLocals
{
	GtkTreeView parent_instance;
};

GType anjuta_symbol_locals_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_symbol_locals_new (void);

G_END_DECLS

#endif /* _ANJUTA_SYMBOL_LOCALS_H_ */
