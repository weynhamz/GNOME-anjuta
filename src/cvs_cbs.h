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

#include "cvs_gui.h"

#include <gnome.h>

#ifndef CVS_CBS_H
#define CVS_CBS_H

/* CVS Settings */
void on_cvs_login_dialog_response (GtkWidget *dialog, gint response,
                                   CVSLoginGUI *gui);

/* CVS File Dialog */
void on_cvs_dialog_response (GtkWidget *dialog, gint response,
                             CVSFileGUI *gui);

/* CVS File Diff Dialog */
void on_cvs_diff_dialog_response (GtkWidget *dialog, gint response,
                                  CVSFileDiffGUI *gui);

/* CVS Import Dialog */
void on_cvs_import_dialog_response (GtkWidget *dialog, gint response,
                                    CVSImportGUI *gui);

void on_cvs_type_combo_changed (GtkWidget *entry, CVSImportGUI *gui);
void on_cvs_diff_use_date_toggled (GtkToggleButton *b, CVSFileDiffGUI *gui);

#endif
