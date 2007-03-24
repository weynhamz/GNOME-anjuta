/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    utilities.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib.h>

#include <gtk/gtk.h>
#include <gnome.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>
#include "plugin.h"
#include "utilities.h"

#define ICON_FILE "anjuta-gdb.plugin.png"
#define	SRCH_CHAR	'\\'

static int
get_hex_as (const gchar c)
{
	if (isdigit (c))
		return c - '0';
	else
		return toupper (c) - 'A' + 10;
}

static gchar
get_hex_b (const gchar c1, const gchar c2)
{
	return get_hex_as (c1) * 16 + get_hex_as(c2);
}

gchar*
gdb_util_get_str_cod (const gchar *szIn)
{
	gchar	*szRet ;
	g_return_val_if_fail( NULL != szIn, NULL );
	szRet = g_malloc( strlen( szIn )+2 );
	if( NULL != szRet )
	{
		gchar*	szDst = szRet ;
		while( szIn[0] )
		{
			if( SRCH_CHAR == szIn[0] )
			{
				if( SRCH_CHAR == szIn[1] )
				{
					*szDst++ = *szIn ++ ;
					szIn ++ ;
				} else
				{
					*szDst ++ = get_hex_b (szIn[1], szIn[2]);
					szIn += 3;
				}
			} else
			{
				*szDst++ = *szIn ++ ;
			}
		}
		szDst [0] = '\0' ;
	}
	return szRet ;
}

gboolean
gdb_util_parse_error_line (const gchar * line, gchar ** filename, guint *lineno)
{
	gint i = 0;
	gint j = 0;
	gint k = 0;
	gchar *dummy;

	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			goto down;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = strtoul (dummy, NULL, 10);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (line, j - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}

      down:
	i = strlen (line) - 1;
	while (isspace (line[i]) == FALSE)
	{
		i--;
		if (i < 0)
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
	k = i++;
	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = strtoul (dummy, NULL, 10);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (&line[k], j - k - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}
	*lineno = 0;
	*filename = NULL;
	return FALSE;
}

gchar *
gdb_util_remove_white_spaces (const gchar * text)
{
	guint src_count, dest_count, tab_count;
	gchar buff[2048];	/* Let us hope that it does not overflow */
	
	tab_count = 8;
	dest_count = 0;
	
	for (src_count = 0; src_count < strlen (text); src_count++)
	{
		if (text[src_count] == '\t')
		{
			gint j;
			for (j = 0; j < tab_count; j++)
				buff[dest_count++] = ' ';
		}
		else if (isspace (text[src_count]))
		{
			buff[dest_count++] = ' ';
		}
		else
		{
			buff[dest_count++] = text[src_count];
		}
	}
	buff[dest_count] = '\0';
	return g_strdup (buff);
}

GList *
gdb_util_remove_blank_lines (const GList * lines)
{
	GList *list, *node;
	gchar *str;

	if (lines)
		list = g_list_copy ((GList*)lines);
	else
		list = NULL;

	node = list;
	while (node)
	{
		str = node->data;
		node = g_list_next (node);
		if (!str)
		{
			list = g_list_remove (list, str);
			continue;
		}
		if (strlen (g_strchomp (str)) < 1)
			list = g_list_remove (list, str);
	}
	return list;
}

/* Excluding the final 0 */
gint gdb_util_calc_string_len( const gchar *szStr )
{
	if( NULL == szStr )
		return 0;
	return strlen( szStr )*3 ;	/* Leave space for the translated character */
}

gint gdb_util_calc_gnum_len(void)
{
	return 24 ;	/* size of a stringfied integer */
}

/* Allocates a struct of pointers */
gchar **
gdb_util_string_parse_separator (const gint nItems, gchar *szStrIn,
								 const gchar chSep)
{
	gchar	**szAllocPtrs = (char**)g_new( gchar*, nItems );
	if( NULL != szAllocPtrs )
	{
		int			i ;
		gboolean	bOK = TRUE ;
		gchar		*p = szStrIn ;
		for( i = 0 ; i < nItems ; i ++ )
		{
			gchar		*szp ;
			szp = strchr( p, chSep ) ;
			if( NULL != szp )
			{
				szAllocPtrs[i] = p ;
				szp[0] = '\0' ;	/* Parse Operation */
				p = szp + 1 ;
			} else
			{
				bOK = FALSE ;
				break;
			}
		}
		if( ! bOK )
		{
			g_free( szAllocPtrs );
			szAllocPtrs = NULL ;
		}
	}
	return szAllocPtrs ;
}

gint
gdb_util_kill_process (pid_t process_id, const gchar* signal)
{
	int status;
	gchar *pid_str;
	pid_t pid;
	
	pid_str = g_strdup_printf ("%d", process_id);
	pid = fork();
	if (pid == 0)
	{
		execlp ("kill", "kill", "-s", signal, pid_str, NULL);
		g_warning (_("Cannot execute command: \"%s\""), "kill");
		_exit(1);
	}
	g_free (pid_str);
	if (pid > 0) {
		waitpid (pid, &status, 0);
		return 0;
	} else {
		return -1;
	}
}

/* Check which gnome-terminal is installed
 Returns: 0 -- No gnome-terminal
 Returns: 1 -- Gnome1 gnome-terminal
 Returns: 2 -- Gnome2 gnome-terminal */
gint 
gdb_util_check_gnome_terminal (void)
{
#ifdef DEBUG
    gchar* term_command = "gnome-terminal --version";
    gchar* term_command2 = "gnome-terminal --disable-factory --version";
#else
    gchar* term_command = "gnome-terminal --version > /dev/null 2> /dev/null";
    gchar* term_command2 = "gnome-terminal --disable-factory --version > /dev/null 2> /dev/null";
#endif
    gint retval;
    
    retval = system (term_command);
    
    /* Command failed or gnome-terminal not found */
    if (WEXITSTATUS(retval) != 0)
        return 0;
    
    /* gnome-terminal found: Determine version 1 or 2 */
    retval = system (term_command2);
    
    /* Command failed or gnome-terminal-2 not found */
    if (WEXITSTATUS(retval) != 0)
        return 1;
    
    /* gnome-terminal-2 found */
    return 2;
}

/* Debugger message manager management */

#if 0
static const gchar * MESSAGE_VIEW_TITLE = N_("Debug");

static void
on_gdb_util_mesg_view_destroy(GdbPlugin* plugin, gpointer destroyed_view)
{
	plugin->mesg_view = NULL;
}

static void
on_gdb_util_debug_buffer_flushed (IAnjutaMessageView *view, const gchar* line,
								  AnjutaPlugin *plugin)
{
	g_return_if_fail (line != NULL);

	IAnjutaMessageViewType type = IANJUTA_MESSAGE_VIEW_TYPE_NORMAL;
	ianjuta_message_view_append (view, type, line, "", NULL);
}

static void
on_gdb_util_debug_mesg_clicked (IAnjutaMessageView* view, const gchar* line,
					   AnjutaPlugin* plugin)
{
	/* FIXME: Parse the given line */
}

static IAnjutaMessageView *
gdb_util_get_message_view (AnjutaPlugin *plugin)
{
	GObject *obj;
	IAnjutaMessageView *message_view;
	IAnjutaMessageManager *message_manager = NULL;
	GdbPlugin *gdb_plugin = ANJUTA_PLUGIN_GDB (plugin);

	g_return_val_if_fail (plugin != NULL, NULL);

	if (gdb_plugin->mesg_view)
		return gdb_plugin->mesg_view;
	
	/* TODO: error checking */
	obj = anjuta_shell_get_object (plugin->shell, "IAnjutaMessageManager",
								   NULL);
	message_manager = IANJUTA_MESSAGE_MANAGER (obj);
	message_view = ianjuta_message_manager_add_view (
			message_manager, MESSAGE_VIEW_TITLE, ICON_FILE, NULL);
	g_object_weak_ref (G_OBJECT (message_view), 
					  (GWeakNotify)on_gdb_util_mesg_view_destroy, plugin);
	g_signal_connect (G_OBJECT (message_view), "buffer-flushed",
			G_CALLBACK (on_gdb_util_debug_buffer_flushed), plugin);
	g_signal_connect (G_OBJECT (message_view), "message-clicked",
			G_CALLBACK (on_gdb_util_debug_mesg_clicked), plugin);
	ianjuta_message_manager_set_current_view (message_manager, message_view,
											  NULL);
	gdb_plugin->mesg_view = message_view;
	
	return message_view;
}

void
gdb_util_append_message (AnjutaPlugin *plugin, const gchar* message)
{
	IAnjutaMessageView *message_view = NULL;

	g_return_if_fail (plugin != NULL);

	/* TODO: error checking */
	message_view = gdb_util_get_message_view (plugin);
	ianjuta_message_view_buffer_append (message_view, message, NULL);
}

void
gdb_util_show_messages (AnjutaPlugin *plugin)
{
	GObject *obj;
	IAnjutaMessageManager *message_manager = NULL;
	IAnjutaMessageView *message_view = NULL;

	/* TODO: error checking */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
			"IAnjutaMessageManager", NULL);
	message_manager = IANJUTA_MESSAGE_MANAGER (obj);
	message_view = gdb_util_get_message_view (plugin);
	ianjuta_message_manager_set_current_view (message_manager, message_view,
											  NULL);
}

void
gdb_util_clear_messages (AnjutaPlugin *plugin)
{
	IAnjutaMessageView *message_view = NULL;

	g_return_if_fail (plugin != NULL);

	/* TODO: error checking */
	message_view = gdb_util_get_message_view (plugin);
	ianjuta_message_view_clear (message_view, NULL);
}
#endif
