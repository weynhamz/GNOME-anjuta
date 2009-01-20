/*
 * print.h
 *
 * Copyright (C) 2002 Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 * Copyright (C) 2002 Naba Kumar <kh_naba@users.sourceforge.net>
 * Copyright (C) 2008 Sebastien Granjoux <seb.sfo@free.fr>
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

#ifndef AN_PRINTING_PRINT_H
#define AN_PRINTING_PRINT_H

#include "text_editor.h"

G_BEGIN_DECLS

void anjuta_print (gboolean preview, AnjutaPreferences *p, TextEditor *te);

#define PRINT_HEADER               "print.header"
#define PRINT_WRAP                 "print.linewrap"
#define PRINT_LINENUM_COUNT        "print.linenumber.count"
#define PRINT_LANDSCAPE            "print.landscape"
#define PRINT_COLOR                "print.color"

G_END_DECLS

#endif /* AN_PRINTING_PRINT_H */
