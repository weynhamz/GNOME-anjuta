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
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

void display_new_file(IAnjutaDocumentManager *docman);	
//~ void clear_new_file(void);	
	
void file_insert_text(IAnjutaEditor *te, gchar *txt, gint offset);

void insert_c_gpl_notice(IAnjutaEditor *te);
void insert_cpp_gpl_notice(IAnjutaEditor *te);
void insert_py_gpl_notice(IAnjutaEditor *te);

void insert_header_template(IAnjutaEditor *te);
void insert_date_time(IAnjutaEditor *te);
void insert_username(IAnjutaEditor *te, AnjutaPreferences *prefs);
void insert_changelog_entry(IAnjutaEditor *te, AnjutaPreferences *prefs);
void insert_header(IAnjutaEditor *te, AnjutaPreferences *prefs);

void insert_switch_template(IAnjutaEditor *te);
void insert_for_template(IAnjutaEditor *te);
void insert_while_template(IAnjutaEditor *te);
void insert_ifelse_template(IAnjutaEditor *te);

void insert_cvs_name(IAnjutaEditor *te);
void insert_cvs_author(IAnjutaEditor *te);
void insert_cvs_date(IAnjutaEditor *te);
void insert_cvs_header(IAnjutaEditor *te);
void insert_cvs_id(IAnjutaEditor *te);
void insert_cvs_log(IAnjutaEditor *te);
void insert_cvs_revision(IAnjutaEditor *te);
void insert_cvs_revision(IAnjutaEditor *te);
void insert_cvs_source(IAnjutaEditor *te);

#ifdef __cplusplus
}
#endif

#endif /* _FILE_INSERT_H */
