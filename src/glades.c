/*
 * glades.c glade CORBA interface
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <errno.h>
#include <sys/wait.h>

#include "anjuta.h"
#include "project_dbase.h"
#include "glade_iface.h"
#include "CORBA-Server.h"
#include "glades.h"

#if 0
gboolean
cglade_iface_generate_source_code(gchar* glade_file)
{
	gchar *dir;
	pid_t pid;
	int status;
	gboolean ret;

	ret = TRUE;
	g_return_val_if_fail (glade_file != NULL, FALSE);

	switch (project_dbase_get_language (app->project_dbase))
	{
		case PROJECT_PROGRAMMING_LANGUAGE_C:
			if (anjuta_is_installed ("glade", TRUE) == FALSE)
				return FALSE;
			
			dir = g_dirname (glade_file);
			if(dir)
			{
				force_create_dir (dir);
				if (chdir (dir) < 0)
				{
					g_free (dir);
					return FALSE;
				}
			}	
			/* Cannot use gnome_execute_shell()*/
			if((pid = fork())==0)
			{
				execlp ("glade", "glade", "--write-source",  glade_file, NULL);
				g_error ("Cannot execute glade\n");
			}
			if(pid <1)
			{
				anjuta_system_error (errno, _("Cannot fork glade."));
				return FALSE;
			}
			waitpid (pid, &status, 0);
			if (WEXITSTATUS(status))
				ret = FALSE;
			g_free (dir);
			break;

		case PROJECT_PROGRAMMING_LANGUAGE_CPP:
		case PROJECT_PROGRAMMING_LANGUAGE_C_CPP:
			if (anjuta_is_installed ("glade--", TRUE) == FALSE)
				return FALSE;

			dir = g_dirname (glade_file);
			if(dir)
			{
				force_create_dir (dir);
				if (chdir (dir) < 0)
				{
					g_free (dir);
					return FALSE;
				}
			}
			g_free (dir);
			
			dir = project_dbase_get_module_name (app->project_dbase, MODULE_SOURCE);
			/* Cannot use gnome_execute_shell()*/
			if((pid = fork())==0)
			{
				execlp ("glade--", "glade--", "--noautoconf",
					"--directory", 
					dir, extract_filename(glade_file), NULL);
				g_error ("Cannot execute glade--\n");
			}
			if(pid <1)
			{
				anjuta_system_error (errno, _("Cannot fork glade--."));
				return FALSE;
			}
			waitpid (pid, &status, 0);
			if (WEXITSTATUS(status))
				ret = FALSE;
			g_free (dir);
			break;
		default:
			return FALSE;
	}
	return ret;
}
#endif

gboolean
gladen_start(void)
{
	if( app->bUseComponentUI )
	{
		gchar *szStrCmd;
		szStrCmd = g_strdup_printf( "gladen --SERVER=%s", GetCorbaManager()->m_serv.m_SrvStr );
		gnome_execute_shell (".", szStrCmd);
		g_free (szStrCmd);
	}
	return TRUE;
}


gboolean
gladen_load_project( const gchar *szFileName )
{
	CORBA_Environment	ev;
	gchar *glade_file;
	g_return_val_if_fail (szFileName != NULL, FALSE);
	
	if ( !IsGladen() )
		return FALSE;
	
	glade_file = strdup ( szFileName );
	g_return_val_if_fail (glade_file != NULL, FALSE);

	CInitEx( &ev );
	Gladen_GladeRef_LoadFile( GetCorbaManager()->m_gladen, szFileName, &ev );
	return TRUE;
}

gboolean 
gladen_add_main_components(void)
{
	CORBA_Object		gladen ;
	CORBA_Environment	ev;

	g_return_val_if_fail (app->project_dbase != NULL, FALSE);
	
	if ( !IsGladen() )
		return FALSE;
	
	gladen = GetCorbaManager()->m_gladen ;

	CInitEx( &ev );
	if (project_dbase_get_project_type (app->project_dbase)->id == PROJECT_TYPE_GTK ||
		project_dbase_get_project_type (app->project_dbase)->id == PROJECT_TYPE_GTK2)
	{
		Gladen_GladeRef_AddComponent( gladen, "GtkWindow", "MainWnd", &ev );
	}else
	{
		Gladen_GladeRef_AddComponent( gladen, "GnomeApp", "MainWnd", &ev );
	}
	CHK_EV(&ev);
	return TRUE;
}

#if 0
gboolean
gladen_write_glade_file ( ProjectDBase * data )
{
	CORBA_Object	gladen;
	FILE *fp;
	gchar *filename, *target, *prj_name;
	gchar *prj, *src, *pix;
	gint type, lang;

	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (IsGladen(), FALSE);
	
	gladen = GetCorbaManager()->m_gladen ;

	type = project_dbase_get_project_type (data);
	target =	prop_get (data->props, "project.source.target");
	g_strdelimit (target, "-", '_');

	prj_name = project_dbase_get_proj_name (data);	
	filename = g_strconcat (data->top_proj_dir, "/", prj_name, ".glade", NULL);
	g_free (prj_name);

	/* FIXME: If *.glade exists, just leave it, for now. */
	if (file_is_regular (filename))
	{
		g_free (filename);
		g_free (target);
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
	
	if (type == PROJECT_TYPE_GTK || type == PROJECT_TYPE_GTK2)
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
	fclose (fp);
	return TRUE;
}

	if (type == PROJECT_TYPE_GTK || type == PROJECT_TYPE_GTK2)
	{
		Gladen_GladeRef_AddComponent( gladen, "GtkWindow", "MainWnd", &ev );
		CHK_EV(&ev);
		Gladen_GladeRef__set_gnomeSupport( gladen, CORBA_FALSE, &ev );
		
	}else
	{
		Gladen_GladeRef_AddComponent( gladen, "GtkWindow", "MainWnd", &ev );
		CHK_EV(&ev);
		Gladen_GladeRef__set_gnomeSupport( gladen, CORBA_TRUE, &ev );	
	}
	CHK_EV(&ev);
	
	
	/* Alla fine... */
	Gladen_GladeRef_SaveFile( gladen, CORBA_FALSE, "",  &ev);
	if( GenerateSourceFile( gladen, &ev ) )
		tutto ok
	else
		false
	
   void Gladen_GladeRef_SetFileAttr(Gladen_GladeRef _obj,
				    const CORBA_char * szFullPathName,
				    CORBA_Environment * ev);
   CORBA_char *Gladen_GladeRef__get_name(Gladen_GladeRef _obj,
					 CORBA_Environment * ev);
   void Gladen_GladeRef__set_name(Gladen_GladeRef _obj,
				  const CORBA_char * value,
				  CORBA_Environment * ev);
   CORBA_char *Gladen_GladeRef__get_programName(Gladen_GladeRef _obj,
						CORBA_Environment * ev);
   void Gladen_GladeRef__set_programName(Gladen_GladeRef _obj,
					 const CORBA_char * value,
					 CORBA_Environment * ev);
   CORBA_char *Gladen_GladeRef__get_dir(Gladen_GladeRef _obj,
					CORBA_Environment * ev);
   void Gladen_GladeRef__set_dir(Gladen_GladeRef _obj,
				 const CORBA_char * value,
				 CORBA_Environment * ev);
   CORBA_char *Gladen_GladeRef__get_sourceDir(Gladen_GladeRef _obj,
					      CORBA_Environment * ev);
   void Gladen_GladeRef__set_sourceDir(Gladen_GladeRef _obj,
				       const CORBA_char * value,
				       CORBA_Environment * ev);
   CORBA_char *Gladen_GladeRef__get_pixmapsDir(Gladen_GladeRef _obj,
					       CORBA_Environment * ev);
   void Gladen_GladeRef__set_pixmapsDir(Gladen_GladeRef _obj,
					const CORBA_char * value,
					CORBA_Environment * ev);
   CORBA_char *Gladen_GladeRef__get_language(Gladen_GladeRef _obj,
					     CORBA_Environment * ev);
   void Gladen_GladeRef__set_language(Gladen_GladeRef _obj,
				      const CORBA_char * value,
				      CORBA_Environment * ev);
   CORBA_boolean Gladen_GladeRef__get_gnomeSupport(Gladen_GladeRef _obj,
						   CORBA_Environment * ev);
   void Gladen_GladeRef__set_gnomeSupport(Gladen_GladeRef _obj,
					  const CORBA_boolean value,
					  CORBA_Environment * ev);
   CORBA_boolean Gladen_GladeRef__get_outputSupportFiles(Gladen_GladeRef _obj,
							 CORBA_Environment *
							 ev);
   void Gladen_GladeRef__set_outputSupportFiles(Gladen_GladeRef _obj,
						const CORBA_boolean value,
						CORBA_Environment * ev);
   CORBA_boolean Gladen_GladeRef__get_outputMainFile(Gladen_GladeRef _obj,
						     CORBA_Environment * ev);
   void Gladen_GladeRef__set_outputMainFile(Gladen_GladeRef _obj,
					    const CORBA_boolean value,
					    CORBA_Environment * ev);
   CORBA_boolean Gladen_GladeRef_GenerateSourceFile(Gladen_GladeRef _obj,
						    CORBA_Environment * ev);


static gboolean
glade_project_options_check_valid	(GladeProjectOptions *options)
{
  gchar *error = NULL;
  gchar *directory, *xml_filename, *project_name, *program_name;
  gchar *source_directory, *pixmaps_directory;
  gboolean output_translatable_strings;
  gchar *translatable_strings_filename;

  directory = get_entry_text (options->directory_entry);
  xml_filename = get_entry_text (options->xml_filename_entry);
  project_name = get_entry_text (options->name_entry);
  program_name = get_entry_text (options->program_name_entry);
  source_directory = get_entry_text (options->source_directory_entry);
  pixmaps_directory = get_entry_text (options->pixmaps_directory_entry);
  output_translatable_strings = GTK_TOGGLE_BUTTON (options->output_translatable_strings)->active;
  translatable_strings_filename = get_entry_text (options->translatable_strings_filename_entry);

  switch (options->action)
    {
    case GLADE_PROJECT_OPTIONS_ACTION_NORMAL:
      if (output_translatable_strings
	  && (translatable_strings_filename == NULL
	      || translatable_strings_filename[0] == '\0'))
	error = _("You need to set the Translatable Strings File option");
      break;
    case GLADE_PROJECT_OPTIONS_ACTION_SAVE:
      if (directory == NULL || directory[0] == '\0')
	error = _("You need to set the Project Directory option");
      else if (xml_filename == NULL || xml_filename[0] == '\0')
	error = _("You need to set the Project File option");
      else if (output_translatable_strings
	       && (translatable_strings_filename == NULL
		   || translatable_strings_filename[0] == '\0'))
	error = _("You need to set the Translatable Strings File option");
      break;
    case GLADE_PROJECT_OPTIONS_ACTION_BUILD:
      if (directory == NULL || directory[0] == '\0')
	error = _("You need to set the Project Directory option");
      else if (xml_filename == NULL || xml_filename[0] == '\0')
	error = _("You need to set the Project File option");
      else if (output_translatable_strings
	       && (translatable_strings_filename == NULL
		   || translatable_strings_filename[0] == '\0'))
	error = _("You need to set the Translatable Strings File option");
      else if (project_name == NULL || project_name[0] == '\0')
	error = _("You need to set the Project Name option");
      else if (program_name == NULL || program_name[0] == '\0')
	error = _("You need to set the Program Name option");
#if 0
      else if (source_directory == NULL || source_directory[0] == '\0')
	error = _("You need to set the Source Directory option");
#endif
      else if (pixmaps_directory == NULL || pixmaps_directory[0] == '\0')
	error = _("You need to set the Pixmaps Directory option");
      break;
    }

  if (error)
    {
      glade_util_show_message_box (error, GTK_WIDGET (options));
      return FALSE;
    }

  return TRUE;
}

static void
glade_project_options_ok (GtkWidget *widget,
			  GladeProjectOptions *options)
{
  GladeProject *project;
  gchar *xml_filename, *directory, *source_directory, *pixmaps_directory;
  gchar *strings_filename;
  gint language;

  project = options->project;

  /* First check that the options are valid, according to the requested action.
     If not, we need to stop the signal so the dialog isn't closed. */
  if (!glade_project_options_check_valid (options))
    {
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "clicked");
      return;
    }

  /* The project directory is the only absolute path. Everything else is
     relative to it, so needs to be turned into an absolute path here. */
  directory = get_entry_text (options->directory_entry);
  glade_project_set_directory (project, directory);

  glade_project_set_name (project, get_entry_text (options->name_entry));
  glade_project_set_program_name (project, get_entry_text (options->program_name_entry));

  pixmaps_directory = get_entry_text (options->pixmaps_directory_entry);
  pixmaps_directory = glade_util_make_absolute_path (directory,
						     pixmaps_directory);
  glade_project_set_pixmaps_directory (project, pixmaps_directory);
  g_free (pixmaps_directory);

  source_directory = get_entry_text (options->source_directory_entry);
  source_directory = glade_util_make_absolute_path (directory,
						    source_directory);
  glade_project_set_source_directory (project, source_directory);
  g_free (source_directory);

  xml_filename = get_entry_text (options->xml_filename_entry);
  xml_filename = glade_util_make_absolute_path (directory, xml_filename);
  glade_project_set_xml_filename (project, xml_filename);
  g_free (xml_filename);

  for (language = 0; language < GladeNumLanguages; language++)
    {

      if (GTK_TOGGLE_BUTTON (options->language_buttons[language])->active)
	{
	  glade_project_set_language_name (project, GladeLanguages[language]);
	  break;
	}
    }


#ifdef USE_GNOME
  glade_project_set_gnome_support (project, GTK_TOGGLE_BUTTON (options->gnome_support)->active ? TRUE : FALSE);
#ifdef USE_GNOME_DB
  glade_project_set_gnome_db_support (project, GTK_TOGGLE_BUTTON (options->gnome_db_support)->active ? TRUE : FALSE);
#endif
#endif

  glade_project_set_gettext_support (project, GTK_TOGGLE_BUTTON (options->gettext_support)->active ? TRUE : FALSE);

  glade_project_set_use_widget_names (project, GTK_TOGGLE_BUTTON (options->use_widget_names)->active ? TRUE : FALSE);

  glade_project_set_output_main_file (project, GTK_TOGGLE_BUTTON (options->output_main_file)->active ? TRUE : FALSE);

  glade_project_set_output_support_files (project, GTK_TOGGLE_BUTTON (options->output_support_files)->active ? TRUE : FALSE);

  glade_project_set_output_build_files (project, GTK_TOGGLE_BUTTON (options->output_build_files)->active ? TRUE : FALSE);

  glade_project_set_backup_source_files (project, GTK_TOGGLE_BUTTON (options->backup_source_files)->active ? TRUE : FALSE);

  glade_project_set_gnome_help_support (project, GTK_TOGGLE_BUTTON (options->gnome_help_support)->active ? TRUE : FALSE);

  glade_project_set_source_files (project,
				  get_entry_text (options->main_source_entry),
				  get_entry_text (options->main_header_entry),
				  get_entry_text (options->handler_source_entry),
				  get_entry_text (options->handler_header_entry));
  glade_project_set_support_source_file (project, get_entry_text (options->support_source_entry));
  glade_project_set_support_header_file (project, get_entry_text (options->support_header_entry));

  glade_project_set_output_translatable_strings (project, GTK_TOGGLE_BUTTON (options->output_translatable_strings)->active ? TRUE : FALSE);

  strings_filename = get_entry_text (options->translatable_strings_filename_entry);
  strings_filename = glade_util_make_absolute_path (directory,
						    strings_filename);
  glade_project_set_translatable_strings_file (project, strings_filename);
  g_free (strings_filename);

  g_free (directory);
  gtk_widget_destroy (GTK_WIDGET (options));
}

#endif

gboolean
gladen_write_source( const gchar *szGladeFileName )
{
	gchar *dir;
	gboolean bRetValue = TRUE ;
	CORBA_Environment	ev;

	g_return_val_if_fail ( szGladeFileName != NULL, FALSE);
	g_return_val_if_fail ( IsGladen(), FALSE);
			
	dir = g_dirname ( szGladeFileName );
	if(dir)
	{
		force_create_dir (dir);
		if (chdir (dir) < 0)
		{
			bRetValue = FALSE;
		}
	}	
	g_free (dir);
	if( bRetValue )
	{
		CInitEx( &ev );
		bRetValue = (Gladen_GladeRef_GenerateSourceFile(GetCorbaManager()->m_gladen,
						    &ev) == CORBA_TRUE ) ? TRUE : FALSE ;
		CHK_EV(ev);
	}
	return bRetValue ;
}

