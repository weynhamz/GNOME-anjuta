/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    program.h
    Copyright (C) 2008 Sébastien Granjoux

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
 
#ifndef PROGRAM_H
#define PROGRAM_H

#include <glib.h>
#include <libanjuta/interfaces/ianjuta-environment.h>
#include <libanjuta/interfaces/ianjuta-builder.h>

typedef struct _BuildProgram BuildProgram;

struct _BuildProgram
{
	gchar *work_dir;
	gchar **argv;
	gchar **envp;
	
	IAnjutaBuilderCallback callback;
	gpointer user_data;
};

BuildProgram* build_program_new (void);
BuildProgram* build_program_new_with_command (const gchar *directory, const gchar *command,...);
void build_program_free (BuildProgram *proc);

gboolean build_program_set_command (BuildProgram *proc, const gchar *command);
const gchar *build_program_get_basename (BuildProgram *proc);

void build_program_set_working_directory (BuildProgram *proc, const gchar *directory);

gboolean build_program_insert_arg (BuildProgram *proc, gint pos, const gchar *arg);
gboolean build_program_replace_arg (BuildProgram *proc, gint pos, const gchar *arg);
gboolean build_program_remove_arg (BuildProgram *proc, gint pos);

gboolean build_program_add_env (BuildProgram *proc, const gchar *name, const gchar *value);
gboolean build_program_remove_env (BuildProgram *proc, const gchar *name);

void build_program_override (BuildProgram *proc, IAnjutaEnvironment *env);

void build_program_set_callback (BuildProgram *proc, IAnjutaBuilderCallback callback, gpointer user_data);
void build_program_callback (BuildProgram *proc, GObject *sender, IAnjutaBuilderHandle handle, GError *err); 

#endif /* PROGRAM_H */
