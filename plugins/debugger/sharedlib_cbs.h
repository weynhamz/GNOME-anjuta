/*
    sharedlib_cbs.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

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

#ifndef _SHAREDLIBS_CBS_H_
#define _SHAREDLIBS_CBS_H_

gint
on_sharedlibs_delete_event(GtkWidget* w, GdkEvent* event, gpointer data);

void
on_sharedlibs_update_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
void
on_sharedlibs_help_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
gboolean
on_sharedlibs_event                (GtkWidget       *widget,
                                        GdkEvent  *event,
                                        gpointer         user_data);
#endif
