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

//	#include <tm_tagmanager.h> 
#include "an_symbol_info.h"


 
AnjutaSymbolInfo* anjuta_symbol_info_new (TMSymbol *sym, SVNodeType node_type )
{
	AnjutaSymbolInfo *sfile = g_new0 (AnjutaSymbolInfo, 1);
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
		
		// adding node type
		sfile->node_type = node_type;
	}
	return sfile;
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
	if (sfile)
	{
		if (sfile->sym_name)
			g_free(sfile->sym_name);
		if (sfile->def.name)
			g_free(sfile->def.name);
		if (sfile->decl.name)
			g_free(sfile->decl.name);
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
