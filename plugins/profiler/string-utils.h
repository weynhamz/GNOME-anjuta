/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * string-utils.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * string-utils.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

#include <glib.h>

gchar *read_to_whitespace(gchar *buffer, int *end_pos, int start_pos);
gchar *read_to_delimiter(gchar *buffer, gchar *delimiter);
gchar *strip_whitespace(gchar *buffer);

#endif
