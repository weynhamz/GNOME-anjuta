/*  cvs_gui.h (c) Johannes Schmid 2002
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

#ifndef CVS_GUI_H
#define CVS_GUI_H

#include "cvs.h"

typedef struct _CVSFileGUI CVSFileGUI;
typedef struct _CVSFileDiffGUI CVSFileDiffGUI;
typedef struct _CVSLoginGUI CVSLoginGUI;
typedef struct _CVSImportGUI CVSImportGUI;

/* 
	This is used to tell create_cvs_file_gui which dialog it should
	dialog should come up
*/

enum
{
	CVS_ACTION_UPDATE,
	CVS_ACTION_COMMIT,
	CVS_ACTION_STATUS,
	CVS_ACTION_LOG,
	CVS_ACTION_ADD,
	CVS_ACTION_REMOVE
};

struct _CVSFileGUI
{
	GtkWidget* dialog;
	
	GtkWidget* entry_file;
	GtkWidget* entry_branch;
	GtkWidget* text_message; /* only in Commit Dialog */
	
	GtkWidget* action_button;
	GtkWidget* cancel_button;
	
	int type;
};

struct _CVSLoginGUI
{
	GtkWidget* dialog;
	
	GtkWidget* combo_type;
	GtkWidget* entry_server;
	GtkWidget* entry_dir;
	GtkWidget* entry_user;
};

struct _CVSImportGUI
{
	GtkWidget* dialog;
	
	GtkWidget* combo_type;
	GtkWidget* entry_server;
	GtkWidget* entry_user;
	GtkWidget* entry_dir;
	
	GtkWidget* entry_module;
	GtkWidget* entry_vendor;
	GtkWidget* entry_release;
	GtkWidget* text_message;
	
};

struct _CVSFileDiffGUI
{
	GtkWidget* dialog;
	
	GtkWidget* entry_file;
	GtkWidget* entry_date;
	GtkWidget* entry_rev;
	
	GtkWidget* diff_button;
	GtkWidget* cancel_button;
};

extern gchar *server_types[4];

void create_cvs_login_gui (CVS * cvs);

void create_cvs_gui (CVS * cvs, int dialog_type, gchar* filename, gboolean bypass_dialog);

void create_cvs_diff_gui (CVS * cvs, gchar* filename, gboolean bypass_dialog);

void create_cvs_import_gui (CVS* cvs);

#endif
