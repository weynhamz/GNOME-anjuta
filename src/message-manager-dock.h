/*
    message-manager-dock.h
    Copyright (C) 2000, 2001  Kh. Naba Kumar Singh, Johannes Schmid

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _MESSAGE_MANAGER_DOCK_H
#define _MESSAGE_MANAGER_DOCK_H

#include "preferences.h"

#ifdef __cplusplus
extern "C" 
{
#endif

void 
amm_dock(GtkWidget* amm, GtkWidget** window);

void 
amm_undock(GtkWidget* amm, GtkWidget** window);
	
void
amm_hide_docked (void);
	
void
amm_show_docked (void);
	
Preferences* get_preferences (void);

#ifdef __cplusplus
}
#endif

#endif
