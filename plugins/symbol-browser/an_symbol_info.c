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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdl/gdl-icons.h>
#include <libanjuta/resources.h>
#include "an_symbol_info.h"

static AnjutaSymbolInfo* symbol_info_dup (const AnjutaSymbolInfo *from);
static void symbol_info_free (AnjutaSymbolInfo *sfile);

AnjutaSymbolInfo* anjuta_symbol_info_new (TMSymbol *sym, SVNodeType node_type )
{
	AnjutaSymbolInfo *sfile = g_new0 (AnjutaSymbolInfo, 1);
	sfile->sym_name = NULL;
	sfile->def.name = NULL;
	sfile->decl.name = NULL;
	
	if (sym && sym->tag && sym->tag->atts.entry.file)
	{
		sfile->sym_name = g_strdup (sym->tag->name);
		sfile->def.name =
			g_strdup (sym->tag->atts.entry.file->work_object.file_name);
		sfile->def.line = sym->tag->atts.entry.line;
		if ((tm_tag_function_t == sym->tag->type) && sym->info.equiv)
		{
			sfile->decl.name =
				g_strdup (sym->info.equiv->atts.entry.file->work_object.file_name);
			sfile->decl.line = sym->info.equiv->atts.entry.line;
		}
		
		/* adding node type */
		sfile->node_type = node_type;
	}
	return sfile;
}

void anjuta_symbol_info_free (AnjutaSymbolInfo *sym) {

	g_return_if_fail( sym != NULL );
	
	/* let's free it! */
	symbol_info_free(sym);
	
}
	
static AnjutaSymbolInfo*
symbol_info_dup (const AnjutaSymbolInfo *from)
{
	if (NULL != from)
	{
		AnjutaSymbolInfo *to = g_new0 (AnjutaSymbolInfo, 1);
		if (from->sym_name)
			to->sym_name = g_strdup (from->sym_name);
		if (from->def.name)
		{
			to->def.name = g_strdup (from->def.name);
			to->def.line = from->def.line;
		}
		if (from->decl.name)
		{
			to->decl.name = g_strdup (from->decl.name);
			to->decl.line = from->decl.line;
		}
		return to;
	}
	else
		return NULL;
}

static void
symbol_info_free (AnjutaSymbolInfo *sfile)
{

	if (sfile != NULL )
	{
		if (sfile->sym_name != NULL ) {
			g_free(sfile->sym_name);
			sfile->sym_name = NULL;
		}
		if (sfile->def.name != NULL ) {
			g_free(sfile->def.name);
			sfile->def.name = NULL;
		}
		if (sfile->decl.name != NULL ) {
			g_free(sfile->decl.name);
			sfile->decl.name = NULL;
		}
		g_free(sfile);
	}
}

GType anjuta_symbol_info_get_type (void) {
	
	static GType type = 0;
	
	if (!type)
	{
		type = g_boxed_type_register_static ("AnjutaSymbolInfo",
											 (GBoxedCopyFunc) symbol_info_dup,
											 (GBoxedFreeFunc) symbol_info_free);
	}
	return type;
}

SVNodeType
anjuta_symbol_info_get_node_type (const TMSymbol *sym, const TMTag *tag)
{
	TMTagType t_type;
	SVNodeType type;
	char access;
	
	if (sym == NULL && tag == NULL)
		return sv_none_t;

	if (sym && sym->tag == NULL)
		return sv_none_t;
	
	if (sym)
		t_type = sym->tag->type;
	else
		t_type = tag->type;
	
	if (t_type == tm_tag_file_t)
		return sv_none_t;
	
	if (sym)
		access = sym->tag->atts.entry.access;
	else
		access = tag->atts.entry.access;
	
	switch (t_type)
	{
	case tm_tag_class_t:
		type = sv_class_t;
		break;
	case tm_tag_struct_t:
		type = sv_struct_t;
		break;
	case tm_tag_union_t:
		type = sv_union_t;
		break;
	case tm_tag_function_t:
	case tm_tag_prototype_t:
	case tm_tag_method_t:
		if (sym && (sym->info.equiv) && (TAG_ACCESS_UNKNOWN == access))
			access = sym->info.equiv->atts.entry.access;
		switch (access)
		{
		case TAG_ACCESS_PRIVATE:
			type = sv_private_func_t;
			break;
		case TAG_ACCESS_PROTECTED:
			type = sv_protected_func_t;
			break;
		case TAG_ACCESS_PUBLIC:
			type = sv_public_func_t;
			break;
		default:
			type = sv_function_t;
			break;
		}
		break;
	case tm_tag_member_t:
	case tm_tag_field_t:
		switch (access)
		{
		case TAG_ACCESS_PRIVATE:
			type = sv_private_var_t;
			break;
		case TAG_ACCESS_PROTECTED:
			type = sv_protected_var_t;
			break;
		case TAG_ACCESS_PUBLIC:
			type = sv_public_var_t;
			break;
		default:
			type = sv_variable_t;
			break;
		}
		break;
	case tm_tag_externvar_t:
	case tm_tag_variable_t:
		type = sv_variable_t;
		break;
	case tm_tag_macro_t:
	case tm_tag_macro_with_arg_t:
		type = sv_macro_t;
		break;
	case tm_tag_typedef_t:
		type = sv_typedef_t;
		break;
	case tm_tag_enumerator_t:
		type = sv_enumerator_t;
		break;
	default:
		type = sv_none_t;
		break;
	}
	return type;
}

SVRootType
anjuta_symbol_info_get_root_type (SVNodeType type)
{
	if (sv_none_t == type)
		return sv_root_none_t;
	switch (type)
	{
	case sv_class_t:
		return sv_root_class_t;
	case sv_struct_t:
		return sv_root_struct_t;
	case sv_union_t:
		return sv_root_union_t;
	case sv_function_t:
		return sv_root_function_t;
	case sv_variable_t:
		return sv_root_variable_t;
	case sv_macro_t:
		return sv_root_macro_t;
	case sv_typedef_t:
		return sv_root_typedef_t;
	default:
		return sv_root_none_t;
	}
}

static GdlIcons *icon_set = NULL;
static GdkPixbuf **sv_symbol_pixbufs = NULL;

#define CREATE_SV_ICON(N, F) \
	pix_file = anjuta_res_get_pixmap_file (F); \
	sv_symbol_pixbufs[(N)] = gdk_pixbuf_new_from_file (pix_file, NULL); \
	g_free (pix_file);

static void
sv_load_symbol_pixbufs (void)
{
	gchar *pix_file;

	if (sv_symbol_pixbufs)
		return;

	if (icon_set == NULL)
		icon_set = gdl_icons_new (16);

	sv_symbol_pixbufs = g_new (GdkPixbuf *, sv_max_t + 1);

	CREATE_SV_ICON (sv_none_t,              "Icons.16x16.Literal");
	CREATE_SV_ICON (sv_class_t,             "Icons.16x16.Class");
	CREATE_SV_ICON (sv_struct_t,            "Icons.16x16.ProtectedStruct");
	CREATE_SV_ICON (sv_union_t,             "Icons.16x16.PrivateStruct");
	CREATE_SV_ICON (sv_typedef_t,           "Icons.16x16.Reference");
	CREATE_SV_ICON (sv_function_t,          "Icons.16x16.Method");
	CREATE_SV_ICON (sv_variable_t,          "Icons.16x16.Literal");
	CREATE_SV_ICON (sv_enumerator_t,        "Icons.16x16.Enum");
	CREATE_SV_ICON (sv_macro_t,             "Icons.16x16.Field");
	CREATE_SV_ICON (sv_private_func_t,      "Icons.16x16.PrivateMethod");
	CREATE_SV_ICON (sv_private_var_t,       "Icons.16x16.PrivateProperty");
	CREATE_SV_ICON (sv_protected_func_t,    "Icons.16x16.ProtectedMethod");
	CREATE_SV_ICON (sv_protected_var_t,     "Icons.16x16.ProtectedProperty");
	CREATE_SV_ICON (sv_public_func_t,       "Icons.16x16.InternalMethod");
	CREATE_SV_ICON (sv_public_var_t,        "Icons.16x16.InternalProperty");
	
	sv_symbol_pixbufs[sv_cfolder_t] = gdl_icons_get_mime_icon (icon_set,
							    "application/directory-normal");
	sv_symbol_pixbufs[sv_ofolder_t] = gdl_icons_get_mime_icon (icon_set,
							    "application/directory-normal");
	sv_symbol_pixbufs[sv_max_t] = NULL;
}

/*-----------------------------------------------------------------------------
 * return the pixbufs. It will initialize pixbufs first if they weren't before
 */
GdkPixbuf*
anjuta_symbol_info_get_pixbuf  (SVNodeType node_type)
{
	if (!sv_symbol_pixbufs)
		sv_load_symbol_pixbufs ();
	g_return_val_if_fail (node_type >=0 && node_type < sv_max_t, NULL);
		
	return sv_symbol_pixbufs[node_type];
}
