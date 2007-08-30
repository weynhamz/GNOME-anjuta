/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    cvs-execute.h
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

#ifndef CVS_EXECUTE_H
#define CVS_EXECUTE_H

#include "plugin.h"

void 
cvs_execute(CVSPlugin* plugin, const gchar* command, const gchar* dir);

void
cvs_execute_status(CVSPlugin* plugin, const gchar* command, const gchar* dir);

void
cvs_execute_diff(CVSPlugin* plugin, const gchar* command, const gchar* dir);

void
cvs_execute_log(CVSPlugin* plugin, const gchar* command, const gchar* dir);

#endif
