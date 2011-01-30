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

#include "cpp-package-scanner.h"
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-pkg-config.h>
#include <gio/gio.h>

enum
{
	PROP_0,
	PROP_PACKAGE,
	PROP_VERSION	
};

G_DEFINE_TYPE (CppPackageScanner, cpp_package_scanner, ANJUTA_TYPE_ASYNC_COMMAND);

static void
cpp_package_scanner_init (CppPackageScanner *object)
{
	object->files = NULL;
}

static void
cpp_package_scanner_finalize (GObject *object)
{
	CppPackageScanner* scanner = CPP_PACKAGE_SCANNER (object);
	g_free (scanner->package);
	g_free (scanner->version);
	anjuta_util_glist_strings_free (scanner->files);
	
	G_OBJECT_CLASS (cpp_package_scanner_parent_class)->finalize (object);
}

static void
cpp_package_scanner_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	CppPackageScanner* scanner;

	g_return_if_fail (CPP_IS_PACKAGE_SCANNER (object));

	scanner = CPP_PACKAGE_SCANNER(object);

	switch (prop_id)
	{
	case PROP_PACKAGE:
		scanner->package = g_value_dup_string (value);
		break;
	case PROP_VERSION:
		scanner->version = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cpp_package_scanner_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	CppPackageScanner* scanner;

	g_return_if_fail (CPP_IS_PACKAGE_SCANNER (object));

	scanner = CPP_PACKAGE_SCANNER(object);
	
	switch (prop_id)
	{
		case PROP_PACKAGE:
			g_value_set_string (value, scanner->package);
			break;
		case PROP_VERSION:
			g_value_set_string (value, scanner->version);
			break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cpp_package_scanner_list_files (GList **children, GFile *dir)
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
				cpp_package_scanner_list_files (children, file);
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
cpp_package_scanner_run (AnjutaCommand* command)
{
	CppPackageScanner* scanner = CPP_PACKAGE_SCANNER (command);
	GList* dirs = anjuta_pkg_config_get_directories (scanner->package, TRUE, NULL);
	GList* dir;

	g_message ("Scanning package: %s", scanner->package);
	
	for (dir = dirs; dir != NULL; dir = g_list_next (dir))
	{
		GFile* file = g_file_new_for_path (dir->data);
		cpp_package_scanner_list_files (&scanner->files, file);
	}
	anjuta_util_glist_strings_free (dirs);
	return 0;
}

static void
cpp_package_scanner_class_init (CppPackageScannerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = cpp_package_scanner_finalize;
	object_class->set_property = cpp_package_scanner_set_property;
	object_class->get_property = cpp_package_scanner_get_property;

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

	command_class->run = cpp_package_scanner_run;
}


AnjutaCommand*
cpp_package_scanner_new (const gchar* package, const gchar* version)
{
	GObject* object =
		g_object_new (CPP_TYPE_PACKAGE_SCANNER,
		              "package", package,
		              "version", version,
		              NULL);

	return ANJUTA_COMMAND (object);
}

const gchar* 
cpp_package_scanner_get_package (CppPackageScanner* scanner)
{
	return scanner->package;
}
const gchar* 
cpp_package_scanner_get_version (CppPackageScanner* scanner)
{
	return scanner->version;
}

GList*
cpp_package_scanner_get_files (CppPackageScanner* scanner)
{
	return scanner->files;
}
