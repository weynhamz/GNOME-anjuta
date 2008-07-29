/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta_trunk
 * Copyright (C) Massimo Cora' 2008 <maxcvs@email.it>
 * 
 * anjuta_trunk is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta_trunk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta_trunk.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _SYMBOL_DB_SYSTEM_H_
#define _SYMBOL_DB_SYSTEM_H_

#include <glib-object.h>

#include "symbol-db-engine.h"

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_SYSTEM             (sdb_system_get_type ())
#define SYMBOL_DB_SYSTEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_SYSTEM, SymbolDBSystem))
#define SYMBOL_DB_SYSTEM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_SYSTEM, SymbolDBSystemClass))
#define SYMBOL_IS_DB_SYSTEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_SYSTEM))
#define SYMBOL_IS_DB_SYSTEM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_SYSTEM))
#define SYMBOL_DB_SYSTEM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_SYSTEM, SymbolDBSystemClass))

typedef struct _SymbolDBSystemClass SymbolDBSystemClass;
typedef struct _SymbolDBSystem SymbolDBSystem;
typedef struct _SymbolDBSystemPriv SymbolDBSystemPriv;

#include "plugin.h"

struct _SymbolDBSystemClass
{
	GObjectClass parent_class;
	
	/* signals */
	void (* single_file_scan_end) 	();
	void (* scan_package_start) 	(guint num_files, const gchar *package);
	void (* scan_package_end) 		(const gchar *package);
};

struct _SymbolDBSystem
{
	GObject parent_instance;
	SymbolDBSystemPriv *priv;
};

typedef void (*PackageParseableCallback) (SymbolDBSystem *sdbs, 
										gboolean is_parseable,
										gpointer user_data);


GType sdb_system_get_type (void) G_GNUC_CONST;

SymbolDBSystem *
symbol_db_system_new (SymbolDBPlugin *sdb_plugin,
					  const SymbolDBEngine *sdbe);

/**
 * Perform a check on db to see if the package_name is present or not.
 */
gboolean 
symbol_db_system_is_package_parsed (SymbolDBSystem *sdbs, 
								   const gchar * package_name);

/**
 * Test whether the package has a good cflags output, i.e. it's parseable.
 * This function does not tell us anything about the db. It could be that 
 * the package is already on db.
 * Because this function calls AnjutaLauncher inside, the answer is asynchronous.
 * So user should put his gui in 'waiting/busy' status before going on, and of course
 * he should also prepare a callback to receive the status.
 */
void 
symbol_db_system_is_package_parseable (SymbolDBSystem *sdbs, 
								   const gchar * package_name,
								   PackageParseableCallback parseable_cb,
								   gpointer user_data);

/**
 * Scan a package. We won't do a check if the package is really parseable, but only
 * if it already exists on db. E.g. if a package has a wrong cflags string then 
 * the population won't start.
 */
gboolean 
symbol_db_system_scan_package (SymbolDBSystem *sdbs,
							  const gchar * package_name);

/**
 * Scan global db for unscanned files.
 * @warning @param files_to_scan_array Must not to be freed by caller. They'll be 
 * freed inside this function. This for speed reasons.
 * @warning @param languages_array Must not to be freed by caller. They'll be 
 * freed inside this function. This for speed reasons.
 */
void 
symbol_db_system_parse_aborted_package (SymbolDBSystem *sdbs, 
								 GPtrArray *files_to_scan_array,
								 GPtrArray *languages_array);
G_END_DECLS

#endif /* _SYMBOL_DB_SYSTEM_H_ */
