/***************************************************************************
 *            file_insert.h
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
	
void file_insert_text(TextEditor *te, gchar *txt, gint offset);

void insert_c_gpl_notice(TextEditor *te);
void insert_cpp_gpl_notice(TextEditor *te);
void insert_py_gpl_notice(TextEditor *te);

void insert_header_template(TextEditor *te);
void insert_date_time(TextEditor *te);
void insert_username(TextEditor *te);
void insert_changelog_entry(TextEditor *te);
void insert_header(TextEditor *te);

void insert_switch_template(TextEditor *te);
void insert_for_template(TextEditor *te);
void insert_while_template(TextEditor *te);
void insert_ifelse_template(TextEditor *te);

void insert_cvs_name(TextEditor *te);
void insert_cvs_author(TextEditor *te);
void insert_cvs_date(TextEditor *te);
void insert_cvs_header(TextEditor *te);
void insert_cvs_id(TextEditor *te);
void insert_cvs_log(TextEditor *te);
void insert_cvs_revision(TextEditor *te);
void insert_cvs_revision(TextEditor *te);
void insert_cvs_source(TextEditor *te);

#ifdef __cplusplus
}
#endif

#endif /* _FILE_INSERT_H */
