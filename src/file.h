/***************************************************************************
 *            file.h
 *
 *  Sun Nov 30 17:45:43 2003
 *  Copyright  2003  Jean-Noel Guiheneuf
 *  jnoel@lotuscompounds.com
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
 #ifndef _FILE_INSERT_H
#define _FILE_INSERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <gnome.h>

	
void display_new_file(void);	
//~ void clear_new_file(void);	
	
void file_insert_text(gchar *txt, gint offset);
	
void file_insert_c_gpl_notice(void);	
void file_insert_cpp_gpl_notice(void);
void file_insert_py_gpl_notice(void);

void file_insert_header_template(void);
void file_insert_date_time(void);
void file_insert_username(void);
void file_insert_changelog_entry(void);
void file_insert_header(void);

void file_insert_switch_template(void);
void file_insert_for_template(void);
void file_insert_while_template(void);
void file_insert_ifelse_template(void);

void file_insert_cvs_author(void);
void file_insert_cvs_date(void);
void file_insert_cvs_header(void);
void file_insert_cvs_id(void);
void file_insert_cvs_log(void);
void file_insert_cvs_revision(void);
void file_insert_cvs_name(void);
void file_insert_cvs_source(void);

#ifdef __cplusplus
}
#endif

#endif /* _FILE_INSERT_H */
