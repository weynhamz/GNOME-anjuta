/*
 *  macro_db.h (c) 2005 Johannes Schmid
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

#ifndef MACRO_DB_H
#define MACRO_DB_H

typedef struct _MacroDB MacroDB;
typedef struct _MacroDBClass MacroDBClass;

#include "plugin.h"

#define MACRO_DB_TYPE            (macro_db_get_type ())
#define MACRO_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MACRO_DB_TYPE, MacroDB))
#define MACRO_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MACRO_DB_TYPE, MacroDBClass))
#define IS_MACRO_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MACRO_DB_TYPE))
#define IS_MACRO_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MACRO_DB_TYPE))

struct _MacroDB
{
	GObject object;

	GtkTreeStore *tree_store;
	GtkTreeIter iter_pre;
	GtkTreeIter iter_user;
};

struct _MacroDBClass
{
	GObjectClass klass;

};

enum
{
	MACRO_NAME = 0,
	MACRO_CATEGORY,
	MACRO_SHORTCUT,
	MACRO_TEXT,
	MACRO_PREDEFINED,
	MACRO_IS_CATEGORY,
	MACRO_N_COLUMNS
};

GType macro_db_get_type (void);
MacroDB *macro_db_new (void);

void macro_db_save (MacroDB * db);
GtkTreeModel *macro_db_get_model (MacroDB * db);

void macro_db_add (MacroDB * db,
				   const gchar * name,
				   const gchar * category,
				   const gchar * shortcut, const gchar * text);

void macro_db_change (MacroDB * db, GtkTreeIter * iter,
					  const gchar * name,
					  const gchar * category,
					  const gchar * shortcut, const gchar * text);

void macro_db_remove (MacroDB * db, GtkTreeIter * iter);

gchar* macro_db_get_macro(MacroDB * db, GtkTreeIter* iter);

#endif
