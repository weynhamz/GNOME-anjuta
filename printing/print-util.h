/*
 * print-util.h
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

#ifndef AN_PRINTING_PRINT_UTIL_H
#define AN_PRINTING_PRINT_UTIL_H

#include "print.h"

#ifdef __cplusplus
extern "C"
{
#endif

int  anjuta_print_unichar_to_utf8(gint c, gchar * outbuf);
void anjuta_print_progress_start(PrintJobInfo *pji);
void anjuta_print_progress_tick(PrintJobInfo *pji, guint idx);
void anjuta_print_progress_end(PrintJobInfo *pji);

#ifdef __cplusplus
}
#endif

#endif /* AN_PRINTING_PRINT_UTIL_H */
