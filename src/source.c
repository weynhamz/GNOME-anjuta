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
			anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), destbuffer);
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
	gchar *filename, *actual_file, *target, *version;
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		return FALSE;
	}
	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
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
		 "AC_ISC_POSIX\n", target, version);
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
			 "\"\n" "AM_GNU_GETTEXT\n" "\n");
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
		fprintf (fp, "%s.desktop\n", target);
	}
	fprintf (fp, "])\n\n");
	fclose (fp);

	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
	g_free (filename);
	g_free (actual_file);
	g_free (target);
	g_free (version);
	return TRUE;
}

static gboolean
source_write_toplevel_makefile_am (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename, *actual_file, *target;
	gint i;
	Project_Type* type;

	g_return_val_if_fail (data != NULL, FALSE);

	filename = get_a_tmp_file();
	actual_file = g_strconcat (data->top_proj_dir, "/Makefile.am", NULL);
	type = project_dbase_get_project_type(data);
	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
	target =
		prop_get (data->props, "project.source.target");
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
		"\tTODO", target, target, target);
	if (prop_get_int (data->props, "project.has.gettext", 1))
	{
		fprintf (fp,
			"\\\n\tABOUT-NLS");
	}
	fprintf (fp,
		"\n\nEXTRA_DIST = %s $(%sdoc_DATA)\n\n",
		extract_filename (data->proj_filename), target);
	if(type->gnome_support)
	{
		gchar *group;
		
		group = prop_get (data->props, "project.menu.group");
		if (!group) group = g_strdup ("Applications");
		fprintf (fp, "gnomemenudir = $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@/%s\n", group);
		fprintf (fp, "gnomemenu_DATA = %s.desktop\n\n", target);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	g_free (filename);
	free_project_type(type);
	return TRUE;
}

static gboolean
source_write_macros_files (ProjectDBase * data)
{
	static const gchar *files[] = {
		"ChangeLog",
		"Makefile.am",
		"aclocal-include.m4",
		"autogen.sh",
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
	/* This is the maximum length of the above filenames, so we can use the
	 * same buffers for each file. */
	static const gint MAX_MACROS_FILENAME_LEN = 64;
	gchar *srcbuffer, *destbuffer;
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

	for (i = 0; files[i]; i++)
	{
		sprintf (srcbuffer, "%s/gnome/%s", app->dirs->data, files[i]);
		sprintf (destbuffer, "%s/macros/%s", data->top_proj_dir, files[i]);
		if (file_is_regular (destbuffer) == FALSE)
		{
			if (!copy_file (srcbuffer, destbuffer, FALSE))
			{
				anjuta_system_error (errno, _("Couldn't create file: %s."), destbuffer);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
		g_free (filename);
		free_project_type(type);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);
	/* If the project directory is the source directory, we need to output
	 * SUBDIRS here. */
	fprintf (fp, "INCLUDES =");
	fprintf (fp, type->cflags);
	compiler_options_set_prjcflags_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");

	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "bin_PROGRAMS = %s\n\n", target);
	fprintf (fp, "%s_SOURCES = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_SOURCE);

	fprintf (fp, "%s_LDADD = ", target);
	fprintf (fp, type->ldadd);
	compiler_options_set_prjlibs_in_file (app->compiler_options, fp);
	fprintf (fp, "\n\n");
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	free_project_type(type);
	g_free (filename);
	return TRUE;
}

static gboolean
source_write_static_lib_source_files (ProjectDBase * data)
{
	g_return_val_if_fail (data != NULL, FALSE);
	
	return TRUE;
}

static gboolean
source_write_dynamic_lib_source_files (ProjectDBase * data)
{
	g_return_val_if_fail (data != NULL, FALSE);
	
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
		g_free (filename);
		g_free (actual_file);
		free_project_type(type);
		return FALSE;
	}
	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);

	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "%s_pixmapsdir = $(prefix)/@NO_PREFIX_PACKAGE_PIXMAPS_DIR@\n\n", target);
	
	fprintf (fp, "%s_pixmaps_DATA = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_PIXMAP);

	fprintf (fp, "EXTRA_DIST = $(%s_pixmaps_DATA)\n", target);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
		g_free (filename);
		free_project_type(type);
		g_free (actual_file);
		return FALSE;
	}

	fprintf (fp,"## Process this file with automake to produce Makefile.in\n\n");
	source_write_no_modify_warning (data, fp);

	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	fprintf (fp, "%s_helpdir = $(prefix)/@NO_PREFIX_PACKAGE_HELP_DIR@/C\n\n", target);
	fprintf (fp, "%s_help_DATA = \\\n", target);
	source_write_module_file_list (data, fp, TRUE, NULL, MODULE_HELP);

	fprintf (fp, "EXTRA_DIST = $(%s_help_DATA)\n", target);
	fclose (fp);
	if (move_file_if_not_same(filename, actual_file) == FALSE)
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), actual_file);
	g_free (actual_file);
	g_free (target);
	g_free (filename);
	return TRUE;
}

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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
source_write_desktop_entry (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename, *target, *name, *comment, *term, *icon;
	gint t;

	g_return_val_if_fail (data != NULL, FALSE);

	target =
		prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	
	filename = g_strconcat (data->top_proj_dir, "/", target, ".desktop.in", NULL);

	/* FIXME: If *.desktop.in exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		g_free (target);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
		g_free (filename);
		return FALSE;
	}
	name = prop_get (data->props, "project.menu.entry");
	if (!name)
		name = g_strdup ("Dummy");
	comment = prop_get (data->props, "project.menu.commnent");
	if (!comment)
		comment = g_strdup ("No comment");
	icon = prop_get (data->props, "project.menu.icon");
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
		"Icon=@PACKAGE_PIXMAPS_DIR@/%s\n"
		"Terminal=%s\n"
		"MultipleArgs=false\n"
		"Type=Application\n\n",
		name, comment, target, icon, term);
	fclose (fp);

	g_free (filename);
	g_free (name);
	g_free (comment);
	g_free (icon);
	g_free (target);
	g_free (term);
	return TRUE;
}


static gboolean
source_write_glade_file (ProjectDBase * data)
{
	FILE *fp;
	gchar *filename, *target;
	gchar *prj, *src, *pix;
	Project_Type* type;
	gint lang;
	gboolean bOK = TRUE ;

	g_return_val_if_fail (data != NULL, FALSE);

	type = project_dbase_get_project_type (data);
	target =	prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	
	filename = g_strconcat (data->top_proj_dir, "/", target, ".glade", NULL);

	/* FIXME: If *.glade exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		free_project_type(type);
		g_free (target);
		return TRUE;
	}

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
		free_project_type(type);
		g_free (filename);
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
	if (lang == PROJECT_PROGRAMMING_LANGUAGE_C)
		fprintf(fp, "  <language>C</language>\n");
	else 
		fprintf(fp, "  <language>CPP</language>\n");
	
	if (!type->gnome_support)
		fprintf(fp, "  <gnome_support>False</gnome_support>\n");
	else
		fprintf(fp, "  <gnome_support>True</gnome_support>\n");
	
	if (prop_get_int (data->props, "project.has.gettex", 1))
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
	{		anjuta_system_error (errno, _("Error writing to : %s."), filename);
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
		anjuta_system_error (errno, _("Couldn't create file: %s."), filename);
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
	return TRUE;
}

gboolean
source_write_build_files (ProjectDBase * data)
{
	Project_Type* type;
	gint ret;

	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data->project_is_open, FALSE);

	ret = source_write_autogen_sh (data);
	if (!ret) return FALSE;
	ret = source_write_acconfig_h (data);
	if (!ret) return FALSE;

	type = project_dbase_get_project_type (data);
	if (type->glade_support)
	{
		switch (type->id)
		{
			case PROJECT_TYPE_GTK:
			case PROJECT_TYPE_GTKMM:
				ret = source_write_glade_file (data);
				if (!ret) return FALSE;
				break;
			case PROJECT_TYPE_GNOME:
			case PROJECT_TYPE_BONOBO:
			case PROJECT_TYPE_GNOMEMM:
				ret = source_write_desktop_entry (data);
				if (!ret) return FALSE;
				ret = source_write_glade_file (data);
				if (!ret) return FALSE;
				ret = source_write_macros_files (data);
				if (!ret) return FALSE;
				break;
			default:
				anjuta_error (_("Project type does not support glade."));
		}
			return FALSE;
	free_project_type(type);
	}

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
