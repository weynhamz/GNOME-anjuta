/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * shell.h
 * Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _ANJUTA_TEST_SHELL_H_
#define _ANJUTA_TEST_SHELL_H_

#include <glib.h>
#include <gtk/gtkwindow.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-preferences.h>

#define ANJUTA_TYPE_TEST_SHELL        (anjuta_test_shell_get_type ())
#define ANJUTA_TEST_SHELL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_TEST_SHELL, AnjutaTestShell))
#define ANJUTA_TEST_SHELL_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_TEST_SHELL, AnjutaTestShellClass))
#define ANJUTA_IS_TEST_SHELL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_TEST_SHELL))
#define ANJUTA_IS_TEST_SHELL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_TEST_SHELL))

typedef struct _AnjutaTestShell AnjutaTestShell;
typedef struct _AnjutaTestShellClass AnjutaTestShellClass;

struct _AnjutaTestShell
{
	GtkWindow parent;
	GtkWidget *box;
	
	GHashTable *values;
	GHashTable *widgets;
	AnjutaStatus *status;
	AnjutaUI *ui;
	AnjutaPreferences *preferences;
	
	gint merge_id;
};

struct _AnjutaTestShellClass {
	GtkWindowClass klass;
};

GType anjuta_test_shell_get_type (void);
GtkWidget* anjuta_test_shell_new (void);

#endif
