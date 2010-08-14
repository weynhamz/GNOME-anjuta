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


#include <glib.h>
#include <config.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "symbol-db-prefs.h"

#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/anjuta-symbol-db.ui"
#define BUILDER_ROOT "symbol_prefs"
#define ICON_FILE "anjuta-symbol-db-plugin-48.png"


enum {
	COLUMN_LOAD,
	COLUMN_NAME,
	COLUMN_MAX
};

enum
{
	BUFFER_UPDATE_TOGGLED,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

struct _SymbolDBPrefsPriv {
	GtkListStore *prefs_list_store;
	GtkBuilder *prefs_bxml;
	AnjutaLauncher *pkg_config_launcher;	
	AnjutaPreferences *prefs;
	
	SymbolDBSystem *sdbs;
	SymbolDBEngine *sdbe_project;
	SymbolDBEngine *sdbe_globals;
	
	gint prefs_notify_id;
};


typedef struct _ParseableData {
	SymbolDBPrefs *sdbp;
	gchar *path_str;
	
} ParseableData;


static void
destroy_parseable_data (ParseableData *pdata)
{
	g_free (pdata->path_str);
	g_free (pdata);
}


G_DEFINE_TYPE (SymbolDBPrefs, sdb_prefs, G_TYPE_OBJECT);


static void
sdb_prefs_init1 (SymbolDBPrefs *sdbp)
{
	SymbolDBPrefsPriv *priv;

	priv = sdbp->priv;

	anjuta_preferences_add_from_builder (priv->prefs, 
									 priv->prefs_bxml, 
									 BUILDER_ROOT, 
									 _("Symbol Database"),  
									 ICON_FILE);
}

static void
sdb_prefs_init (SymbolDBPrefs *object)
{
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;
	
	sdbp = SYMBOL_DB_PREFS (object);
	sdbp->priv = g_new0 (SymbolDBPrefsPriv, 1);
	priv = sdbp->priv;

	anjuta_preferences_add_from_builder (priv->prefs, 
									 priv->prefs_bxml, 
									 BUILDER_ROOT, 
									 _("Symbol Database"),  
									 ICON_FILE);
	
	if (priv->prefs_bxml == NULL)
	{
		GError* error = NULL;
		/* Create the preferences page */
		priv->prefs_bxml = gtk_builder_new ();
		if (!gtk_builder_add_from_file (priv->prefs_bxml, BUILDER_FILE, &error))
		{
			g_warning ("Couldn't load builder file: %s", error->message);
			g_error_free(error);
		}		
	}
}

static void
sdb_prefs_finalize (GObject *object)
{
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;
	
	sdbp = SYMBOL_DB_PREFS (object);
	priv = sdbp->priv;
	
	anjuta_preferences_notify_remove(priv->prefs, priv->prefs_notify_id);
	anjuta_preferences_remove_page(priv->prefs, _("Symbol Database"));

	if (priv->prefs_bxml != NULL)
		g_object_unref (priv->prefs_bxml);

	G_OBJECT_CLASS (sdb_prefs_parent_class)->finalize (object);
}

static void
sdb_prefs_class_init (SymbolDBPrefsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	signals[BUFFER_UPDATE_TOGGLED]
		= g_signal_new ("buffer-update-toggled",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBPrefsClass, buffer_update_toggled),
						NULL, NULL,
						g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 
						1,
						G_TYPE_UINT);	
	
	object_class->finalize = sdb_prefs_finalize;
}

SymbolDBPrefs *
symbol_db_prefs_new (SymbolDBSystem *sdbs, SymbolDBEngine *sdbe_project,
					 SymbolDBEngine *sdbe_globals, AnjutaPreferences *prefs)
{
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;

	sdbp = g_object_new (SYMBOL_TYPE_DB_PREFS, NULL);
	
	priv = sdbp->priv;	
	
	priv->sdbs = sdbs;
	priv->prefs = prefs;
	priv->sdbe_project = sdbe_project;
	priv->sdbe_globals = sdbe_globals;

	return sdbp;
}

