/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
    anjuta-debug.h
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * SECTION:anjuta-debug
 * @title: Debugging
 * @short_description: Debug functions 
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-debug.h
 * 
 * Anjuta debug messages displayed with g_debug() can be filtered based on their
 * domain. By default if DEBUG is defiled, all message are displayed. If DEBUG is not
 * defined, all messages are hidden.
 *
 * This behavior can be changed using the ANJUTA_LOG_DOMAINS environment
 * variable. If this variable is set to "all", all message are displayed whatever is the
 * value of DEBUG. Else you can set it to a list of domains separated by space to 
 * display messages from these selected domains only.
 * If G_LOG_DOMAIN is undefined, it will match a domain named "NULL".
 *
 * By example
 *<programlisting>
 * ANJUTA_LOG_DOMAINS=Gtk Anjuta libanjuta-gdb
 *</programlisting>
 * will display debug messages from Gtk, Anjuta and gdb plugin only.
 */

#ifndef __ANJUTA_DEBUG__
#define __ANJUTA_DEBUG__

/**
 * DEBUG_PRINT:
 * 
 * Equivalent to g_debug() showing the FILE, the LINE and the FUNC,
 * except it has only effect when DEBUG is defined. Used for printing debug 
 * messages.
 */
#if defined (DEBUG)
	#if defined (__GNUC__) && (__GNUC__ >= 3) && !defined(__STRICT_ANSI__)
		#define DEBUG_PRINT(format, ...) g_debug ("%s:%d (%s) " format, __FILE__, __LINE__, G_STRFUNC, ##__VA_ARGS__)
	#else
		#define DEBUG_PRINT g_debug
	#endif
#else
	#define DEBUG_PRINT(...)
#endif

void anjuta_debug_init (void);

#endif /* _ANJUTA_DEBUG_H_ */
