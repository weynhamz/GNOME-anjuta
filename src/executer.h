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
typedef struct _ExecuterGUI ExecuterGUI;
		
struct _ExecuterGUI {
	GtkWidget *dialog;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GtkWidget *check_terminal;
} ;

struct _Executer
{
	GladeXML *gxml;
	PropsID		props;
	gboolean    terminal;
	GList		*m_PgmArgs;	/* The program arguments */
	/* UI */
	ExecuterGUI	m_gui;
};

#define EXECUTER_PROGRAM_ARGS_KEY "anjuta.program.arguments"

Executer* executer_new(PropsID props);

/* Executer is auto hide */
void executer_show (Executer *e);
void executer_destroy (Executer *e);
void executer_execute (Executer *e);

void executer_save_session( Executer *e, ProjectDBase *p );
void executer_load_session( Executer *e, ProjectDBase *p );

#endif
