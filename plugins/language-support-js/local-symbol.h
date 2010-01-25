/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

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

#ifndef _LOCAL_SYMBOL_H_
#define _LOCAL_SYMBOL_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define LOCAL_TYPE_SYMBOL             (local_symbol_get_type ())
#define LOCAL_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOCAL_TYPE_SYMBOL, LocalSymbol))
#define LOCAL_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LOCAL_TYPE_SYMBOL, LocalSymbolClass))
#define LOCAL_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOCAL_TYPE_SYMBOL))
#define LOCAL_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOCAL_TYPE_SYMBOL))
#define LOCAL_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), LOCAL_TYPE_SYMBOL, LocalSymbolClass))

typedef struct _LocalSymbolClass LocalSymbolClass;
typedef struct _LocalSymbol LocalSymbol;

struct _LocalSymbolClass
{
	GObjectClass parent_class;
};

struct _LocalSymbol
{
	GObject parent_instance;
};

GType local_symbol_get_type (void) G_GNUC_CONST;
LocalSymbol* local_symbol_new (const gchar *filename);
GList* local_symbol_list_member_with_line (LocalSymbol* object, gint line);
GList* local_symbol_get_missed_semicolons (LocalSymbol* object);

G_END_DECLS

#endif /* _LOCAL_SYMBOL_H_ */
