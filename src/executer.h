/*
    executer.h
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
#ifndef _EXECUTE_H_
#define _EXECUTE_H_

typedef struct _Executer Executer;

struct _Executer
{
	PropsID props;
	gboolean    terminal;
};

#define EXECUTER_PROGRAM_ARGS_KEY "anjuta.program.arguments"

Executer* executer_new(PropsID props);

/* Executer is auto hide */
void executer_show(Executer*);

void executer_destroy(Executer*);
void executer_execute (Executer * e);

#endif

