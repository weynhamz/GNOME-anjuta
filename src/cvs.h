/*  cvs.h (C) 2002 Johannes Schmid
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

#ifndef CVS_H
#define CVS_H

#include "project_dbase.h"
#include "properties.h"

#include <time.h>

typedef struct _CVS CVS;
typedef guint ServerType;

/* The server type possible for CVS */
enum
{
	CVS_LOCAL = 0,
	CVS_PASSWORD,
	CVS_EXT,
	CVS_SERVER,
	CVS_GSSAPI,		/* unsupported */
	CVS_FORK,		/* unsupported */
	CVS_END
};

CVS *cvs_new (PropsID p);
void cvs_destroy (CVS * cvs);

void cvs_set_server (CVS * cvs, gchar * server);
void cvs_set_server_type (CVS * cvs, ServerType type);
void cvs_set_directory (CVS * cvs, gchar * directory);
void cvs_set_username (CVS * cvs, gchar * username);
void cvs_set_passwd (CVS * cvs, gchar * passwd);
void cvs_set_compression (CVS * cvs, guint compression);

gchar *cvs_get_server (CVS * cvs);
ServerType cvs_get_server_type (CVS * cvs);
gchar *cvs_get_directory (CVS * cvs);
gchar *cvs_get_username (CVS * cvs);
gchar *cvs_get_passwd (CVS * cvs);
guint cvs_get_compression (CVS * cvs);

/* File functions */
void cvs_update_file (CVS * cvs, gchar * filename, gchar * branch);
void cvs_commit_file (CVS * cvs, gchar * filename, gchar * revision,
		      gchar * message);
void cvs_add_file (CVS * cvs, gchar * filename, gchar * message);
void cvs_remove_file (CVS * cvs, gchar * filename);
void cvs_status_file (CVS * cvs, gchar * filename);
void cvs_diff_file (CVS * cvs, gchar * filename, gchar * rev,
			time_t date, gboolean unified);

void cvs_login (CVS * cvs);


gboolean cvs_save_yourself (CVS * cvs, FILE * stream);

#endif
