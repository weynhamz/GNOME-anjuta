/*
 *  macro_dialog.h (c) 2005 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "plugin.h"
#include "macro-db.h"

#ifndef MACRO_DIALOG_H
#define MACRO_DIALOG_H

#define MACRO_DIALOG_TYPE            (macro_dialog_get_type ())
#define MACRO_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MACRO_DIALOG_TYPE, MacroDialog))
#define MACRO_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MACRO_DIALOG_TYPE, MacroDialogClass))
#define IS_MACRO_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MACRO_DIALOG_TYPE))
#define IS_MACRO_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MACRO_DIALOG_TYPE))


typedef struct _MacroDialog MacroDialog;
typedef struct _MacroDialogClass MacroDialogClass;

struct _MacroDialog
{
	GtkDialog dialog;

	GtkWidget *details_label;
	GtkWidget *preview_text;
	GtkWidget *macro_tree;

	MacroDB *macro_db;
	MacroPlugin *plugin;
	GladeXML *gxml;
};

struct _MacroDialogClass
{
	GtkDialogClass klass;

};

GType macro_dialog_get_type (void);
GtkWidget *macro_dialog_new (MacroPlugin * plugin);

#endif
