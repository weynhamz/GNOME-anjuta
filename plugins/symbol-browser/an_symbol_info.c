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
anjuta_symbol_info_get_node_type (TMSymbol * sym)
{
	SVNodeType type;
	char access;

	if (!sym || !sym->tag || (tm_tag_file_t == sym->tag->type))
		return sv_none_t;
	access = sym->tag->atts.entry.access;
	switch (sym->tag->type)
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
		if ((sym->info.equiv) && (TAG_ACCESS_UNKNOWN == access))
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
