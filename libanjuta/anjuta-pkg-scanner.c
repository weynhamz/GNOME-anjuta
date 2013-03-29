/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2011 <jhs@Obelix>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION: anjuta-pkg-scanner
 * @short_description: Scan files contained by a library using pkg-config
 * @see_also: #AnjutaAsyncCommand
 * @include libanjuta/anjuta-pkg-scanner.h
 *
 * #AnjutaPkgScanner is an async command that can be used to query the list
 * of files and the version of a package.
 * 
 */

#include <libanjuta/anjuta-pkg-scanner.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-pkg-config.h>
#include <gio/gio.h>

enum
{
	PROP_0,
	PROP_PACKAGE,
	PROP_VERSION
};

struct _AnjutaPkgScannerPrivate
{
	gchar* package;
	gchar* version;
	GList* files;
};

G_DEFINE_TYPE (AnjutaPkgScanner, anjuta_pkg_scanner, ANJUTA_TYPE_ASYNC_COMMAND);



static void
anjuta_pkg_scanner_init (AnjutaPkgScanner *object)
{
	object->priv = G_TYPE_INSTANCE_GET_PRIVATE (object,
	                                            ANJUTA_TYPE_PKG_SCANNER,
	                                            AnjutaPkgScannerPrivate);
	object->priv->files = NULL;
}

static void
anjuta_pkg_scanner_finalize (GObject *object)
{
	AnjutaPkgScanner* scanner = ANJUTA_PKG_SCANNER (object);
	g_free (scanner->priv->package);
	g_free (scanner->priv->version);
	anjuta_util_glist_strings_free (scanner->priv->files);
	
	G_OBJECT_CLASS (anjuta_pkg_scanner_parent_class)->finalize (object);
}

static void
anjuta_pkg_scanner_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	AnjutaPkgScanner* scanner;

	g_return_if_fail (ANJUTA_IS_PKG_SCANNER (object));

	scanner = ANJUTA_PKG_SCANNER(object);

	switch (prop_id)
	{
	case PROP_PACKAGE:
		scanner->priv->package = g_value_dup_string (value);
		break;
	case PROP_VERSION:
		scanner->priv->version = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_pkg_scanner_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	AnjutaPkgScanner* scanner;

	g_return_if_fail (ANJUTA_IS_PKG_SCANNER (object));

	scanner = ANJUTA_PKG_SCANNER(object);
	
	switch (prop_id)
	{
		case PROP_PACKAGE:
			g_value_set_string (value, scanner->priv->package);
			break;
		case PROP_VERSION:
			g_value_set_string (value, scanner->priv->version);
			break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_pkg_scanner_list_files (GList **children, GFile *dir)
{
	GFileEnumerator *list;
					
	list = g_file_enumerate_children (dir,
	    G_FILE_ATTRIBUTE_STANDARD_NAME,
	    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	    NULL,
	    NULL);

	if (list != NULL)
	{
		GFileInfo *info;
		
		while ((info = g_file_enumerator_next_file (list, NULL, NULL)) != NULL)
		{
			const gchar *name;
			GFile *file;

			name = g_file_info_get_name (info);
			file = g_file_get_child (dir, name);
			g_object_unref (info);

			if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
			{
				anjuta_pkg_scanner_list_files (children, file);
			}
			else
			{
				gchar* filename = g_file_get_path (file);
				*children = g_list_prepend (*children, filename);
			}
			g_object_unref (file);
		}
		g_file_enumerator_close (list, NULL, NULL);
		g_object_unref (list);
	}
}

static guint
anjuta_pkg_scanner_run (AnjutaCommand* command)
{
	AnjutaPkgScanner* scanner = ANJUTA_PKG_SCANNER (command);
	GList* dirs = anjuta_pkg_config_get_directories (scanner->priv->package, TRUE, NULL);
	GList* dir;
	
	for (dir = dirs; dir != NULL; dir = g_list_next (dir))
	{
		GFile* file = g_file_new_for_path (dir->data);
		anjuta_pkg_scanner_list_files (&scanner->priv->files, file);
	}
	anjuta_util_glist_strings_free (dirs);
	return 0;
}

static void
anjuta_pkg_scanner_class_init (AnjutaPkgScannerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = anjuta_pkg_scanner_finalize;
	object_class->set_property = anjuta_pkg_scanner_set_property;
	object_class->get_property = anjuta_pkg_scanner_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_PACKAGE,
	                                 g_param_spec_string ("package",
	                                                      "package",
	                                                      "Name of the package",
	                                                      NULL, 
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_VERSION,
	                                 g_param_spec_string ("version",
	                                                      "version",
	                                                      "Version of the package",
	                                                      NULL, 
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	command_class->run = anjuta_pkg_scanner_run;

	g_type_class_add_private (klass, sizeof (AnjutaPkgScannerPrivate));
}

/*
 * anjuta_pkg_scanner_new:
 * @package: Name of the package to scan
 * @version: Version of the package
 *
 * Returns: a new #AnjutaCommand to scan for files.
 */
AnjutaCommand*
anjuta_pkg_scanner_new (const gchar* package, const gchar* version)
{
	GObject* object =
		g_object_new (ANJUTA_TYPE_PKG_SCANNER,
		              "package", package,
		              "version", version,
		              NULL);

	return ANJUTA_COMMAND (object);
}

/*
 * anjuta_pkg_scanner_get_files:
 * @scanner: PkgScanner object
 *
 * Returns: Name of the package to scan.
 */
const gchar* 
anjuta_pkg_scanner_get_package (AnjutaPkgScanner* scanner)
{
	return scanner->priv->package;
}

/*
 * anjuta_pkg_scanner_get_files:
 * @scanner: PkgScanner object
 *
 * Returns: Version of the package to scan.
 */
const gchar* 
anjuta_pkg_scanner_get_version (AnjutaPkgScanner* scanner)
{
	return scanner->priv->version;
}

/*
 * anjuta_pkg_scanner_get_files:
 * @scanner: PkgScanner object
 *
 * Returns: (element-type GFile*): List of files to scan.
 */
GList*
anjuta_pkg_scanner_get_files (AnjutaPkgScanner* scanner)
{
	return scanner->priv->files;
}
