/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005		Massimo Corà <maxcvs@email.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __AN_SYMBOL_SEARCH__
#define __AN_SYMBOL_SEARCH__

#include <glib-object.h>
#include <gtk/gtkwidget.h>
#include "an_symbol_view.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_SYMBOL_SEARCH		(anjuta_symbol_search_get_type ())
#define ANJUTA_SYMBOL_SEARCH(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SYMBOL_SEARCH, AnjutaSymbolSearch))
#define ANJUTA_SYMBOL_SEARCH_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SYMBOL_SEARCH, AnjutaSymbolSearchClass))
#define ANJUTA_SYMBOL_IS_SEARCH(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SYMBOL_SEARCH))
#define ANJUTA_SYMBOL_IS_SEARCH_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SYMBOL_SEARCH))

typedef struct _AnjutaSymbolSearchPriv   	AnjutaSymbolSearchPriv;
typedef struct _AnjutaSymbolSearch     	AnjutaSymbolSearch;
typedef struct _AnjutaSymbolSearchClass  	AnjutaSymbolSearchClass;

struct _AnjutaSymbolSearch {
        GtkVBox parent;
        
        AnjutaSymbolSearchPriv  *priv;
};

struct _AnjutaSymbolSearchClass {
        GtkVBoxClass   parent_class;

        /* Signals */
        void (*symbol_selected) (AnjutaSymbolSearch *search, AnjutaSymbolInfo *sym);
};

GType anjuta_symbol_search_get_type (void);
GtkWidget * anjuta_symbol_search_new (AnjutaSymbolView *symbol_view); 

void anjuta_symbol_search_set_search_string  (AnjutaSymbolSearch *search, const gchar *str);
void anjuta_symbol_search_grab_focus (AnjutaSymbolSearch *search);

void anjuta_symbol_search_set_file_symbol_model(AnjutaSymbolSearch *search, GtkTreeModel * model);
void anjuta_symbol_search_set_keywords_symbols(AnjutaSymbolSearch *search, GList *keywords);
void anjuta_symbol_search_set_pixbufs(AnjutaSymbolSearch *search, GdkPixbuf ** pix);
void anjuta_symbol_search_clear (AnjutaSymbolSearch *search);

G_END_DECLS

#endif /* __AN_SYMBOL_SEARCH__H__ */
