/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta_trunk
 * Copyright (C) Massimo Cora' 2008 <maxcvs@email.it>
 * 
 * anjuta_trunk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta_trunk is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYMBOL_DB_PREFS_H_
#define _SYMBOL_DB_PREFS_H_

#include <glib-object.h>


G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_PREFS             (sdb_prefs_get_type ())
#define SYMBOL_DB_PREFS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_PREFS, SymbolDBPrefs))
#define SYMBOL_DB_PREFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_PREFS, SymbolDBPrefsClass))
#define SYMBOL_IS_DB_PREFS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_PREFS))
#define SYMBOL_IS_DB_PREFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_PREFS))
#define SYMBOL_DB_PREFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_PREFS, SymbolDBPrefsClass))

typedef struct _SymbolDBPrefsClass SymbolDBPrefsClass;
typedef struct _SymbolDBPrefs SymbolDBPrefs;
typedef struct _SymbolDBPrefsPriv SymbolDBPrefsPriv;

#include "plugin.h"
#include "symbol-db-system.h"


#define CTAGS_PREFS_KEY		"preferences_entry:text:/usr/bin/ctags:0:symboldb.ctags"
#define PROJECT_AUTOSCAN	"preferences_toggle:bool:1:1:symboldb.scan_prj_pkgs"
#define PARALLEL_SCAN		"preferences_toggle:bool:1:1:symboldb.parallel_scan"
#define BUFFER_AUTOSCAN		"preferences_toggle:bool:1:1:symboldb.buffer_update"

struct _SymbolDBPrefsClass
{
	GObjectClass parent_class;
	
	/* signals */
	void (* package_add) 			(const gchar *package);
	void (* package_remove)			(const gchar *package);
	void (* buffer_update_toggled)	(guint value);
};

struct _SymbolDBPrefs
{
	GObject parent_instance;	
	SymbolDBPrefsPriv *priv;
};

GType sdb_prefs_get_type (void) G_GNUC_CONST;

SymbolDBPrefs *
symbol_db_prefs_new (SymbolDBSystem *sdbs, SymbolDBEngine *sdbe_project,
					 SymbolDBEngine *sdbe_globals, AnjutaPreferences *prefs,
					 GList *enabled_packages);


G_END_DECLS

#endif /* _SYMBOL_DB_PREFS_H_ */
