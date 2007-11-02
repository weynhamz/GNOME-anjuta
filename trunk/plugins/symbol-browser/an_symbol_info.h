/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_symbol_view.c
 * Copyright (C) 2004 Naba Kumar
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __AN_SYMBOL_INFO_H__
#define __AN_SYMBOL_INFO_H__

#include <gtk/gtk.h>
#include <tm_tagmanager.h>


G_BEGIN_DECLS
#define ANJUTA_TYPE_SYMBOL_INFO        (anjuta_symbol_info_get_type ())
#define ANJUTA_SYMBOL_INFO(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_SYMBOL_INFO, AnjutaSymbolInfo))
#define ANJUTA_IS_SYMBOL_INFO(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_SYMBOL_INFO))
#define ANJUTA_IS_SYMBOL_INFO_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_SYMBOL_INFO))
typedef struct _AnjutaSymbolInfo AnjutaSymbolInfo;
typedef struct _AnjutaSymbolInfoPriv AnjutaSymbolInfoPriv;

typedef enum
{
	sv_none_t,
	sv_namespace_t,
	sv_class_t,
	sv_struct_t,
	sv_union_t,
	sv_typedef_t,
	sv_function_t,
	sv_variable_t,
	sv_enumerator_t,
	sv_macro_t,
	sv_private_func_t,
	sv_private_var_t,
	sv_protected_func_t,
	sv_protected_var_t,
	sv_public_func_t,
	sv_public_var_t,
	sv_cfolder_t,
	sv_ofolder_t,
	sv_max_t
} SVNodeType;

typedef enum
{
	sv_root_namespace_t,
	sv_root_class_t,
	sv_root_struct_t,
	sv_root_union_t,
	sv_root_function_t,
	sv_root_variable_t,
	sv_root_macro_t,
	sv_root_typedef_t,
	sv_root_none_t,
	sv_root_max_t
} SVRootType;

struct _AnjutaSymbolInfo
{

	gchar *sym_name;	/* symbol name */
	SVNodeType node_type;	/* symbol node_type: defines the type of the Symbol. This item was added. */
	struct
	{
		char *name;	/* file name */
		glong line;	/* and line of the file in which the symbol is defined */
	} def;			/* the definition struct for the symbol */
	struct
	{
		char *name;
		glong line;
	} decl;			/* the declaration struct for the symbol */

};

GType anjuta_symbol_info_get_type (void);
AnjutaSymbolInfo *anjuta_symbol_info_new (TMSymbol *sym, SVNodeType node_type);
void anjuta_symbol_info_free (AnjutaSymbolInfo *sym);

/* If sym is give sym->tag is used, otherwise the passed tag is used
 * to determine the sv node type
 */
SVNodeType anjuta_symbol_info_get_node_type (const TMSymbol *sym,
											 const TMTag *tag);
SVRootType anjuta_symbol_info_get_root_type (SVNodeType type);

/* Returns the icon pixbuf. Caller does not get a reference. */
const GdkPixbuf* anjuta_symbol_info_get_pixbuf  (SVNodeType type);

G_END_DECLS
#endif /* __AN_SYMBOL_INFO_H__ */
