/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
    anjuta-debug.c
    Copyright (C) 2008 Sébastien Granjoux  <seb.sfo@free.fr>

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

#include "anjuta-debug.h"

#include <glib.h>

#include <string.h>

static gchar **anjuta_log_modules = NULL;

static void
anjuta_log_handler (const char *log_domain, 
					GLogLevelFlags log_level,
					const char *message,
					gpointer user_data)
{
	
	if (log_level & G_LOG_LEVEL_DEBUG)
	{
		/* Filter only debugging messages */
		gint i;
		const gchar *domain;

		/* Do not display anything if anjuta_log_modules is empty
		 * (happens only in non debugging mode) */
		if (anjuta_log_modules == NULL) return;

		/* log_domain can be NULL*/
		domain = (log_domain == NULL) || (*log_domain == '\0') ? "NULL" : log_domain;
		
		for (i = 0; anjuta_log_modules[i] != NULL; i++)
		{
			if (strcmp(domain, anjuta_log_modules[i]) == 0)
			{
				/* Display message */
				g_log_default_handler (log_domain, log_level, message, user_data);
				return;
			}
		}
		
		return;
	}

	g_log_default_handler (log_domain, log_level, message, user_data);
}

/**
 * anjuta_debug_init :
 * 
 * Initialize filtering of debug messages.
 */
void
anjuta_debug_init (void)
{
	const gchar *log;
	gboolean all = FALSE;
	
	log = g_getenv ("ANJUTA_LOG_DOMAINS");
	if (log != NULL)
	{
		anjuta_log_modules = g_strsplit_set (log, ": ", -1);
		
		if (anjuta_log_modules != NULL)
		{
			gint i;
			
			for (i = 0; anjuta_log_modules[i] != NULL; i++)
			{
				if (strcmp("all", anjuta_log_modules[i]) == 0)
				{
					all = TRUE;
					break;
				}
			}
		}
	}

#ifdef DEBUG
	if ((anjuta_log_modules != NULL) && (anjuta_log_modules[0] != NULL) && !all)
	{
		/* Display selected message in debugging mode if environment
		 * variable is no set to all */
#else
	if (!all)
	{
		/* Display selected message in non debugging mode if the
		 * environment variable doesn't exist or if not set to all */
#endif		
		g_log_set_default_handler (anjuta_log_handler, NULL);
	}
}
