/*  
 *  macro-actions.c (c) 2005 Johannes Schmid
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

#include "macro-actions.h"
#include "macro-db.h"
#include "macro-dialog.h"


void
on_menu_insert_macro (GtkAction * action, MacroPlugin * plugin)
{
	if (plugin->macro_dialog == NULL)
		plugin->macro_dialog = macro_dialog_new (plugin);
	gtk_widget_show (plugin->macro_dialog);
}
