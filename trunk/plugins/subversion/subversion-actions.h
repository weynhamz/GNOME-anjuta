/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    cvs-actions.h
    Copyright (C) 2004 Johannes Schmid

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SUBVERSION_ACTIONS_H
#define SUBVERSION_ACTIONS_H

#include "plugin.h"

/* CVS menu callbacks */
void on_menu_subversion_add (GtkAction* action, Subversion* plugin);
void on_menu_subversion_remove (GtkAction* action, Subversion* plugin);
void on_menu_subversion_commit (GtkAction* action, Subversion* plugin);
void on_menu_subversion_update (GtkAction* action, Subversion* plugin);
void on_menu_subversion_diff (GtkAction* action, Subversion* plugin);
void on_menu_subversion_status (GtkAction* action, Subversion* plugin);
void on_menu_subversion_log (GtkAction* action, Subversion* plugin);
void on_menu_subversion_import (GtkAction* action, Subversion* plugin);

/* File manager popup callbacks */
void on_fm_subversion_commit (GtkAction* action, Subversion* plugin);
void on_fm_subversion_update (GtkAction* action, Subversion* plugin);
void on_fm_subversion_diff (GtkAction* action, Subversion* plugin);
void on_fm_subversion_status (GtkAction* action, Subversion* plugin);
void on_fm_subversion_log (GtkAction* action, Subversion* plugin);

#endif
