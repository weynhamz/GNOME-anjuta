/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/*
 * macro-util.c (c) 2005 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "macro-util.h"
#include <libanjuta/interfaces/ianjuta-document-manager.h>


static char *get_date_time(void);
static gchar *get_date_Ymd(void);
static gchar *get_date_Y(void);
static gchar *get_username(MacroPlugin * plugin);
static gchar *get_email(MacroPlugin * plugin);
static IAnjutaEditor *get_current_editor (AnjutaPlugin *plugin);
static gchar *get_filename(MacroPlugin * plugin);
static gchar *get_filename_up(MacroPlugin * plugin);
static gchar *get_filename_up_prefix(MacroPlugin * plugin);
static gboolean expand_keyword(MacroPlugin * plugin, gchar *keyword, gchar **expand);


static char *
get_date_time(void)
{
	time_t cur_time = time(NULL);
	gchar *DateTime;
	gchar *buffer;

	DateTime = g_new(gchar, 100);
	sprintf(DateTime,ctime(&cur_time));
	buffer = g_strndup(DateTime, strlen(DateTime) - 1);
	g_free(DateTime);
	return buffer;
}

static gchar *
get_date_Ymd(void)
{
	gchar *datetime;
	struct tm *lt;
		
	time_t cur_time = time(NULL);
	datetime = g_new(gchar, 20);
	lt = localtime(&cur_time);
	/* Macros can expand the current date in the format specified below */
	strftime (datetime, 20, N_("%Y-%m-%d"), lt);
	return datetime;
}
  	
static gchar *
get_date_Y(void)
{
	gchar *datetime;
	struct tm *lt;
		
	time_t cur_time = time(NULL);
	datetime = g_new(gchar, 20);
	lt = localtime(&cur_time);
	/* Macros can expand the year in the format specified below */
	strftime (datetime, 20, N_("%Y"), lt);
	return datetime;
}

static gchar *
get_username(MacroPlugin * plugin)
{
	AnjutaPreferences *prefs;
	gchar *username;
	
	prefs = anjuta_shell_get_preferences ((ANJUTA_PLUGIN(plugin))->shell, NULL);
	username = anjuta_preferences_get (prefs, "anjuta.project.user");
	if (!username || strlen(username) == 0)
		username = anjuta_preferences_get (prefs, "anjuta.user.name");
	if (!username || strlen(username) == 0)
		username = getenv("USERNAME");
	if (!username || strlen(username) == 0)
		username = getenv("USER");
	if (!username || strlen(username) == 0)
		username = "<username>";
	return g_strdup(username);
}	


static gchar *
get_email(MacroPlugin * plugin)
{
	AnjutaPreferences *prefs;
	gchar *email;
	
	prefs = anjuta_shell_get_preferences ((ANJUTA_PLUGIN(plugin))->shell, NULL);
	email = anjuta_preferences_get (prefs, "anjuta.project.email");
	if (!email || strlen(email) == 0)
		email = anjuta_preferences_get (prefs, "anjuta.user.email");
	if (!email || strlen(email) == 0)
	{
		gchar* host = getenv("HOSTNAME");
		gchar* username = get_username(plugin);
		
		if (host == NULL || !strlen(host))
			host = "<host>";
		email = g_strconcat(username, "@", host, NULL);
		g_free(username);
		return email;
	}
	return g_strdup(email);
}
	
static gchar *
get_tab_size(MacroPlugin * plugin)
{
	AnjutaPreferences *prefs;
	gchar *ts;
	
	prefs = anjuta_shell_get_preferences ((ANJUTA_PLUGIN(plugin))->shell, NULL);
	ts = g_strdup_printf("tab-width: %d", anjuta_preferences_get_int (prefs, "tabsize"));
	return ts;
}

static gchar *
get_indent_size(MacroPlugin * plugin)
{
	AnjutaPreferences *prefs;
	gchar *is;
	
	prefs = anjuta_shell_get_preferences ((ANJUTA_PLUGIN(plugin))->shell, NULL);
	is = g_strdup_printf("c-basic-offset: %d", anjuta_preferences_get_int (prefs, "indent.size"));
	return is;
}

static gchar *
get_use_tabs(MacroPlugin * plugin)
{
	AnjutaPreferences *prefs;
	gchar *ut;
	
	prefs = anjuta_shell_get_preferences ((ANJUTA_PLUGIN(plugin))->shell, NULL);
	if (anjuta_preferences_get_int (prefs, "use.tabs"))
		ut = g_strdup("indent-tabs: t");
	else
		ut = g_strdup("");
	return ut;
}

static IAnjutaEditor*
get_current_editor (AnjutaPlugin *plugin)
{
	IAnjutaDocumentManager* docman;
	docman = anjuta_shell_get_interface (plugin->shell,
										 IAnjutaDocumentManager,
										 NULL);
	if (docman)
	{
		IAnjutaDocument* doc = ianjuta_document_manager_get_current_document (docman, NULL);
		if (doc && IANJUTA_IS_EDITOR(doc))
			return IANJUTA_EDITOR(doc);
	}
	return NULL;
}

static gchar *
get_filename(MacroPlugin * plugin)
{
	IAnjutaEditor *te;
	gchar *filename;
	
	te = get_current_editor (ANJUTA_PLUGIN(plugin));
	if (te != NULL)
		filename = g_strdup(ianjuta_document_get_filename (IANJUTA_DOCUMENT (te), NULL));	
	else
		filename = g_strdup("<filename>");

	return filename;
}

static gchar *
get_filename_up(MacroPlugin * plugin)
{
	gchar *name;
	gchar* score;
	gchar *filename = get_filename(plugin);
	name = g_ascii_strup(filename, -1);
	
	/* Fix bug #333606 and bug #419036,
	 * Replace all invalid C character by underscore */
	for (score = name; *score != '\0'; score++)
		if (!g_ascii_isalnum (*score)) *score = '_';
	
	g_free(filename);
	return name;
}

static gchar *
get_filename_up_prefix(MacroPlugin * plugin)
{
	gchar *name;
	gchar *filename = get_filename_up(plugin);
	name = g_strndup(filename, strlen(filename) - 2);
	g_free(filename);
	return name;
}


static gboolean
expand_keyword(MacroPlugin * plugin, gchar *keyword, gchar **expand)
{
	enum {_DATETIME = 0, _DATE_YMD, _DATE_Y, _USER_NAME , _FILE_NAME,  _FILE_NAME_UP, 
			_FILE_NAME_UP_PREFIX, _EMAIL, _TABSIZE, _INDENTSIZE,
		    _USETABS, _ENDKEYW };		
	gchar *tabkey[_ENDKEYW] =
		{"@DATE_TIME@", "@DATE_YMD@", "@DATE_Y@", "@USER_NAME@", "@FILE_NAME@",
		 "@FILE_NAME_UP@", "@FILE_NAME_UP_PREFIX@", "@EMAIL@" , "@TABSIZE@",
		 "@INDENTSIZE@", "@USETABS@"};
	gint key;
		
	for (key=0; key<_ENDKEYW; key++)
		if ( strcmp(keyword, tabkey[key]) == 0)
			break;
		
	switch (key)
	{
		case _DATETIME :
		    *expand = get_date_time();
		    break;
		case _DATE_YMD :
		    *expand = get_date_Ymd();
		    break;
		case _DATE_Y :
		    *expand = get_date_Y();
		    break;
		case _USER_NAME :
			*expand = get_username(plugin);
			break;
		case _FILE_NAME :
			*expand = get_filename(plugin);
			break;
		case _FILE_NAME_UP :
			*expand = get_filename_up(plugin);
			break;
		case _FILE_NAME_UP_PREFIX :
			*expand = get_filename_up_prefix(plugin);
			break;
		case _EMAIL :
			*expand = get_email(plugin);
			break;
		case _TABSIZE :
			*expand = get_tab_size(plugin);
			break;
		case _INDENTSIZE :
			*expand = get_indent_size(plugin);
			break;
		case _USETABS :
			*expand = get_use_tabs(plugin);
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}




gchar*
expand_macro(MacroPlugin * plugin, gchar *txt, gint *offset)
{
	gchar *ptr = txt;
	gchar *c = txt;
	gchar *buffer = "";
	gchar *begin;
	gchar *keyword;
	gchar *buf = NULL;
	gchar *expand = NULL;
	gboolean found_curs = FALSE;
	
	while ( *(c) )
	{
		if ( *c =='@' )
		{
			begin = c++;
			while ( *(c) )
			{
				if ( *c==' ')
				   break;
				if ( *c=='@' )
				{
					keyword = g_strndup(begin, c-begin+1);
				
					if (expand_keyword(plugin, keyword, &expand))
					{
						buf = g_strndup(ptr, begin - ptr);
						buffer = g_strconcat(buffer, buf, expand, NULL);
						g_free(expand);
					}
					else
					{
						buf = g_strndup(ptr, c - ptr + 1);
						buffer = g_strconcat(buffer, buf, NULL);
					}
					g_free(buf);
			        ptr = c + 1;
			       break;
				}	
				c++;
			}
		}
		/* Move the cursor at '|' position */
		else if ( !found_curs && *c=='|' )		
		{
			found_curs = TRUE;
			buf = g_strndup(ptr, c - ptr);

			buffer = g_strconcat(buffer, buf, NULL);
			*offset = strlen(buffer);
			ptr = c + 1;	
		}    
		c++;
	}
    buf = g_strndup(ptr, c - ptr);
    buffer = g_strconcat(buffer, buf, NULL);
	
    g_free(buf);
    return buffer;
}
