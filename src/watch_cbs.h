/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    watch_cbs.h
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

#ifndef _WATCH_CBS_H_
#define _WATCH_CBS_H_

#include <gnome.h>
#include "watch.h"

void
on_watch_add_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_watch_remove_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_watch_update_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_watch_clear_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_watch_toggle_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
										
void
on_watch_change_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_watch_help_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_watch_event                (GtkWidget       *widget,
                                        GdkEvent  *event,
                                        gpointer         user_data);

/********************************************************************************/

void on_ew_add_response                (GtkWidget       *dlg, gint res,
                                        gpointer         user_data);

void on_ew_change_response             (GtkWidget       *dlg, gint res,
                                        gpointer         user_data);

/********************************************************************************/
void on_eval_response                  (GtkWidget       *dlg, gint res,
                                        gpointer         user_data);

void on_eval_add_watch                 (GtkButton * button,
                                        gpointer user_data);

/********************************************************************************/

#endif
