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
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-macro.h>
#include <plugin.h>

void display_new_file(AnjutaFileWizardPlugin *plugin,
					  IAnjutaDocumentManager *docman);	
gboolean on_new_file_cancelbutton_clicked(GtkWidget *window, GdkEvent *event,
			                              gboolean user_data);
gboolean on_new_file_okbutton_clicked(GtkWidget *window, GdkEvent *event,
			                          gboolean user_data);	
void on_new_file_entry_changed (GtkEditable *entry, gpointer user_data);	
void on_new_file_type_changed (GtkOptionMenu   *optionmenu, gpointer user_data);	
void on_new_file_license_toggled(GtkToggleButton *button, gpointer user_data);

typedef enum _Lge
{
	LGE_C,
	LGE_HC,
	LGE_CPLUS,
	LGE_CSHARP,
	LGE_JAVA,
	LGE_PERL,
	LGE_PYTHON,
	LGE_SHELL
} Lge;

typedef enum _Cmt
{
	CMT_C,
	CMT_CPP,
	CMT_P
} Cmt;

#ifdef __cplusplus
}
#endif

#endif /* _FILE_INSERT_H */
