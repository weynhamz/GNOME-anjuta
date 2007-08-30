/*  
 *  macro-actions.h (c) 2005 Johannes Schmid
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef MACRO_ACTIONS_H
#define MACRO_ACTIONS_H

#include "plugin.h"
#include <glade/glade.h>

void on_menu_insert_macro (GtkAction * action, MacroPlugin * plugin);
void on_menu_add_macro (GtkAction * action, MacroPlugin * plugin);
void on_menu_manage_macro (GtkAction * action, MacroPlugin * plugin);

#endif
