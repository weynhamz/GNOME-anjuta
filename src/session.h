/*
 * session.h Copyright (C) 2001  lb
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

#ifndef _SESSION_H_
#define _SESSION_H_

/* This global enum is used to avoid names duplication */
enum _SessionSectionTypes {
	SECTION_FILELIST,
	SECTION_FILENUMBER,
	SECTION_BREAKPOINTS,
	SECTION_FILEMARKERS,
	SECTION_REPLACETEXT,
	SECTION_FINDTEXT,
	SECTION_PROGRAM_ARGUMENTS,
	SECTION_FIND_IN_FILES,
	SECTION_EXECUTERARGS,
	SECTION_EXECUTER,
	SECTION_PROJECTDBASE,
	SECTION_RECENTFILES,
	SECTION_RECENTPROJECTS,
	SECTION_FIND_IN_FILES_SETTINGS,
	SECTION_FIND_IN_FILES_PROFILES,
	SECTION_GENERAL,
	SECTION_PROJECT_TREE,
	SECTION_SYMBOL_TREE,
	SECTION_FILE_TREE,
	SECTION_END
};

typedef enum _SessionSectionTypes SessionSectionTypes ;
	
const gchar	*SessionSectionString( const SessionSectionTypes p_Session );

#define	SECSTR(x)	SessionSectionString( x )

/* Config (gnome) */
void write_config(void);
gchar *GetProfileString( const gchar* szSection, const gchar* szItem, const gchar* szDefault ); 
gboolean GetProfileBool( const gchar* szSection, const gchar* szItem, const gboolean bDefault ); 
gint GetProfileInt( const gchar* szSection, const gchar* szItem, const gint iDefault ); 

gboolean WriteProfileString( const gchar* szSection, const gchar* szItem, const gchar* szValue ); 
gboolean WriteProfileBool( const gchar* szSection, const gchar* szItem, const gboolean bValue ); 
gboolean WriteProfileInt( const gchar* szSection, const gchar* szItem, const gint iValue ); 

/* Session management */
void
session_sync(void);
/* Changed: n is for a vector */
void
session_save_string_n( ProjectDBase * p, const gchar *szSection, const gint nItem, const gchar *szValue );
void
session_save_string( ProjectDBase * p, const gchar *szSection, const gchar *szItem, const gchar *szValue );
void
session_save_long_n( ProjectDBase * p, const gchar *szSection, const gint nItem, const glong lValue );
glong
session_get_long_n( ProjectDBase * p, const gchar *szSection, const gint nItem, const glong lValue );
gchar*
session_get_string_n( ProjectDBase * p, const gchar *szSection, const gint nItem, const gchar *szValue );
gchar*
session_get_string( ProjectDBase * p, const gchar *szSection, const gchar *szItem, const gchar *szValue );
gboolean
session_clear_section( ProjectDBase * p, const gchar *szSection );
gchar * 
GetSessionFile( ProjectDBase * p );
void
session_get_strings( ProjectDBase * p, const gchar *szSection, gint *pnItems, gchar*** argvp );
gpointer
session_get_iterator( ProjectDBase * p, const gchar *szSection );
void
session_save_strings( ProjectDBase *p, const gchar *szSession, GList *pLStrings );

GList*
session_load_strings(ProjectDBase * p, const gchar *szSection, GList *pList );

void
session_save_bool( ProjectDBase * p, const gchar *szSection, const gchar *szItem, const gboolean bVal );

gboolean
session_get_bool( ProjectDBase * p, const gchar *szSection, const gchar *szItem, const gboolean bValDefault );

gboolean
anjuta_save_strings( const gchar *szSection, GList *pLStrings );
gboolean
anjuta_session_clear_section( const gchar *szSection );
GList*
anjuta_session_load_strings( const gchar *szSection, GList *pList );
gpointer
anjuta_session_get_iterator( const gchar *szSection );
gboolean
anjuta_session_save_strings( const gchar *szSection, GList *pLStrings );

// Global Anjuta configuration
#define	GLOBAL_SECTION_SEARCHPATTERNS	"searchpatterns"
GList*
config_load_strings( const gchar *szSection, GList *pList );
gpointer
config_get_iterator( const gchar *szSection );
gboolean
config_copy_strings( const gchar *szSection );

#endif	/*_SESSION_H_*/
