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

void cvs_set_force_update (CVS * cvs, gboolean force_update);
void cvs_set_unified_diff (CVS * cvs, gboolean unified_diff);
void cvs_set_context_diff (CVS * cvs, gboolean context_diff);
void cvs_set_compression (CVS * cvs, guint compression);
void cvs_set_diff_use_date(CVS * cvs, gboolean state);

gboolean cvs_get_force_update(CVS * cvs);
gboolean cvs_get_unified_diff(CVS* cvs);
gboolean cvs_get_context_diff(CVS* cvs);
guint cvs_get_compression (CVS * cvs);
gboolean cvs_get_diff_use_date(CVS * cvs);

/* Command functions */
void cvs_update (CVS * cvs, gchar * filename, gchar * branch, gboolean is_dir);
void cvs_commit (CVS * cvs, gchar * filename, gchar * revision,
		      gchar * message, gboolean is_dir);
void cvs_import_project (CVS * cvs, ServerType type, gchar* server,
			gchar* dir, gchar* user, gchar* module, gchar* release,
			gchar* vendor, gchar* message);
void cvs_status (CVS * cvs, gchar * filename, gboolean is_dir);
void cvs_log (CVS * cvs, gchar * filename, gboolean is_dir);
void cvs_diff (CVS * cvs, gchar * filename, gchar * rev,
			time_t date, gboolean is_dir);
void cvs_add_file (CVS * cvs, gchar * filename, gchar * message);
void cvs_remove_file (CVS * cvs, gchar * filename);

void cvs_login (CVS * cvs, ServerType type, gchar* server, 
			gchar* dir, gchar* user);

void cvs_set_editor_destroyed (CVS* cvs);

void cvs_apply_preferences(CVS *cvs, PropsID p);
gboolean cvs_save_yourself (CVS * cvs, FILE * stream);

/* Checks whether there exists a file CVS/Entries under this directory */
gboolean is_cvs_active_for_dir(const gchar *dir);

#endif
