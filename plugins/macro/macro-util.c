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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "macro-util.h"
#include <libanjuta/interfaces/ianjuta-document-manager.h>

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
		username = getenv("USERNAME");
	if (!username || strlen(username) == 0)
		username = getenv("USER");
	return username;
}	


static gchar *
get_email(MacroPlugin * plugin)
{
	AnjutaPreferences *prefs;
	gchar *email;
	gchar *username;
	
	prefs = anjuta_shell_get_preferences ((ANJUTA_PLUGIN(plugin))->shell, NULL);
	email = anjuta_preferences_get (prefs, "anjuta.project.email");
	if (!email || strlen(email) == 0)
	{
		email = getenv("HOSTNAME");
		if (!(username = getenv("USERNAME")) || strlen(username) == 0)
			username = getenv("USER");
		email = g_strconcat(username, "@", email, NULL);
		g_free(username);
	}
	return email;
}
	
static IAnjutaEditor*
get_current_editor (AnjutaPlugin *plugin)
{
	IAnjutaEditor *editor;
	IAnjutaDocumentManager* docman;
	docman = anjuta_shell_get_interface (plugin->shell,
										 IAnjutaDocumentManager,
										 NULL);
	editor = ianjuta_document_manager_get_current_editor (docman, NULL);
	return editor;
}

static gchar *
get_filename(MacroPlugin * plugin)
{
	IAnjutaEditor *te;
	gchar *filename;
	
	te = get_current_editor (plugin);
	filename = g_strdup(ianjuta_editor_get_filename (IANJUTA_EDITOR (te), NULL));	

	return filename;
}

static gchar *
get_filename_up(MacroPlugin * plugin)
{
	gchar *name;
	
	gchar *filename = get_filename(plugin);
	name = g_ascii_strup(filename, -1);
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
			_FILE_NAME_UP_PREFIX, _EMAIL, _ENDKEYW };		
	gchar *tabkey[_ENDKEYW] =
		{"@DATE_TIME@", "@DATE_YMD@", "@DATE_Y@", "@USER_NAME@", "@FILE_NAME@",
		 "@FILE_NAME_UP@", "@FILE_NAME_UP_PREFIX@", "@EMAIL@" };
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
		default:
			//*expand = 
			return FALSE;
	}
	
	return TRUE;
}




gchar*
expand_macro(MacroPlugin * plugin, gchar *txt)
{
	gchar *ptr = txt;
	gchar *c = txt;
	gchar *buffer = "";
	gchar *begin;
	gchar *keyword;
	gchar *buf = NULL;
	gchar *expand = NULL;
	
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
	    c++;
	}
    buf = g_strndup(ptr, c - ptr);
    buffer = g_strconcat(buffer, buf, NULL);
    g_free(buf);
    return buffer;
}
