/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
    anjuta-utils.h
    Copyright (C) 2003 Naba Kumar  <naba@gnome.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef __ANJUTA_DEBUG__
#define __ANJUTA_DEBUG__

/**
 * DEBUG_PRINT:
 * 
 * Equivalent to g_message(), except it has only effect when DEBUG is
 * defined. Used for printing debug messages.
 */
#ifdef DEBUG
#  define DEBUG_PRINT g_message
#else
#  define DEBUG_PRINT(...)
#endif

#endif
