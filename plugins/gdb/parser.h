/* 
 * parser.c Copyright (C) 2002
 *        Etay Meiri            <etay-m@bezeqint.net>
 *        Jean-Noel Guiheneuf   <jnoel@saudionline.com.sa>
 *
 * Adapted from kdevelop - gdbparser.cpp  Copyright (C) 1999
 *        by John Birch         <jb.nz@writeme.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _PARSER_H_
#define _PARSER_H_

#include "gdbmi.h"

#include <libanjuta/interfaces/ianjuta-debugger.h>

G_BEGIN_DECLS

IAnjutaDebuggerWatch* gdb_watch_parse (const GDBMIValue *mi_results);
void gdb_watch_free (IAnjutaDebuggerWatch* this);

G_END_DECLS

#endif
