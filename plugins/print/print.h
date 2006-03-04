/*
 * print.h
 * Copyright (C) 2002
 *     Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 *     Naba Kumar <kh_naba@users.sourceforge.net>
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

#ifndef AN_PRINTING_PRINT_H
#define AN_PRINTING_PRINT_H

#include <libanjuta/ianjuta-editor>

G_BEGIN_DECLS

void anjuta_print (gboolean preview, AnjutaPreferences *p, IAnjutaEditor *te);

#define PRINT_PAPER_SIZE           "print.paper.size"
#define PRINT_HEADER               "print.header"
#define PRINT_WRAP                 "print.linewrap"
#define PRINT_LINENUM_COUNT        "print.linenumber.count"
#define PRINT_LANDSCAPE            "print.landscape"
#define PRINT_MARGIN_LEFT          "print.margin.left"
#define PRINT_MARGIN_RIGHT         "print.margin.right"
#define PRINT_MARGIN_TOP           "print.margin.top"
#define PRINT_MARGIN_BOTTOM        "print.margin.bottom"
#define PRINT_COLOR                "print.color"

G_END_DECLS

#endif /* AN_PRINTING_PRINT_H */
