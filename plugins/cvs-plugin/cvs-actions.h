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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef CVS_ACTIONS_H
#define CVS_ACTIONS_H

#include "plugin.h"

void on_cvs_add_activate (GtkAction* action, CVSPlugin* plugin);
void on_cvs_remove_activate (GtkAction* action, CVSPlugin* plugin);

void on_cvs_commit_activate (GtkAction* action, CVSPlugin* plugin);
void on_cvs_update_activate (GtkAction* action, CVSPlugin* plugin);

void on_cvs_diff_file_activate (GtkAction* action, CVSPlugin* plugin);
void on_cvs_diff_tree_activate (GtkAction* action, CVSPlugin* plugin);

void on_cvs_import_activate (GtkAction* action, CVSPlugin* plugin);

#endif
