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

static gchar *
get_date_time(void)
{
	time_t cur_time = time(NULL);
	gchar *DateTime;

	DateTime = g_new(gchar, 100);
	sprintf(DateTime,ctime(&cur_time));
	return DateTime;
}

static gchar *
get_date_Ymd(void)
{
	gchar *datetime;
	struct tm *lt;
		
	time_t cur_time = time(NULL);
    datetime = g_new(gchar, 20);
	lt = localtime(&cur_time);
	//strftime (datetime, 20, N_("%Y-%m-%d"), lt);
	strftime (datetime, 20, "%Y-%m-%d", lt);
	return datetime;
}
  	
	

static gboolean
expand_keyword(gchar *keyword, gchar **expand)
{
	enum {_DATETIME = 0, _DATE_YMD, _USER_NAME , _FILE_NAME, _EMAIL, 
		  _ENDKEYW };		
	gchar *tabkey[_ENDKEYW] =
		{"@DATE_TIME@", "@DATE_YMD@", "@USER_NAME@", "@FILE_NAME@", "@EMAIL@" };
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
		case _USER_NAME :
			//*expand = 
			break;
		case _FILE_NAME :
			//*expand = 
			break;
		case _EMAIL :
			//*expand = 
			break;
		default:
			//*expand = 
			return FALSE;
	}
	
	return TRUE;
}




gchar*
expand_macro(gchar *txt)
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
				
					if (expand_keyword(keyword, &expand))
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
