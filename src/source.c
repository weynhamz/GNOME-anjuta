/*
    source.c
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

/* Most of the code stolen from Glade and modified */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <gnome.h>
#include "anjuta.h"
#include "utilities.h"
#include "source.h"
#include "glades.h"

/*************************************************************************
 * Build Files.
 *************************************************************************/
static gboolean
source_write_module_file_list (ProjectDBase * data, FILE * fp, gboolean with_line_break, gchar * prefix,
			       PrjModule module)
{
	GList *files, *node;
	gboolean first;
	
	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (fp != NULL, FALSE);

	files = project_dbase_get_module_files (data, module);
	first = TRUE;
	node = files;
	while (node)
	{
		gchar *str;
		
		if (node->data)
		{
			if (prefix)
				str = g_strconcat (prefix, "/", node->data, NULL);
			else
				str = g_strdup (node->data);
			if (with_line_break)
			{
				/* We avoid printing the first '\' in the file */
				if (first)
					fprintf (fp, "\t%s", str);
				else
					fprintf (fp, "\\\n\t%s", str);
			}
			else
			{
				fprintf (fp, "%s\n", str);
			}
			first = FALSE;
			g_free (str);
		}
		node = g_list_next (node);
	}
	fprintf (fp, "\n\n");
	glist_strings_free (files);
	return TRUE;
}

/* This creates an empty file in the given directory if it doesn't already
   exist. It is used to create the empty NEWS, README, AUTHORS & ChangeLog. */
static gboolean
source_create_file_if_not_exist (const gchar * directory,
				 const gchar * filename,
				 const gchar * contents)
{
	gchar *pathname;
	FILE *fp;
	gint bytes_written, ret;

	g_return_val_if_fail (directory != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	ret = TRUE;
	pathname = g_strconcat (directory, "/", filename, NULL);

	if (!file_is_regular (pathname))
	{
		fp = fopen (pathname, "w");
		if (fp)
		{
			if (contents)
			{
				gint contents_len;
				contents_len = strlen (contents);
				bytes_written = fwrite (contents, 1, contents_len, fp);
				if (bytes_written != contents_len)
				{
					anjuta_system_error (errno, _("Error writing to file: %s."), pathname);
					ret = FALSE;
				}
			}
			fclose (fp);
		}
		else
		{
			anjuta_system_error (errno, _("Unable to create file: %s."), filename);
			ret = FALSE;
		}
	}
	g_free (pathname);
	return ret;
}

static gboolean
source_write_no_modify_warning (ProjectDBase * data, FILE * fp)
{
	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (fp != NULL, FALSE);

	fprintf (fp,
	"## Created by Anjuta - will be overwritten\n"
	"## If you don't want it to overwrite it,\n"
	"## \tPlease disable it in the Anjuta project configuration\n\n");
	return TRUE;
}

/* Copies the generic autogen.sh script which runs aclocal, automake
   & autoconf to create the Makefiles etc. */
static gboolean
source_write_autogen_sh (ProjectDBase * data)
{
	gchar *srcbuffer, *destbuffer;
	gint old_umask;
	Project_Type* type;

	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);

	srcbuffer = g_strconcat (app->dirs->data, type->autogen_file, NULL);
	destbuffer = g_strconcat (data->top_proj_dir, "/autogen.sh", NULL);

	/* FIXME: If *.desktop.in exists, just leave it, for now. */
	if (file_is_regular (destbuffer))
	{
		g_free (destbuffer);
		g_free (srcbuffer);
		return TRUE;
	}

	if (!copy_file (srcbuffer, destbuffer, FALSE))
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), destbuffer);
		g_free (destbuffer);
		g_free (srcbuffer);
		return FALSE;
	}

	/* We need to make the script executable, but we try to honour any umask. */
	old_umask = umask (0666);
	chmod (destbuffer, 0777 & ~old_umask);
	umask (old_umask);

	g_free (srcbuffer);
	g_free (destbuffer);
	return TRUE;
}

static gboolean
source_write_configure_in (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename, *actual_file, *prj_name, *version;
	Project_Type* type;
	gint lang_type, i;
	GList *list, *node;

	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);
	
	lang_type = project_dbase_get_language (data);
	g_return_val_if_fail (lang_type < 	PROJECT_PROGRAMMING_LANGUAGE_END_MARK, FALSE);
	
	filename = get_a_tmp_file();
	actual_file = g_strconcat (data->top_proj_dir, "/configure.in", NULL);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		return FALSE;
	}

	prj_name = project_dbase_get_proj_name (data);
	version = project_dbase_get_version (data);

	/* FIXME: Using AC_INIT(configure.in) is not really correct - we should be
	 * using a file unique to the project. */
	fprintf (fp,
		 "dnl Process this file with autoconf to produce a configure script.\n"
		 "dnl Created by Anjuta - will be overwritten\n"
		 "dnl If you don't want it to overwrite it,\n"
		 "dnl \tPlease disable it in the Anjuta project configuration\n"
		 "\n"
		 "AC_INIT(configure.in)\n"
		 "AM_INIT_AUTOMAKE(%s, %s)\n"
		 "AM_CONFIG_HEADER(config.h)\n"
		 "\n"
		 "AC_ISC_POSIX\n", prj_name, version);
	switch (lang_type)
	{
		case PROJECT_PROGRAMMING_LANGUAGE_C:
			fprintf (fp, "AC_PROG_CC\n");
			break;
		case PROJECT_PROGRAMMING_LANGUAGE_CPP:
			fprintf (fp, "AC_PROG_CXX\n");
			break;
		case PROJECT_PROGRAMMING_LANGUAGE_C_CPP:
			fprintf (fp, "AC_PROG_CC\n");
			fprintf (fp, "AC_PROG_CXX\n");
			break;
		default:
			fprintf (fp, "AC_PROG_CC\n");
			break;
	}
	fprintf(fp, "AM_PROG_CC_STDC\n"
		 "AC_HEADER_STDC\n");
	if (project_dbase_get_source_target != 
		PROJECT_TARGET_TYPE_EXECUTABLE)
	{
		fprintf (fp, "AM_PROG_LIBTOOL\n");
	}
 	fprintf(fp, type->configure_macros);
	fprintf (fp, "\n");
	project_config_write_scripts (data->project_config, fp);

	if (prop_get_int (data->props, "project.has.gettext", 1))
	{
		GList *files, *node;
		fprintf (fp,
			 "\n"
			 "dnl Languages which your application supports\n"
			 "ALL_LINGUAS=\"");
		files = project_dbase_get_module_files (data, MODULE_PO);
		node = glist_strings_sort (files);
		while (node)
		{
			if (node->data)
			{
				gchar *lang, *ext;
				lang = g_strdup (extract_filename (node->data));
				ext = get_file_extension (lang);
				if (ext)
					*(--ext) = '\0';
				fprintf(fp, "%s", lang);
				g_free (lang);
			}
			node = g_list_next (node);
			if (node)
				fprintf(fp, " ");
		}
		fprintf (fp,
			 "\"\nAM_GNU_GETTEXT\n");
		fprintf (fp,
			 "AM_GNU_GETTEXT_VERSION(0.10.40)\n\n");
		
		fprintf (fp,
			 "dnl Set PACKAGE_LOCALE_DIR in config.h.\n"
			 "if test \"x${prefix}\" = \"xNONE\"; then\n"
			 "  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, \"${ac_default_prefix}/${DATADIRNAME}/locale\")\n"
			 "else\n"
			 "  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, \"${prefix}/${DATADIRNAME}/locale\")\n"
			 "fi\n" "\n");
	}

	/* Define PACKAGE_SOURCE_DIR in config.h so the app can find uninstalled
	 * pixmaps. */
	fprintf (fp,
		 "\n"
		 "dnl Set PACKAGE SOURCE DIR in config.h.\n"
		 "packagesrcdir=`cd $srcdir && pwd`\n"
		 "\n");
	
	/* Define PACKAGE_DATA and PACKAGE_DOC DIR in config.h so the app can find installed pixmaps.
	 */
	fprintf (fp,
		 "dnl Set PACKAGE PREFIX\n"
		 "if test \"x${prefix}\" = \"xNONE\"; then\n"
		 "  packageprefix=${ac_default_prefix}\n"
		 "else\n"
		 "  packageprefix=${prefix}\n"
		 "fi\n\n");
	fprintf (fp,
		 "dnl Set PACKAGE DATA & DOC DIR\n"
		 "packagedatadir=share/${PACKAGE}\n"
		 "packagedocdir=doc/${PACKAGE}\n"
		 "\n");
	
	/* Define PIXMAPS, HELP, MENU DIR so that the app can find installed pixmaps.
	 */
	if (!type->gnome_support)
	{
		fprintf (fp,
			 "dnl Set PACKAGE DIRS in config.h.\n"
			 "packagepixmapsdir=${packagedatadir}/pixmaps\n"
			 "packagehelpdir=${packagedatadir}/help\n"
			 "packagemenudir=${packagedatadir}\n");
	}
	if (type->gnome_support)
	{
		fprintf (fp,
			 "dnl Set PACKAGE DIRs in config.h.\n"
			 "packagepixmapsdir=share/pixmaps/${PACKAGE}\n"
			 "packagehelpdir=share/gnome/help/${PACKAGE}\n"
			 "packagemenudir=share/gnome/apps\n");
	}
	fprintf (fp, "\n");
	/* Substitute Directories in Makefiles */
	fprintf (fp,
		 "dnl Subst PACKAGE_DATA_DIR.\n"
		 "NO_PREFIX_PACKAGE_DATA_DIR=\"${packagedatadir}\"\n"
		 "AC_SUBST(NO_PREFIX_PACKAGE_DATA_DIR)\n"
		 "PACKAGE_DATA_DIR=\"${packageprefix}/${packagedatadir}\"\n"
		 "AC_SUBST(PACKAGE_DATA_DIR)\n"
		 "\n"
		 "dnl Subst PACKAGE_DOC_DIR.\n"
		 "NO_PREFIX_PACKAGE_DOC_DIR=\"${packagedocdir}\"\n"
		 "AC_SUBST(NO_PREFIX_PACKAGE_DOC_DIR)\n"
		 "PACKAGE_DOC_DIR=\"${packageprefix}/${packagedocdir}\"\n"
		 "AC_SUBST(PACKAGE_DOC_DIR)\n"
		 "\n"
		 "dnl Subst PACKAGE_PIXMAPS_DIR.\n"
		 "NO_PREFIX_PACKAGE_PIXMAPS_DIR=\"${packagepixmapsdir}\"\n"
		 "AC_SUBST(NO_PREFIX_PACKAGE_PIXMAPS_DIR)\n"
		 "PACKAGE_PIXMAPS_DIR=\"${packageprefix}/${packagepixmapsdir}\"\n"
		 "AC_SUBST(PACKAGE_PIXMAPS_DIR)\n"
		 "\n"
		 "dnl Subst PACKAGE_HELP_DIR.\n"
		 "NO_PREFIX_PACKAGE_HELP_DIR=\"${packagehelpdir}\"\n"
		 "AC_SUBST(NO_PREFIX_PACKAGE_HELP_DIR)\n"
		 "PACKAGE_HELP_DIR=\"${packageprefix}/${packagehelpdir}\"\n"
		 "AC_SUBST(PACKAGE_HELP_DIR)\n"
		 "\n"
		 "dnl Subst PACKAGE_MENU_DIR.\n"
		 "NO_PREFIX_PACKAGE_MENU_DIR=\"${packagemenudir}\"\n"
		 "AC_SUBST(NO_PREFIX_PACKAGE_MENU_DIR)\n"
		 "PACKAGE_MENU_DIR=\"${packageprefix}/${packagemenudir}\"\n"
		 "AC_SUBST(PACKAGE_MENU_DIR)\n"
		 "\n");
	/* Define derectories in config.h file */
	fprintf (fp,
		 "AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, \"${packageprefix}/${packagedatadir}\")\n"
		 "AC_DEFINE_UNQUOTED(PACKAGE_DOC_DIR, \"${packageprefix}/${packagedocdir}\")\n"
		 "AC_DEFINE_UNQUOTED(PACKAGE_PIXMAPS_DIR, \"${packageprefix}/${packagepixmapsdir}\")\n"
		 "AC_DEFINE_UNQUOTED(PACKAGE_HELP_DIR, \"${packageprefix}/${packagehelpdir}\")\n"
		 "AC_DEFINE_UNQUOTED(PACKAGE_MENU_DIR, \"${packageprefix}/${packagemenudir}\")\n"
		 "AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR, \"${packagesrcdir}\")\n");
	fprintf (fp, "\n");

	fprintf (fp, "AC_OUTPUT([\n" "Makefile\n");
	if (prop_get_int (data->props, "project.has.gettext", 1))
	{
		fprintf (fp,
			 "intl/Makefile\n"
			 "po/Makefile.in\n");
	}
	if (type->gnome_support)
	{
		fprintf (fp, "macros/Makefile\n");
	}

	list = glist_from_string (data->project_config->extra_modules_before);
	node = list;
	while (node)
	{
		if (node->data)
		{
			fprintf (fp, "%s/Makefile\n", (gchar*)node->data);
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_string (data->project_config->config_files);
	node = list;
	while (node)
	{
		if (node->data)
		{
			fprintf (fp, "%s\n", (gchar*)node->data);
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);

	for (i = 0; i < MODULE_END_MARK; i++)
	{
		gchar *subdir;
		if (i == MODULE_PO) /* po dir is already included */
			continue;
		if (project_dbase_module_is_empty (data, i))
			continue;
		subdir = project_dbase_get_module_name (data, i);
		if (subdir)
		{
			fprintf (fp, "%s/Makefile\n", subdir);
			g_free (subdir);
		}
	}

	list = glist_from_string (data->project_config->extra_modules_after);
	node = list;
	while (node)
	{
		if (node->data)
		{
			fprintf (fp, "%s/Makefile\n", (gchar*)node->data);
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);

	if (type->gnome_support)
	{
		fprintf (fp, "%s.desktop\n", prj_name);
	}
	fprintf (fp, "])\n\n");
	fclose (fp);

	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (filename);
	g_free (actual_file);
	g_free (prj_name);
	g_free (version);
	return TRUE;
}

static gboolean
source_write_toplevel_makefile_am (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename, *actual_file, *target, *prj_name;
	gint i;
	Project_Type* type;

	g_return_val_if_fail (data != NULL, FALSE);

	filename = get_a_tmp_file();
	actual_file = g_strconcat (data->top_proj_dir, "/Makefile.am", NULL);
	type = project_dbase_get_project_type(data);
	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp, "## Process this file with automake to produce Makefile.in\n");
	source_write_no_modify_warning (data, fp);

	fprintf (fp, "SUBDIRS =");

	if (prop_get_int (data->props, "project.has.gettext", 1))
		fprintf (fp, " intl po");
	if (type->gnome_support)
		fprintf (fp, " macros");

	if (data->project_config->extra_modules_before)
		fprintf (fp, " %s", data->project_config->extra_modules_before);

	for (i = 0; i < MODULE_END_MARK; i++)
	{
		gchar *subdir;
		if (i == MODULE_PO)
			continue;
		if (project_dbase_module_is_empty (data, i))
			continue;

		subdir = project_dbase_get_module_name (data, i);

		if (subdir)
		{
			fprintf (fp, " %s", subdir);
			g_free (subdir);
		}
	}

	if (data->project_config->extra_modules_after)
		fprintf (fp, " %s", data->project_config->extra_modules_after);

	fprintf (fp, "\n\n");

	prj_name = project_dbase_get_proj_name (data);
	target = prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');

	fprintf (fp,
		"%sdocdir = ${prefix}/doc/%s\n"
		"%sdoc_DATA = \\\n"
		"\tREADME\\\n"
		"\tCOPYING\\\n"
		"\tAUTHORS\\\n"
		"\tChangeLog\\\n"
		"\tINSTALL\\\n"
		"\tNEWS\\\n"
		"\tTODO", target, prj_name, target);
	if (prop_get_int (data->props, "project.has.gettext", 1))
	{
		fprintf (fp,
			"\\\n\tABOUT-NLS");
	}
	fprintf (fp,
		"\n\nEXTRA_DIST = $(%sdoc_DATA)\n\n", target);
	if(type->gnome_support)
	{
		gchar *group;
		
		group = prop_get (data->props, "project.menu.group");
		if (!group) group = g_strdup ("Applications");
		fprintf (fp, "gnomemenudir = $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@/%s\n", group);
		fprintf (fp, "gnomemenu_DATA = %s.desktop\n\n", prj_name);
		g_free (group);
	}
	if (data->project_config->makefile_am)
	{
		fprintf (fp, "\n");
		fprintf (fp, data->project_config->makefile_am);
		fprintf (fp, "\n");
	}
	fprintf (fp, "# Copy all the spec files. Of cource, only one is actually used.\n");
	fprintf (fp, "dist-hook:\n"
		  "\tfor specfile in *.spec; do \\\n"
		  "\t\tif test -f $$specfile; then \\\n"
		  "\t\t\tcp -p $$specfile $(distdir); \\\n"
		  "\t\tfi \\\n"
		  "\tdone\n"
		  "\n");
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	g_free (prj_name);
	g_free (filename);
	free_project_type(type);
	return TRUE;
}

static gboolean
source_write_macros_files (ProjectDBase * data)
{
	static const gchar *gnome1_files[] = {
		"ChangeLog",
		"Makefile.am",
		"autogen.sh",
		"aclocal-include.m4",
		"compiler-flags.m4",
		"curses.m4",
		"gnome-bonobo-check.m4",
		"gnome-common.m4",
		"gnome-fileutils.m4",
		"gnome-gettext.m4",
		"gnome-ghttp-check.m4",
		"gnome-gnorba-check.m4",
		"gnome-guile-checks.m4",
		"gnome-libgtop-check.m4",
		"gnome-objc-checks.m4",
		"gnome-orbit-check.m4",
		"gnome-print-check.m4",
		"gnome-pthread-check.m4",
		"gnome-support.m4",
		"gnome-undelfs.m4",
		"gnome-vfs.m4",
		"gnome-x-checks.m4",
		"gnome-xml-check.m4",
		"gnome.m4",
		"linger.m4",
		"need-declaration.m4",
		NULL
	};
	static const gchar *gnome2_files[] = {
		"ChangeLog",
		"Makefile.am",
		"autogen.sh",
		"check-utmp.m4",
		"gnome-cxx-check.m4",
		"gnome-pthread-check.m4",
		"compiler-flags.m4",
		"gnome-gettext.m4",
		"gnome-x-checks.m4",
		"curses.m4",
		"gnome-pkgconfig.m4",
		"linger.m4",
		"gnome-common.m4",
		"gnome-platform.m4",
		NULL
	};
	gchar const **files;
	/* This is the maximum length of the above filenames, so we can use the
	 * same buffers for each file. */
	static const gint MAX_MACROS_FILENAME_LEN = 64;
	gchar *srcbuffer, *destbuffer;
	Project_Type* type;
	gint i;

	g_return_val_if_fail (data != NULL, FALSE);

	srcbuffer =
		g_malloc (strlen (app->dirs->data) + MAX_MACROS_FILENAME_LEN);
	destbuffer =
		g_malloc (strlen (data->top_proj_dir) +
			  MAX_MACROS_FILENAME_LEN);

	/* Create the source directory if it doesn't exist. */
	sprintf (destbuffer, "%s/macros", data->top_proj_dir);
	force_create_dir (destbuffer);

	type = project_dbase_get_project_type(data);
	
	if (type->gnome2_support) {
		files = gnome2_files;
	} else {
		files = gnome1_files;
	}
	for (i = 0; files[i]; i++)
	{
		if (type->gnome2_support)
		{
			sprintf (srcbuffer, "%s/gnome2/%s", app->dirs->data, files[i]);
		} else {
			sprintf (srcbuffer, "%s/gnome/%s", app->dirs->data, files[i]);
		}
		sprintf (destbuffer, "%s/macros/%s", data->top_proj_dir, files[i]);
		if (file_is_regular (destbuffer) == FALSE)
		{
			if (!copy_file (srcbuffer, destbuffer, FALSE))
			{
				anjuta_system_error (errno, _("Unable to create file: %s."), destbuffer);
				g_free (srcbuffer);
				g_free (destbuffer);
				return FALSE;
			}
		}
	}

	g_free (srcbuffer);
	g_free (destbuffer);
	return TRUE;
}

static gboolean
source_write_po_files (ProjectDBase * data)
{
	gchar *dirname = NULL, *filename = NULL, *actual_file, *prefix, *str;
	FILE *fp;

	g_return_val_if_fail (data != NULL, FALSE);

	/* Returns if gettext support isn't wanted. */
	if (prop_get_int (data->props, "project.has.gettext", 1) == FALSE)
		return TRUE;

	str = project_dbase_get_module_name (data, MODULE_PO);
	dirname = g_strconcat (data->top_proj_dir, "/", str, NULL);
	g_free (str);
	force_create_dir (dirname);

	/* Create ChangeLog if it doesn't exist. */
	source_create_file_if_not_exist (dirname, "/ChangeLog", NULL);

	filename = get_a_tmp_file();
	actual_file  = g_strconcat (dirname, "/POTFILES.in", NULL);
	g_free (dirname);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		return FALSE;
	}

	/* We need the relative path from the project directory to the source
	 * directory, to prefix each source file. */
	prefix = project_dbase_get_module_name (data, MODULE_SOURCE);
	fprintf (fp, "# List of source files containing translatable strings.\n\n");
	source_write_no_modify_warning (data, fp);
	fprintf (fp,"# Source files\n");
	source_write_module_file_list (data, fp, FALSE, prefix, MODULE_SOURCE);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (actual_file);
	g_free (prefix);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_executable_source_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;
	Project_Type* type;
	gint lang;
	
	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);

	if (project_dbase_module_is_empty (data, MODULE_SOURCE))
		return TRUE;
	
	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_SOURCE);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		free_project_type(type);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);
	/* If the project directory is the source directory, we need to output
	 * SUBDIRS here. */
	/* Fixed bug #460321 "Problem with optimization options" by separating 
	   INCLUDE AND C(XX)FLAGS. Johannes Schmid 3.2.2002 */ 
	
	fprintf (fp, "INCLUDES =");
	fprintf (fp, type->cflags);
	compiler_options_set_prjincl_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	
	lang = project_dbase_get_language(data);
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C ||
		lang == PROJECT_PROGRAMMING_LANGUAGE_C_CPP)
	{
		fprintf (fp, "CFLAGS =");
		compiler_options_set_prjcflags_in_file (app->compiler_options, fp);
		fprintf (fp, "\n\n");
	}
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_CPP ||
		lang == PROJECT_PROGRAMMING_LANGUAGE_C_CPP)
	{
		fprintf (fp, "CXXFLAGS =");
		compiler_options_set_prjcflags_in_file (app->compiler_options, fp);
		fprintf (fp, "\n\n");
	}
	
	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "bin_PROGRAMS = %s\n\n", target);
	fprintf (fp, "%s_SOURCES = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_SOURCE);

	fprintf (fp, "%s_LDFLAGS = ", target);
	/* If any equivalent for this could be done in type->... */
	/* fprintf (fp, type->ldadd); */
	compiler_options_set_prjlflags_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	
	fprintf (fp, "%s_LDADD = ", target);
	fprintf (fp, type->ldadd);
	compiler_options_set_prjlibs_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	fclose (fp);
	
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	
	g_free (actual_file);
	g_free (target);
	free_project_type(type);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_static_lib_source_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;
	Project_Type* type;
	gint lang;
	
	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);

	if (project_dbase_module_is_empty (data, MODULE_SOURCE))
		return TRUE;
	
	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_SOURCE);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		free_project_type(type);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);
	/* If the project directory is the source directory, we need to output
	 * SUBDIRS here. */
	/* Fixed bug #460321 "Problem with optimization options" by separating 
	   INCLUDE AND C(XX)FLAGS. Johannes Schmid 3.2.2002 */ 
	
	fprintf (fp, "INCLUDES =");
	fprintf (fp, type->cflags);
	compiler_options_set_prjincl_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	
	lang = project_dbase_get_language(data);
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C ||
		lang == PROJECT_PROGRAMMING_LANGUAGE_C_CPP)
	{
		fprintf (fp, "CFLAGS =");
		compiler_options_set_prjcflags_in_file (app->compiler_options, fp);
		fprintf (fp, "\n\n");
	}
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_CPP ||
		lang == PROJECT_PROGRAMMING_LANGUAGE_C_CPP)
	{
		fprintf (fp, "CXXFLAGS =");
		compiler_options_set_prjcflags_in_file (app->compiler_options, fp);
		fprintf (fp, "\n\n");
	}
	
	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "lib_LIBRARIES = %s%s\n\n", target, ".a");
	fprintf (fp, "%s_%s_SOURCES = \\\n", target, "a");
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_SOURCE);

	/* automake says that this is invalid */
	/* fprintf (fp, "%s_%s_LDFLAGS = ", target, "a");*/
	compiler_options_set_prjlflags_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	
	fprintf (fp, "%s_%s_LIBADD = ", target, "a");
	fprintf (fp, type->ldadd);
	compiler_options_set_prjlibs_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	fclose (fp);
	
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	
	g_free (actual_file);
	g_free (target);
	free_project_type(type);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_dynamic_lib_source_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;
	Project_Type* type;
	gint lang;
	
	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);

	if (project_dbase_module_is_empty (data, MODULE_SOURCE))
		return TRUE;
	
	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_SOURCE);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		free_project_type(type);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);
	/* If the project directory is the source directory, we need to output
	 * SUBDIRS here. */
	/* Fixed bug #460321 "Problem with optimization options" by separating 
	   INCLUDE AND C(XX)FLAGS. Johannes Schmid 3.2.2002 */ 
	
	fprintf (fp, "INCLUDES =");
	fprintf (fp, type->cflags);
	compiler_options_set_prjincl_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	
	lang = project_dbase_get_language(data);
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C ||
		lang == PROJECT_PROGRAMMING_LANGUAGE_C_CPP)
	{
		fprintf (fp, "CFLAGS =");
		compiler_options_set_prjcflags_in_file (app->compiler_options, fp);
		fprintf (fp, "\n\n");
	}
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_CPP ||
		lang == PROJECT_PROGRAMMING_LANGUAGE_C_CPP)
	{
		fprintf (fp, "CXXFLAGS =");
		compiler_options_set_prjcflags_in_file (app->compiler_options, fp);
		fprintf (fp, "\n\n");
	}
	
	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "lib_LTLIBRARIES = %s%s\n\n", target, ".la");
	fprintf (fp, "%s_%s_SOURCES = \\\n", target, "la");
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_SOURCE);

	fprintf (fp, "%s_%s_LDFLAGS = ", target, "la");
	/* If any equivalent for this could be done in type->... */
	/* fprintf (fp, type->ldadd); */
	compiler_options_set_prjlflags_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	
	fprintf (fp, "%s_%s_LIBADD = ", target, "la");
	fprintf (fp, type->ldadd);
	compiler_options_set_prjlibs_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	fclose (fp);
	
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	
	g_free (actual_file);
	g_free (target);
	free_project_type(type);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_source_files (ProjectDBase * data)
{
	gint target_type, ret;
	g_return_val_if_fail (data != NULL, FALSE);

	target_type = project_dbase_get_target_type (data);
	switch (target_type)
	{
		case PROJECT_TARGET_TYPE_EXECUTABLE:
			ret = source_write_executable_source_files (data);
			break;
		case PROJECT_TARGET_TYPE_STATIC_LIB:
			ret = source_write_static_lib_source_files (data);
			break;
		case PROJECT_TARGET_TYPE_DYNAMIC_LIB:
			ret = source_write_dynamic_lib_source_files (data);
			break;
		default:
			anjuta_error ("Unkown Project's target type");
			return FALSE;
	}
	return ret;
}

static gboolean
source_write_include_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;
	g_return_val_if_fail (data != NULL, FALSE);

	if (project_dbase_module_is_empty (data, MODULE_INCLUDE))
		return TRUE;

	src_dir = project_dbase_get_module_dir (data, MODULE_INCLUDE);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_INCLUDE);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);

	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "%s_includedir = $(pkgincludedir)\n\n", target);
	fprintf (fp, "%s_include_DATA = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_INCLUDE);

	fprintf (fp, "EXTRA_DIST = $(%s_include_DATA)\n", target);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_pixmap_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;
	Project_Type* type;

	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);

	if (project_dbase_module_is_empty (data, MODULE_PIXMAP))
		return TRUE;

	src_dir = project_dbase_get_module_dir (data, MODULE_PIXMAP);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_PIXMAP);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		free_project_type(type);
		return FALSE;
	}
	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);

	target = prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "%s_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@\n\n", target);
	
	fprintf (fp, "%s_pixmaps_DATA = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_PIXMAP);

	fprintf (fp, "EXTRA_DIST = $(%s_pixmaps_DATA)\n", target);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	g_free (filename);
	free_project_type(type);
	return TRUE;
}

static gboolean
source_write_help_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;
	Project_Type* type;

	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);
	if (project_dbase_module_is_empty (data, MODULE_HELP))
		return TRUE;

	src_dir = project_dbase_get_module_dir (data, MODULE_HELP);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_HELP);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		free_project_type(type);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);

	target = prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "%s_helpdir = $(prefix)/@NO_PREFIX_PACKAGE_HELP_DIR@/C\n\n", target);
	fprintf (fp, "%s_help_DATA = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_HELP);

	fprintf (fp, "EXTRA_DIST = $(%s_help_DATA)\n", target);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	free_project_type(type);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_doc_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;

	g_return_val_if_fail (data != NULL, FALSE);

	if (project_dbase_module_is_empty (data, MODULE_DOC))
		return TRUE;

	src_dir = project_dbase_get_module_dir (data, MODULE_DOC);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_DOC);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		return FALSE;
	}
	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);

	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "%s_docdir = $(prefix)/@NO_PREFIX_PACKAGE_DOC_DIR@\n\n", target);
	fprintf (fp, "%s_doc_DATA = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_DOC);

	fprintf (fp, "EXTRA_DIST = $(%s_doc_DATA)\n", target);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_data_files (ProjectDBase * data)
{
	FILE *fp;
	gchar *src_dir, *filename, *actual_file, *target;

	g_return_val_if_fail (data != NULL, FALSE);

	if (project_dbase_module_is_empty (data, MODULE_DATA))
		return TRUE;

	src_dir = project_dbase_get_module_dir (data, MODULE_DATA);
	force_create_dir (src_dir);
	g_free (src_dir);

	src_dir = project_dbase_get_module_name (data, MODULE_DATA);
	filename = get_a_tmp_file();
	actual_file =	g_strconcat (data->top_proj_dir, "/", src_dir, "/Makefile.am", NULL);
	g_free (src_dir);

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		return FALSE;
	}
	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);

	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "%s_datadir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@\n\n", target);
	fprintf (fp, "%s_data_DATA = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_DATA);

	fprintf (fp, "EXTRA_DIST = $(%s_data_DATA)\n", target);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Unable to create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	g_free (filename);
	return TRUE;
}

/* FIXME: Newer autoconf deprecates the use of this file, but just
gives a little warning */
static gboolean
source_write_acconfig_h (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename;

	g_return_val_if_fail (data != NULL, FALSE);

	filename = g_strconcat (data->top_proj_dir, "/acconfig.h", NULL);

	/* FIXME: If file exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}

	/* If we aren't using gettext or Gnome some of these may not be necessary,
	 * but I don't think they cause problems. */
	fprintf (fp,
		 "#undef ENABLE_NLS\n"
		 "#undef HAVE_CATGETS\n"
		 "#undef HAVE_GETTEXT\n"
		 "#undef HAVE_LC_MESSAGES\n"
		 "#undef HAVE_STPCPY\n"
		 "#undef HAVE_LIBSM\n"
		 "#undef PACKAGE_LOCALE_DIR\n"
		 "#undef PACKAGE_DOC_DIR\n"
		 "#undef PACKAGE_DATA_DIR\n"
		 "#undef PACKAGE_PIXMAPS_DIR\n"
		 "#undef PACKAGE_HELP_DIR\n"
		 "#undef PACKAGE_MENU_DIR\n"
		 "#undef PACKAGE_SOURCE_DIR\n");
	fclose (fp);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_acinclude_m4 (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename;

	g_return_val_if_fail (data != NULL, FALSE);

	filename = g_strconcat (data->top_proj_dir, "/acinclude.m4", NULL);

	/* FIXME: If file exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}

	fprintf (fp, "AC_DEFUN([AM_GNU_GETTEXT_VERSION], [])\n");
	fclose (fp);
	g_free (filename);
	return TRUE;
}

/* Copies the generic autogen.sh script which runs aclocal, automake
   & autoconf to create the Makefiles etc. */
static gboolean
source_write_setup_gettext (ProjectDBase * data)
{
	gchar *srcbuffer, *destbuffer;
	gint old_umask;
	Project_Type* type;

	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type(data);

	srcbuffer = g_strconcat (app->dirs->data, "/setup-gettext", NULL);
	destbuffer = g_strconcat (data->top_proj_dir, "/setup-gettext", NULL);

	/* FIXME: If *.desktop.in exists, just leave it, for now. */
	if (file_is_regular (destbuffer))
	{
		g_free (destbuffer);
		g_free (srcbuffer);
		return TRUE;
	}

	if (!copy_file (srcbuffer, destbuffer, FALSE))
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), destbuffer);
		g_free (destbuffer);
		g_free (srcbuffer);
		return FALSE;
	}

	/* We need to make the script executable, but we try to honour any umask. */
	old_umask = umask (0666);
	chmod (destbuffer, 0777 & ~old_umask);
	umask (old_umask);

	g_free (srcbuffer);
	g_free (destbuffer);
	return TRUE;
}

static gboolean
source_write_desktop_entry (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename, *target, *name, *comment, *term, *icon, *prj_name;
	gint t;
	gboolean has_icon;

	g_return_val_if_fail (data != NULL, FALSE);

	prj_name = project_dbase_get_proj_name (data);
	target = prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	
	filename = g_strconcat (data->top_proj_dir, "/", prj_name, ".desktop.in", NULL);

	/* FIXME: If *.desktop.in exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		g_free (target);
		g_free (prj_name);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		g_free (target);
		g_free (prj_name);
		return FALSE;
	}
	name = prop_get (data->props, "project.menu.entry");
	if (!name)
		name = g_strdup ("Dummy");
	comment = prop_get (data->props, "project.menu.comment");
	if (!comment)
		comment = g_strdup ("No comment");
	icon = prop_get (data->props, "project.menu.icon");
	has_icon = (gboolean) icon;
	if (!icon)
		icon = g_strdup (" ");
	t = prop_get_int (data->props, "project.menu.need.terminal", 1);
	if (t)
		term = g_strdup ("true");
	else
		term = g_strdup ("false");

	fprintf (fp,
		"\n[Desktop Entry]\n"
		"Name=%s\n"
		"Comment=%s\n"
		"Exec=%s\n"
		"Icon=%s%s\n"
		"Terminal=%s\n"
		"MultipleArgs=false\n"
		"Type=Application\n\n",
		name, comment, target,
		icon ? "@PACKAGE_PIXMAPS_DIR@/" : "",
		icon ? icon : "",
		term);

	fclose (fp);

	g_free (filename);
	g_free (name);
	g_free (comment);
	g_free (icon);
	g_free (target);
	g_free (prj_name);
	g_free (term);

	return TRUE;
}


static gboolean
source_write_glade_file (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename, *target, *prj_name;
	gchar *prj, *src, *pix;
	Project_Type* type;
	gint lang;
	gboolean bOK = TRUE ;

	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type (data);
	prj_name = project_dbase_get_proj_name (data);
	target = prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	
	filename = g_strconcat (data->top_proj_dir, "/", prj_name, ".glade", NULL);
	g_free (prj_name);

	/* FIXME: If *.glade exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		free_project_type(type);
		g_free (filename);
		g_free (target);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		free_project_type(type);
		g_free (filename);
		g_free (target);
		return FALSE;
	}
	fprintf(fp,
		"<?xml version=\"1.0\"?>\n"
		"<GTK-Interface>\n\n"
		);
	
	prj = project_dbase_get_proj_name (data);
	fprintf(fp,
		"<project>\n"
		"  <name>%s</name>\n"
		"  <program_name>%s</program_name>\n",
		prj, target);
	g_free (prj);
	g_free (target);
	
	src = project_dbase_get_module_name (data, MODULE_SOURCE);
	pix = project_dbase_get_module_name (data, MODULE_PIXMAP);
	fprintf(fp,
		"  <directory></directory>\n"
		"  <source_directory>%s</source_directory>\n"
		"  <pixmaps_directory>%s</pixmaps_directory>\n",
		src, pix);
	g_free (src);
	g_free (pix);
	
	lang = project_dbase_get_language (data);
	if (type->id == PROJECT_TYPE_GTK || 
		type->id == PROJECT_TYPE_GNOME ||
		type->id == PROJECT_TYPE_GTK2 ||
		type->id == PROJECT_TYPE_LIBGLADE2 ||
		type->id == PROJECT_TYPE_GNOME2)
	{
		fprintf(fp, "  <language>C</language>\n");
	}
	else if (type->id == PROJECT_TYPE_GTKMM ||
			type->id == PROJECT_TYPE_GNOMEMM ||
			type->id == PROJECT_TYPE_GTKMM2 ||
			type->id == PROJECT_TYPE_GNOMEMM2 )
	{	
		fprintf(fp, "  <language>CPP</language>\n");
	}
	else
		fprintf(fp, "  <language>C</language>\n");
	if (!type->gnome_support)
		fprintf(fp, "  <gnome_support>False</gnome_support>\n");
	else
		fprintf(fp, "  <gnome_support>True</gnome_support>\n");
	
	if (prop_get_int (data->props, "project.has.gettext", 1))
		fprintf(fp, "  <gettext_support>True</gettext_support>\n");
	else
		fprintf(fp, "  <gettext_support>False</gettext_support>\n");
	
	fprintf(fp, 
		"  <use_widget_names>False</use_widget_names>\n"
		"  <output_main_file>True</output_main_file>\n"
		"  <output_support_files>True</output_support_files>\n"
		"  <output_build_files>False</output_build_files>\n"
		"  <backup_source_files>True</backup_source_files>\n"
		"  <main_source_file>interface.c</main_source_file>\n"
		"  <main_header_file>interface.h</main_header_file>\n"
		"  <handler_source_file>callbacks.c</handler_source_file>\n"
		"  <handler_header_file>callbacks.h</handler_header_file>\n"
		"  <support_source_file>support.c</support_source_file>\n"
		"  <support_header_file>support.h</support_header_file>\n"
		"  <output_translatable_strings>True</output_translatable_strings>\n"
		"  <translatable_strings_file>strings.tbl</translatable_strings_file>\n"
		"</project>\n\n"
		);
	fprintf(fp,
		"<widget>\n"
		"  <class>GtkWindow</class>\n"
		"  <name>window1</name>\n"
		"  <title>window1</title>\n"
		"  <type>GTK_WINDOW_TOPLEVEL</type>\n"
		"  <position>GTK_WIN_POS_NONE</position>\n"
		"  <modal>False</modal>\n"
		"  <default_width>510</default_width>\n"
		"  <default_height>300</default_height>\n"
		"  <allow_shrink>False</allow_shrink>\n"
		"  <allow_grow>True</allow_grow>\n"
		"  <auto_shrink>False</auto_shrink>\n\n"
		"  <widget>\n"
		"    <class>Placeholder</class>\n"
		"  </widget>\n"
		"</widget>\n\n"
		"</GTK-Interface>\n"
		);
	free_project_type(type);
	fflush( fp );
	if( ferror( fp ) )
	{		anjuta_system_error (errno, _("Error writing to: %s."), filename);
		bOK = FALSE ;
	}
	fclose (fp);
	if( bOK )
	{
		gladen_load_project(filename);
		gladen_add_main_components();
	}
	g_free( filename );
	return bOK ;
}

gboolean
source_write_generic_main_c (ProjectDBase *data)
{
	FILE *fp;
	gchar *filename, *src_dir;
	gint lang;

	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data->project_is_open, FALSE);

	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	if (!src_dir)
		return FALSE;
	force_create_dir (src_dir);

	lang = project_dbase_get_language (data);
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C)
	{
		filename = g_strconcat (src_dir, "/main.c", NULL);
	}
	else
	{
		filename = g_strconcat (src_dir, "/main.cc", NULL);
	}
	g_free (src_dir);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C)
	{
		fprintf(fp,
			"#include <stdio.h>\n"
			"int main()\n\n"
			"{\n"
			"\tprintf(\"Hello world\\n\");\n"
			"\treturn (0);\n"
			"}\n\n");
	}
	else
	{
		fprintf(fp,
			"#include <iostream.h>\n"
			"int main()\n\n"
			"{\n"
			"\tcout << \"Hello world\\n\";\n"
			"\treturn (0);\n"
			"}\n\n");
	}
	fclose (fp);
	g_free (filename);
	return TRUE;
}

gboolean
source_write_libglade_main_c (ProjectDBase *data)
{
	FILE *fp;
	gchar *filename, *src_dir, *gladefile, *prj_name;
	gint lang;

	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data->project_is_open, FALSE);

	prj_name = project_dbase_get_proj_name (data);
	gladefile = g_strconcat (data->top_proj_dir, "/", prj_name, ".glade", NULL);
	g_free (prj_name);
	
	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	if (!src_dir)
		return FALSE;
	force_create_dir (src_dir);

	lang = project_dbase_get_language (data);
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C)
	{
		filename = g_strconcat (src_dir, "/main.c", NULL);
	}
	else
	{
		filename = g_strconcat (src_dir, "/main.cc", NULL);
	}
	g_free (src_dir);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C)
	{

		fprintf(fp,
			"#ifdef HAVE_CONFIG_H\n"
			"#  include <config.h>\n"
			"#endif\n\n"
			"#include <gnome.h>\n"
			"#include <glade/glade.h>\n\n"
			"int main (int argc, char *argv[])\n"
			"{\n"
			"	GtkWidget *window1;\n"
			"	GladeXML *xml;\n\n"
			"	#ifdef ENABLE_NLS\n"
			"		bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);\n"
			"		textdomain (PACKAGE);\n"
			"	#endif\n\n"
			"	gnome_init (PACKAGE, VERSION, argc, argv);\n"
			"	glade_gnome_init ();\n"
			"	/*\n"
			"	 * The .glade filename should be on the next line.\n"
			"	 */\n"
			"	xml = glade_xml_new(\"%s\", NULL);\n\n"
			"	/* This is important */\n"
			"	glade_xml_signal_autoconnect(xml);\n"
			"	window1 = glade_xml_get_widget(xml, \"window1\");\n"
			"	gtk_widget_show (window1);\n\n"
			"	gtk_main ();\n"
			"	return 0;\n"
			"}\n", gladefile);
	}
	else
	{
		fprintf(fp,
			"#include <iostream.h>\n"
			"int main()\n\n"
			"{\n"
			"\tcout << \"Hello world\\n\";\n"
			"\treturn (0);\n"
			"}\n\n");
	}
	fclose (fp);
	g_free (filename);
	return TRUE;
}


gboolean
source_write_wxwin_main_c (ProjectDBase *data)
{
	FILE *fp;
	gchar *filename, *src_dir;

	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data->project_is_open, FALSE);

	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	if (!src_dir)
		return FALSE;
	force_create_dir (src_dir);

	filename = g_strconcat (src_dir, "/main.cc", NULL);
	g_free (src_dir);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	fprintf(fp,
		"#ifdef HAVE_CONFIG_H\n"
		"#  include <config.h>\n"
		"#endif\n"
		"#include <wx/wx.h>\n\n"
	      	"class MyApp : public wxApp\n"
	      	"{\n"
	      	"  public:\n"
	      	"    virtual bool OnInit();\n"
	      	"};\n\n"
	      	"IMPLEMENT_APP(MyApp)\n\n"
	      	"bool MyApp::OnInit()\n"
	      	"{\n"
	      	"  wxFrame *frame = new wxFrame((wxFrame *)NULL, -1, \"Hello World\",\n"
	      	"                               wxPoint(50, 50), wxSize(450, 340));\n\n"
	      	"  frame->Show(TRUE);\n"
	      	"  return TRUE;\n"
	      	"}\n");
	fclose (fp);
	g_free (filename);
	return TRUE;
}

gboolean
source_write_xwin_main_c (ProjectDBase *data)
{
	FILE *fp;
	gchar *filename, *src_dir;
	fprintf(stderr, "xwin main\n");
	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data->project_is_open, FALSE);

	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	if (!src_dir)
		return FALSE;
	force_create_dir (src_dir);

	filename = g_strconcat (src_dir, "/main.c", NULL);
	g_free (src_dir);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	fprintf(fp,
		"/*Program closes with a mouse click or keypress */\n\n");    
	fprintf(fp,
		"#ifdef HAVE_CONFIG_H\n"
		"#  include <config.h>\n"
		"#endif\n"
		"#include <stdio.h>\n"
		"#include <X11/Xlib.h>\n\n"
    
		"int main (int argc, char *argv[])\n"
		"{\n"
		"\tDisplay              *dpy;\n"
		"\tVisual               *visual;\n"
		"\tint                   depth;\n"
		"\tXSetWindowAttributes  attributes;\n"    
		"\tWindow                win;\n"
		"\tXFontStruct         *fontinfo;\n"
		"\tXColor               color, dummy;\n"
		"\tXGCValues            gr_values;\n"
		"\tGC                   gc;\n"
		"\tXKeyEvent event;\n\n"

		"\tdpy = XOpenDisplay(NULL);\n"
		"\tvisual = DefaultVisual(dpy, 0);\n"
		"\tdepth  = DefaultDepth(dpy, 0);\n"
		"\tattributes.background_pixel = XWhitePixel(dpy, 0);\n"

		"\t/* create the application window */\n"
		"\twin = XCreateWindow(dpy, XRootWindow(dpy, 0),\n"
		"\t\t\t50, 50, 400, 400, 5, depth,\n"  
		"\t\t\tInputOutput, visual, CWBackPixel,\n" 
		"\t\t\t&attributes);\n"
		"\tXSelectInput(dpy, win, ExposureMask | KeyPressMask |\n" 
		"\t\t\tButtonPressMask | StructureNotifyMask);\n\n"
		"\tfontinfo = XLoadQueryFont(dpy, \"6x10\");\n"

		"\tXAllocNamedColor(dpy, DefaultColormap(dpy, 0),\n"
		"\t\t\"green\", &color, &dummy);\n"

		"\tgr_values.font = fontinfo->fid;\n"
		"\tgr_values.foreground = color.pixel;\n"
		"\tgc = XCreateGC(dpy, win, GCFont+GCForeground, &gr_values);\n"
		"\tXMapWindow(dpy, win);\n\n"
        
		"\t/* run till key press */\n"
		"\twhile(1){\n"

		"\t\tXNextEvent(dpy, &event);\n"
		"\t\tswitch(event.type) {\n"
		"\t\t\tcase Expose:\n"
		"\t\t\t\tXDrawLine(dpy, win, gc, 0, 0, 100, 100);\n"
		"\t\t\t\tXDrawRectangle(dpy, win, gc, 140, 140, 50, 50);\n"
		"\t\t\t\tXDrawString(dpy, win, gc, 100, 100, \"hello X world\", 13);\n"
		"\t\t\t\tbreak;\n"
        
		"\t\t\tcase ButtonPress:\n"
		"\t\t\tcase KeyPress:\n"        
		"\t\t\t\tXUnloadFont(dpy, fontinfo->fid);\n"
		"\t\t\t\tXFreeGC(dpy, gc);\n"
		"\t\t\t\tXCloseDisplay(dpy);\n"
		"\t\t\t\texit(0);\n"
		"\t\t\t\tbreak;\n"
        
		"\t\t\tcase ConfigureNotify:\n"
		"\t\t\t\t/* reconfigure size of window here */\n"          
		"\t\t\t\tbreak;\n"    
        
		"\t\t\tdefault:\n" 
		"\t\t\t\tbreak;\n"
		"\t\t}\n"
        
		"\t}\n"
		"\treturn(0);\n"
	"}\n");
	fclose (fp);
	g_free (filename);
	return TRUE;
}

gboolean
source_write_xwindockapp_main_c (ProjectDBase *data)
{
	FILE *fp;
	gchar *filename, *src_dir;

	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data->project_is_open, FALSE);

	src_dir = project_dbase_get_module_dir (data, MODULE_SOURCE);
	if (!src_dir)
		return FALSE;
	force_create_dir (src_dir);

	/* wmgeneral.h */
	filename = g_strconcat (src_dir, "/wmgeneral.h", NULL);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	fprintf(fp,
		"#ifndef WMGENERAL_H_INCLUDED\n"
		"#define WMGENERAL_H_INCLUDED\n"

		"/* Defines */\n"
		"#define MAX_MOUSE_REGION (8)\n\n"

		"/* Typedefs */\n"
		"typedef struct _rckeys rckeys;\n\n"

		"struct _rckeys {\n"
		"\tconst char	*label;\n"
		"\tchar		**var;\n"
		"};\n\n"

		"typedef struct {\n"
		"\tPixmap		pixmap;\n"
		"\tPixmap		mask;\n"
		"\tXpmAttributes	attributes;\n"
		"} XpmIcon;\n\n"

		"/* Global variable */\n"
		"Display	*display;\n"
		"Window          Root;\n"
		"GC              NormalGC;\n"
		"XpmIcon         wmgen;\n\n"

		"/* Function Prototypes */\n"
		"void AddMouseRegion(int index, int left, int top, int right, int bottom);\n"
		"int CheckMouseRegion(int x, int y);\n"
		"void openXwindow(int argc, char *argv[], char **, char *, int, int);\n"
		"void RedrawWindow(void);\n"
		"void RedrawWindowXY(int x, int y);\n"
		"void copyXPMArea(int, int, int, int, int, int);\n"
		"void copyXBMArea(int, int, int, int, int, int);\n"
		"void setMaskXY(int, int);\n"
		"void parse_rcfile(const char *, rckeys *);\n\n"

		"#endif\n");
	fclose (fp);
	g_free (filename);
  
	/* wmgeneral.c */
	filename = g_strconcat (src_dir, "/wmgeneral.c", NULL);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	fprintf(fp,
		"/*\twmgeneral was taken from wmppp.\n\n"
		"\tIt has a lot of routines which most of the wm* programs use.\n\n"

		"\t------------------------------------------------------------\n"
		"\tAuthor: Martijn Pieterse (pieterse@xs4all.nl)\n\n"
	
		"\t--- CHANGES: ---\n"
		"\t02/05/1998 (Martijn Pieterse, pieterse@xs4all.nl)\n"
		"\t\t* changed the read_rc_file to parse_rcfile, as suggester by Marcelo E. Magallon\n"
		"\t\t* debugged the parse_rc file.\n"
		"\t30/04/1998 (Martijn Pieterse, pieterse@xs4all.nl)\n"
		"\t\t* Ripped similar code from all the wm* programs,\n"
		"\t\t  and put them in a single file.\n"
		"*/\n\n"

		"#include <stdlib.h>\n"
		"#include <stdio.h>\n"
		"#include <string.h>\n"
		"#include <unistd.h>\n"
		"#include <ctype.h>\n"
		"#include <stdarg.h>\n"

		"#include <X11/Xlib.h>\n"
		"#include <X11/xpm.h>\n"
		"#include <X11/extensions/shape.h>\n"

		"#include \"wmgeneral.h\"\n\n"

		"/* X11 Variables */\n"
		"int\t\tscreen;\n"
		"int\t\tx_fd;\n"
		"int\t\td_depth;\n"
		"XSizeHints\tmysizehints;\n"
		"XWMHints\tmywmhints;\n"
		"Pixel\t\tback_pix, fore_pix;\n"
		"char\t\t*Geometry = \"\";\n"
		"Window\t\ticonwin, win;\n"
		"Pixmap\t\tpixmask;\n\n"

		"/* Mouse Regions */\n"
		"typedef struct {\n"
		"\tint\tenable;\n"
		"\tint\ttop;\n"
		"\tint\tbottom;\n"
		"\tint\tleft;\n"
		"\tint\tright;\n"
		"} MOUSE_REGION;\n\n"

		"#define MAX_MOUSE_REGION (8)\n"
		"MOUSE_REGION	mouse_region[MAX_MOUSE_REGION];\n\n"

		"/* Function Prototypes */\n"
		"static void GetXPM(XpmIcon *, char **);\n"
		"static Pixel GetColor(char *);\n"
		"void RedrawWindow(void);\n"
		"void AddMouseRegion(int, int, int, int, int);\n"
		"int CheckMouseRegion(int, int);\n\n"

		"/* read_rc_file */\n"										
		"void parse_rcfile(const char *filename, rckeys *keys)\n"
		"{\n"
		"\tchar	*p;\n"
		"\tchar	temp[128];\n"
		"\tchar	*tokens = \" :\t\\n\";\n"
		"\tFILE	*fp;\n"
		"\tint	i,key;\n\n"

		"\tfp = fopen(filename, \"r\");\n"
		"\tif (fp) {\n"
		"\t\twhile (fgets(temp, 128, fp)) {\n"
		"\t\t\tkey = 0;\n"
		"\t\t\t\twhile (key >= 0 && keys[key].label) {\n"
		"\t\t\t\t\tif ((p = strstr(temp, keys[key].label))) {\n"
		"\t\t\t\t\t\tp += strlen(keys[key].label);\n"
		"\t\t\t\t\t\tp += strspn(p, tokens);\n"
		"\t\t\t\t\t\tif ((i = strcspn(p, \"#\\n\"))) p[i] = 0;\n"
		"\t\t\t\t\t\tfree(*keys[key].var);\n"
		"\t\t\t\t\t\t*keys[key].var = strdup(p);\n"
		"\t\t\t\t\t\tkey = -1;\n"
		"\t\t\t\t\t} else key++;\n"
		"\t\t\t\t}\n"
		"\t\t\t}\n"
		"\t\tfclose(fp);\n"
		"\t}\n"
		"}\n\n"
		
		"static void GetXPM(XpmIcon *wmgen, char *pixmap_bytes[])\n"
		"{\n"
		"\tXWindowAttributes	attributes;\n"
		"\tint			err;\n\n"
		"\t/* For the colormap */\n"
		"\tXGetWindowAttributes(display, Root, &attributes);\n\n"
		"\twmgen->attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);\n\n"
		"\terr = XpmCreatePixmapFromData(display, Root, pixmap_bytes, &(wmgen->pixmap),\n"
		"\t\t&(wmgen->mask), &(wmgen->attributes));\n\n"
	
		"\tif (err != XpmSuccess) {\n"
		"\t\tfprintf(stderr, \"Not enough free colorcells.\\n\");\n"
		"\t\texit(1);\n"
		"\t}\n"
		"}\n\n"

		"static Pixel GetColor(char *name)\n"
		"{\n"
		"\tXColor		color;\n"
		"\tXWindowAttributes	attributes;\n\n"

		"\tXGetWindowAttributes(display, Root, &attributes);\n\n"

		"\tcolor.pixel = 0;\n"
		"\tif (!XParseColor(display, attributes.colormap, name, &color)) {\n"
		"\t\tfprintf(stderr, \"wm.app: can't parse %%s.\\n\", name);\n"
		"\t} else if (!XAllocColor(display, attributes.colormap, &color)) {\n"
		"\t\tfprintf(stderr, \"wm.app: can't allocate %%s.\\n\", name);\n"
		"\t}\n"
		"\treturn color.pixel;\n"
		"}\n\n"
		
		"static int flush_expose(Window w)\n"
		"{\n"
		"\tXEvent	dummy;\n"
		"\tint		i=0;\n\n"

		"\twhile (XCheckTypedWindowEvent(display, w, Expose, &dummy))\n"
		"\t\ti++;\n"
		"\treturn i;\n"
		"}\n\n"

		"void RedrawWindow(void)\n"
		"{\n"
		"\tflush_expose(iconwin);\n"
		"\tXCopyArea(display, wmgen.pixmap, iconwin, NormalGC,\n"
		"\t\t0,0, wmgen.attributes.width, wmgen.attributes.height, 0,0);\n"
		"\tflush_expose(win);\n"
		"\tXCopyArea(display, wmgen.pixmap, win, NormalGC,\n"
		"\t\t0,0, wmgen.attributes.width, wmgen.attributes.height, 0,0);\n"
		"}\n\n"

		"void RedrawWindowXY(int x, int y)\n"
		"{\n"
		"\tflush_expose(iconwin);\n"
		"\tXCopyArea(display, wmgen.pixmap, iconwin, NormalGC,\n"
		"\t\tx,y, wmgen.attributes.width, wmgen.attributes.height, 0,0);\n"
		"\tflush_expose(win);\n"
		"\tXCopyArea(display, wmgen.pixmap, win, NormalGC,\n"
		"\t\tx,y, wmgen.attributes.width, wmgen.attributes.height, 0,0);\n"
		"}\n\n"

		"void AddMouseRegion(int index, int left, int top, int right, int bottom)\n"
		"{\n"
		"\tif (index < MAX_MOUSE_REGION) {\n"
		"\t\tmouse_region[index].enable = 1;\n"
		"\t\tmouse_region[index].top = top;\n"
		"\t\tmouse_region[index].left = left;\n"
		"\t\tmouse_region[index].bottom = bottom;\n"
		"\t\tmouse_region[index].right = right;\n"
		"\t}\n"
		"}\n\n"
		
		"int CheckMouseRegion(int x, int y)\n"
		"{\n"
		"\tint	i;\n\n"

		"\tfor (i=0; i<MAX_MOUSE_REGION; i++) {\n"
		"\t\tif (mouse_region[i].enable &&\n"
		"\t\t\tx <= mouse_region[i].right &&\n"
		"\t\t\tx >= mouse_region[i].left &&\n"
		"\t\t\ty <= mouse_region[i].bottom &&\n"
		"\t\t\ty >= mouse_region[i].top)\n"
		"\t\t\treturn (i-1);\n"
		"\t}\n"
		"\treturn -1;\n"
		"}\n\n"

		"void copyXPMArea(int x, int y, int sx, int sy, int dx, int dy)\n"
		"{\n"
		"\tXCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC, x, y, sx, sy, dx, dy);\n"
		"}\n\n"

		"void copyXBMArea(int x, int y, int sx, int sy, int dx, int dy)\n"
		"{\n"
		"\tXCopyArea(display, wmgen.mask, wmgen.pixmap, NormalGC, x, y, sx, sy, dx, dy);\n"
		"}\n\n"

		"void setMaskXY(int x, int y)\n"
		"{\n"
		"\tXShapeCombineMask(display, win, ShapeBounding, x, y, pixmask, ShapeSet);\n"
		"\tXShapeCombineMask(display, iconwin, ShapeBounding, x, y, pixmask, ShapeSet);\n"
		"}\n\n"

		"void openXwindow(int argc, char *argv[], char *pixmap_bytes[], char *pixmask_bits, int pixmask_width, int pixmask_height)\n"
		"{\n"
		"\tunsigned int	borderwidth = 1;\n"
		"\tXClassHint	classHint;\n"
		"\tchar		*display_name = NULL;\n"
		"\tchar		*wname = argv[0];\n"
		"\tXTextProperty	name;\n"
		"\tXGCValues	gcv;\n"
		"\tunsigned long	gcm;\n"
		"\tint		i, dummy = 0;\n\n"

		"\tfor (i=1; argv[i]; i++) {\n"
		"\t\tif (!strcmp(argv[i], \"-display\"))\n"
		"\t\t\tdisplay_name = argv[i+1];\n"
		"\t}\n\n"

		"\tif (!(display = XOpenDisplay(display_name))) {\n"
		"\t\tfprintf(stderr, \"%%s: can't open display %%s\\n\",\n"
		"\t\t\twname, XDisplayName(display_name));\n"
		"\t\texit(1);\n"
		"\t}\n"
		"\tscreen  = DefaultScreen(display);\n"
		"\tRoot    = RootWindow(display, screen);\n"
		"\td_depth = DefaultDepth(display, screen);\n"
		"\tx_fd    = XConnectionNumber(display);\n\n"

		"\t/* Convert XPM to XImage */\n"
		"\tGetXPM(&wmgen, pixmap_bytes);\n\n"
		"\t/* Create a window to hold the stuff */\n"
		"\tmysizehints.flags = USSize | USPosition;\n"
		"\tmysizehints.x = 0;\n"
		"\tmysizehints.y = 0;\n\n"

		"\tback_pix = GetColor(\"white\");\n"
		"\tfore_pix = GetColor(\"black\");\n\n"

		"\tXWMGeometry(display, screen, Geometry, NULL, borderwidth, &mysizehints,\n"
		"\t\t&mysizehints.x, &mysizehints.y,&mysizehints.width,&mysizehints.height, &dummy);\n\n"

		"\tmysizehints.width = 64;\n"
		"\tmysizehints.height = 64;\n\n"
		
		"\twin = XCreateSimpleWindow(display, Root, mysizehints.x, mysizehints.y,\n"
		"\t\tmysizehints.width, mysizehints.height, borderwidth, fore_pix, back_pix);\n\n"
	
		"\ticonwin = XCreateSimpleWindow(display, win, mysizehints.x, mysizehints.y,\n"
		"\t\tmysizehints.width, mysizehints.height, borderwidth, fore_pix, back_pix);\n\n"

		"\t/* Activate hints */\n"
		"\tXSetWMNormalHints(display, win, &mysizehints);\n"
		"\tclassHint.res_name = wname;\n"
		"\tclassHint.res_class = wname;\n"
		"\tXSetClassHint(display, win, &classHint);\n\n"

		"\tXSelectInput(display, win, ButtonPressMask | ExposureMask | ButtonReleaseMask |\n"
		"\t\tPointerMotionMask | StructureNotifyMask);\n"
		"\tXSelectInput(display, iconwin, ButtonPressMask | ExposureMask | ButtonReleaseMask |\n"
		"\t\tPointerMotionMask | StructureNotifyMask);\n\n"

		"\tif (XStringListToTextProperty(&wname, 1, &name) == 0) {\n"
		"\t\tfprintf(stderr, \"%%s: can't allocate window name\\n\", wname);\n"
		"\t\texit(1);\n"
		"\t}\n\n"

		"\tXSetWMName(display, win, &name);\n\n"

		"\t/* Create GC for drawing */\n"
		"\tgcm = GCForeground | GCBackground | GCGraphicsExposures;\n"
		"\tgcv.foreground = fore_pix;\n"
		"\tgcv.background = back_pix;\n"
		"\tgcv.graphics_exposures = 0;\n"
		"\tNormalGC = XCreateGC(display, Root, gcm, &gcv);\n\n"

		"\t/* ONLYSHAPE ON */\n"
		"\tpixmask = XCreateBitmapFromData(display, win, pixmask_bits, pixmask_width, pixmask_height);\n"
		"\tXShapeCombineMask(display, win, ShapeBounding, 0, 0, pixmask, ShapeSet);\n"
		"\tXShapeCombineMask(display, iconwin, ShapeBounding, 0, 0, pixmask, ShapeSet);\n\n"

		"\t/* ONLYSHAPE OFF */\n"
		"\tmywmhints.initial_state = WithdrawnState;\n"
		"\tmywmhints.icon_window = iconwin;\n"
		"\tmywmhints.icon_x = mysizehints.x;\n"
		"\tmywmhints.icon_y = mysizehints.y;\n"
		"\tmywmhints.window_group = win;\n"
		"\tmywmhints.flags = StateHint | IconWindowHint | IconPositionHint | WindowGroupHint;\n\n"

		"\tXSetWMHints(display, win, &mywmhints);\n\n"

		"\tXSetCommand(display, win, argv, argc);\n"
		"\tXMapWindow(display, win);\n"
		"}\n");
	fclose (fp);
	g_free (filename);
  
	/* pixmaps.h */
	filename = g_strconcat (src_dir, "/pixmaps.h", NULL);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	fprintf(fp,
		"#define xpm_mask_width 64\n"
		"#define xpm_mask_height 64\n\n"

		"static char * xpm_master[] = {\n"
		"\"64 64 2 1\",\n"
		"\" 	c None\",\n"
		"\".	c #000000\",\n"
		"\"                                                                \",\n"
		"\"                                                                \",\n"
		"\"                                                                \",\n"
		"\"                                                                \",\n"
		"\"                                                                \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"     ......................................................     \",\n"
		"\"                                                                \",\n"
		"\"                                                                \",\n"
		"\"                                                                \",\n"
		"\"                                                                \",\n"
		"\"                                                                \"};\n\n"

		"static char xpm_mask_bits[] = {\n"
		"0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\n"
		"0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\n"
		"0x00,0x00,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,\n"
		"0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,\n"
		"0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,\n"
		"0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,\n"
		"0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,\n"
		"0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,\n"
		"0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,\n"
		"0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,\n"
		"0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,\n"
		"0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,\n"
		"0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,\n"
		"0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,\n"
		"0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,\n"
		"0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,\n"
		"0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,\n"
		"0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,\n"
		"0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,\n"
		"0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,\n"
		"0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,\n"
		"0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,\n"
		"0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,\n"
		"0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,\n"
		"0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,\n"
		"0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,\n"
		"0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,\n"
		"0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,\n"
		"0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,\n"
		"0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,\n"
		"0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,\n"
		"0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,\n"
		"0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\n"
		"0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\n"
		"0x00,0x00};\n"
	);
	fclose (fp);
	g_free (filename);
  
	/* main.c */
	filename = g_strconcat (src_dir, "/main.c", NULL);
	g_free (src_dir);

	/* FIXME: If main.c exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Unable to create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	fprintf(fp,
		"/* Created by Anjuta version %s */\n", VERSION);
	fprintf(fp,
		"/*\tThis file will not be overwritten */\n\n");
	fprintf(fp,
		"#ifdef HAVE_CONFIG_H\n"
		"#  include <config.h>\n"
		"#endif\n"
		"#include <stdio.h>\n"
		"#include <X11/X.h>\n"
		"#include <X11/xpm.h>\n"
		"#include <X11/Xlib.h>\n\n"

		"#include \"wmgeneral.h\"\n"
		"#include \"pixmaps.h\"\n\n"

		"/* Prototypes */\n"
		"static void print_usage(void);\n"
		"static void ParseCMDLine(int argc, char *argv[]);\n\n"

		"static void print_usage(void)\n"
		"{\n"
		"\tprintf(\"\\nHello Dock App version: %%s\\n\", VERSION);\n"
		"\tprintf(\"\\nTODO: Write This.\\n\\n\");\n"
		"}\n\n"

		"void ParseCMDLine(int argc, char *argv[])\n"
		"{\n"
		"\tint	i;\n\n"
    
		"\tfor (i = 1; i < argc; i++) {\n"
		"\t\tif (!strcmp(argv[i], \"-display\")) {\n"
		"\t\t\t++i; /* -display is used in wmgeneral */\n"
		"\t\t/*} else if (!strcmp(argv[i], \"-option_with_param\")) {\n"
		"\t\t\tstrcpy(param, argv[++i]);\n"
		"\t\t} else if (!strcmp(argv[i], \"-option_wo_param\")) {\n"
		"\t\t\tparam = 1;*/\n"
		"\t\t} else {\n"
		"\t\t\tprint_usage();\n"
		"\t\t\texit(1);\n"
		"\t\t}\n"
		"\t}\n"
		"}\n\n"

		"int main(int argc, char *argv[])\n"
		"{\n"
		"\tXEvent	event;\n\n"
  
		"\tParseCMDLine(argc, argv);\n"
		"\topenXwindow(argc, argv, xpm_master, xpm_mask_bits, xpm_mask_width, xpm_mask_height);\n\n"
    
		"\t/* Loop Forever */\n"
		"\twhile (1) {\n"
		"\t\t/* Process any pending X events. */\n"
		"\t\twhile (XPending(display)) {\n"
		"\t\t\tXNextEvent(display, &event);\n"
		"\t\t\t\tswitch (event.type) {\n"
		"\t\t\t\t\tcase Expose:\n"
		"\t\t\t\t\t\tRedrawWindow();\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t\tcase ButtonPress:\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t\tcase ButtonRelease:\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t}\n"
		"\t\t}\n"
		"\t\tusleep(10000);\n"
		"\t}\n\n"
    
		"\t/* we should never get here */\n"
		"\treturn (0);\n"
		"}\n");
	fclose (fp);
	g_free (filename);

	return TRUE;
}

gboolean
source_write_build_files (ProjectDBase * data)
{
	Project_Type* type;
	gint ret;

	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data->project_is_open, FALSE);

	/* project is blocked, don't bother to disturb the
	project */
	if (data->project_config->blocked)
		return TRUE;
	
	ret = source_write_autogen_sh (data);
	if (!ret) return FALSE;
	ret = source_write_acconfig_h (data);
	if (!ret) return FALSE;
	ret = source_write_acinclude_m4 (data);
	if (!ret) return FALSE;
	ret = source_write_setup_gettext (data);
	if (!ret) return FALSE;

	type = project_dbase_get_project_type (data);
	if (type->glade_support)
	{
		ret = source_write_glade_file (data);
		if (!ret) return FALSE;
	}
	if (type->gnome_support)
	{
		ret = source_write_desktop_entry (data);
		if (!ret) return FALSE;
	}
	if (type->gnome_support)
	{
		ret = source_write_macros_files (data);
		if (!ret) return FALSE;
	}
	free_project_type(type);

	if (data->project_config->disable_overwrite[BUILD_FILE_CONFIGURE_IN] == FALSE)
	{
		ret = source_write_configure_in (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_TOP_MAKEFILE_AM] == FALSE)
	{
		ret = source_write_toplevel_makefile_am (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_SOURCE] == FALSE)
	{
		ret = source_write_source_files (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_INCLUDE] == FALSE)
	{
		ret = source_write_include_files (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_HELP] == FALSE)
	{
		ret = source_write_help_files (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_PIXMAP] == FALSE)
	{
		ret = source_write_pixmap_files (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_DATA] == FALSE)
	{
		ret = source_write_data_files (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_DOC] == FALSE)
	{
		ret = source_write_doc_files (data);
		if (!ret) return FALSE;
	}
	if (data->project_config->disable_overwrite[BUILD_FILE_PO] == FALSE)
	{
		ret = source_write_po_files (data);
		if (!ret) return FALSE;
	}

	ret = source_create_file_if_not_exist (data->top_proj_dir, "TODO", NULL);
	if (!ret) return FALSE;
	ret = source_create_file_if_not_exist (data->top_proj_dir, "NEWS", NULL);
	if (!ret) return FALSE;
	ret = source_create_file_if_not_exist (data->top_proj_dir, "README", NULL);
	if (!ret) return FALSE;
	ret = source_create_file_if_not_exist (data->top_proj_dir, "AUTHORS", NULL);
	if (!ret) return FALSE;
	ret = source_create_file_if_not_exist (data->top_proj_dir, "ChangeLog", NULL);
	if (!ret) return FALSE;
	ret = source_create_file_if_not_exist (data->top_proj_dir, "stamp-h.in", "timestamp\n");
	if (!ret) return FALSE;
	return TRUE;
}
