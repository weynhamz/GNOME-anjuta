/*
 * clsGen.h Copyright (C) 2002  Dave Huseby
 * class_gen.c Copyright (C) 2005 Massimo Cora' [porting to Anjuta 2.x plugin style]
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */


/*
 * V 0.0.2 -- Generic Class Generator Plugins
 *	Author: Dave Huseby (huseby@linuxprogrammer.org)
 *	Notes: I was asked to generalize my C++ class generator into Anjuta's
 *	general class generator plugin.  This is the result.
 *
 * V 0.0.1 -- Generic C++ Class Generator Plugin
 * 	Author: Dave Huseby (huseby@linuxprogrammer.org)
 *
 * Based on the Gtk clsGen plugin by:
 *	Tom Dyas (tdyas@romulus.rutgers.edu)
 *	JP Rosevear (jpr@arcavia.com)
 *
 */

#include "class_gen.h"
#include "action-callbacks.h"
#include "plugin.h"
#include "class_logo.xpm"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>


#define IDENT_NAME	"ident.name"
#define IDENT_EMAIL	"ident.email"


/*
 *------------------------------------------------------------------------------
 * Foward Declarations
 *------------------------------------------------------------------------------
 */

gchar* GetDescr(void);
glong GetVersion(void);
gchar* GetMenuTitle(GModule* plugin, void* pUserData);
gchar* GetTooltipText(GModule* plugin, void* pUserData);
gchar *GetMenu(void);

static struct tm* GetNowTime(void);
static void generate_header (AnjutaClassGenPlugin * plugin, FILE* fpOut);
static void generate_source (AnjutaClassGenPlugin* plugin, FILE* fpOut);


gchar*
GetDescr()
{
	return g_strdup("Class Builder");
}

glong
GetVersion()
{
	return 0x10000L; 
}

gchar *GetMenu(void)
{
	return g_strdup("project");
}

gchar*
GetMenuTitle( GModule *plugin, void *pUserData )
{
	return g_strdup("Class Builder");
}

gchar*
GetTooltipText( GModule *plugin, void *pUserData ) 
{
   return g_strdup("Class Builder");
}

void
class_gen_message_box(const char* szMsg)
{
	/* Create the dialog using a couple of stock buttons */
	GtkWidget * msgBox = gnome_message_box_new
	(
		szMsg, 
		GNOME_MESSAGE_BOX_QUESTION,
		GNOME_STOCK_BUTTON_OK,
		NULL
	);
	
	gnome_dialog_run_and_close (GNOME_DIALOG (msgBox));
}

void 
class_gen_set_root (AnjutaClassGenPlugin* plugin, gchar* root_dir) 
{
	if (!plugin->top_dir || 0 != strcmp(plugin->top_dir, root_dir))
	{
		if (plugin->top_dir)
			g_free(plugin->top_dir);
		plugin->top_dir = g_strdup(root_dir);
	}	
}


static struct tm*
GetNowTime(void)
{
	time_t l_time;

	l_time = time(NULL);
	return localtime(&l_time);
}


gboolean
is_legal_class_name (const char *szClassName)
{
	int nLen, i;
	
	if(NULL == szClassName)
		return FALSE;
	
	nLen = strlen(szClassName);
	if(!nLen)
		return FALSE;
	
	if(!isalpha(szClassName[0]))
		return FALSE;
	
	for(i = 1; i < nLen; i ++)
	{
		if(!isalnum(szClassName[i]))
			return FALSE;
	}
	
	return TRUE;
}


gboolean
is_legal_file_name (const char *szFileName)
{
	int nLen;
	
	if(NULL == szFileName)
		return FALSE;
	
	nLen = strlen(szFileName);
	if(!nLen)
		return FALSE;
	
	return TRUE;
}


void
class_gen_generate (AnjutaClassGenPlugin *plugin)
{
	gboolean bOK = FALSE;
	gchar* szSrcDir; 
	gchar* szfNameHeader;
	gchar* szfNameSource;
	FILE* fpHeader;
	FILE* fpSource;
	IAnjutaProjectManager *pm;
	
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);
	
	g_return_if_fail(pm != NULL);
	
	/* set the root dir */
	szSrcDir = plugin->top_dir;
	
	/* FIXME?: we redirect both the source_module and the include_module
	into the same szSrcDir. Is this what we want? */
	if (!plugin->m_bUserSelectedHeader)
		szfNameHeader = g_strdup_printf ("%s/%s", szSrcDir
	  , plugin->m_szDeclFile);
	else
		szfNameHeader = g_strdup (plugin->m_szImplFile);
	
	if (!plugin->m_bUserSelectedSource)
		szfNameSource = g_strdup_printf ("%s/%s", szSrcDir
	  , plugin->m_szImplFile);
	else
		szfNameSource = g_strdup (plugin->m_szImplFile);

	if (!plugin->m_bInline)
	{
		if (file_is_directory (szSrcDir) == FALSE)
			mkdir (szSrcDir, 0755);
		
		fpHeader = fopen (szfNameHeader, "at");
		if (fpHeader != NULL)
		{
			generate_header (plugin, fpHeader);
			fflush (fpHeader);
			bOK = !ferror (fpHeader);
			fclose (fpHeader);
			fpHeader = NULL;
		}
		
		fpSource = fopen (szfNameSource, "at");
		if(fpSource != NULL)
		{
			generate_source (plugin, fpSource);
			fflush (fpSource);
			bOK = !ferror (fpSource);
			fclose (fpSource);
			fpSource = NULL;
		}
	}
	else
	{	
		if (file_is_directory (szSrcDir) == FALSE)
			mkdir (szSrcDir, 0755);
		
		fpHeader = fopen (szfNameHeader, "at");
		if(fpHeader != NULL)
		{
			generate_header (plugin, fpHeader);
			generate_source (plugin, fpHeader);
			fflush (fpHeader);
			bOK = !ferror (fpHeader);
			fclose (fpHeader);
			fpHeader = NULL;
		}
	}
	
	if(bOK)
	{
		if(!plugin->m_bInline) 
			ianjuta_project_manager_add_source (pm, szfNameSource, szfNameSource, NULL);
		ianjuta_project_manager_add_source (pm, szfNameHeader, szfNameHeader, NULL);
	}
	else
		class_gen_message_box(_("Error in importing files"));
	
	g_free(szfNameHeader);
	g_free(szfNameSource);
}


static void
generate_header (AnjutaClassGenPlugin*plugin, FILE* fpOut)
{
	int i;
	gchar* all_uppers = (gchar*)malloc((strlen(plugin->m_szClassName) + 1) * sizeof(gchar));
	strcpy(all_uppers, plugin->m_szClassName);
	for(i = 0; i < strlen(all_uppers); i++)
	{
		all_uppers[i] = toupper(all_uppers[i]);
	}

	if(strcmp (plugin->m_szClassType, "Generic C++ Class") == 0)
	{
		/* output a C++ header */
		fprintf(fpOut, "//\n// File: %s\n", plugin->m_szDeclFile);
		
		{
			gchar* username = anjuta_preferences_get (plugin->prefs, IDENT_NAME);
			gchar* email =	anjuta_preferences_get (plugin->prefs, IDENT_EMAIL);
			
			fprintf( fpOut, "// Created by: %s <%s>\n"
			  , username?username:"", email?email:"" );
			SAFE_FREE(username);
			SAFE_FREE(email);
		}

		{
			struct tm *t = GetNowTime();
			fprintf( fpOut, "// Created on: %s//\n\n", asctime(t) );
		}

		fprintf
		(
			fpOut,
			"#ifndef _%s_H_\n"
			"#define _%s_H_\n"
			"\n"
			"\n",
			all_uppers, 
			all_uppers
		);
		
		if(plugin->m_bInline)
		{
			/* output some nice deliniation comments */
			fprintf
			(
				fpOut,
				"//------------------------------------------------------------------------------\n"
				"// %s Declaration\n"
				"//------------------------------------------------------------------------------\n"
				"\n"
				"\n",
				plugin->m_szClassName
			);
		}
		
		if (strlen (plugin->m_szBaseClassName) > 0)
		{
			/* output class with inheritence */
			fprintf
			(
				fpOut, 
				"class %s : %s %s\n"
				"{\n"
				"\tpublic:\n"
				"\t\t%s();\n",
				plugin->m_szClassName, 
				plugin->m_szAccess,
				plugin->m_szBaseClassName,
				plugin->m_szClassName
			);
		}
		else
		{
			/* output class without inheritence */
			fprintf
			(
				fpOut, 
				"class %s\n"
				"{\n"
				"\tpublic:\n"
				"\t\t%s();\n",
				plugin->m_szClassName, 
				plugin->m_szClassName
			);
		}
		
		if (plugin->m_bVirtualDestructor)
		{
			/* output virtual destructor */
			fprintf
			(
				fpOut,
				"\t\tvirtual ~%s();\n",
				plugin->m_szClassName
			);
		}
		else
		{
			/* output non-virtual destructor */
			fprintf
			(
				fpOut,
				"\t\t ~%s();\n",
				plugin->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			"\t\n"
			"\t\t// %s interface\n"
			"\t\n"
			"\t\t// TODO: add member function declarations...\n"
			"\t\n"
			"\tprotected:\n"
			"\t\t// %s variables\n"
			"\t\n"
			"\t\t// TODO: add member variables...\n"
			"\t\n"
			"};\n"
			"\n"
			"\n",
			plugin->m_szClassName,
			plugin->m_szClassName
		);
		
		if(!plugin->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	//_%s_H_\n"
				"\n",
				all_uppers
			);
		}
	}
	else if (strcmp (plugin->m_szClassType, "GTK+ Class") == 0)
	{
		/* output a GTK+ class header */
		fprintf (fpOut, "/*\n * File: %s\n", plugin->m_szDeclFile);
		
		{
			gchar* username = anjuta_preferences_get (plugin->prefs, IDENT_NAME);
			gchar* email= anjuta_preferences_get (plugin->prefs, IDENT_EMAIL);
			
			fprintf( fpOut, " * Created by: %s <%s>\n"
			  , username?username:"", email?email:"" );
			SAFE_FREE(username);
			SAFE_FREE(email);
		}
		
		{
			struct tm *t = GetNowTime();
			fprintf( fpOut, " * Created on: %s */\n\n", asctime(t) );
		}
		
		
		fprintf
		(
			fpOut,
			"#ifndef _%s_H_\n"
			"#define _%s_H_\n"
			"\n",
			all_uppers, 
			all_uppers
		);
		
		/* output the includes */
		fprintf
		(
			fpOut,
			"#ifdef HAVE_CONFIG_H\n"
			"#  include <config.h>\n"
			"#endif\n"
			"\n"
			"#include <sys/types.h>\n"
			"#include <sys/stat.h>\n"
			"#include <unistd.h>\n"
			"#include <string.h>\n"
			"\n"
			"#include <gnome.h>\n"
			"\n"
			"\n"
		);
		
		if(plugin->m_bInline)
		{
			/* output some nice deliniation comments */
			fprintf
			(
				fpOut,
				"/*\n"
				" * %s Declaration\n"
				" */\n"
				"\n",
				plugin->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			"typedef struct td_test {\n"
			"\t/* TODO: put your data here */\n"
			"} %s, *%sPtr;\n"
			"\n"
			"\n",
			plugin->m_szClassName,
			plugin->m_szClassName
		);
		
		if(plugin->m_bInline)
		{
			fprintf
			(
				fpOut,
				"/*\n"
				" * %s Forward Declarations\n"
				" */\n"
				"\n",
				plugin->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			"%s* %s_new(void);\n"
			"void %s_delete(%s* self);\n"
			"gboolean %s_init(%s* self);\n"
			"void %s_end(%s* self);\n"
			"\n"
			"\n",
			plugin->m_szClassName,
			plugin->m_szClassName,
			plugin->m_szClassName,
			plugin->m_szClassName,
			plugin->m_szClassName,
			plugin->m_szClassName,
			plugin->m_szClassName,
			plugin->m_szClassName
		);
		
		if (!plugin->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	/*_%s_H_*/\n"
				"\n",
				all_uppers
			);
		}
	}	
	free (all_uppers);
}

static void
generate_source (AnjutaClassGenPlugin *plugin, FILE* fpOut)
{
	int i;
	gchar* all_uppers = (gchar*)malloc((strlen(plugin->m_szClassName) + 1) * sizeof(gchar));
	strcpy(all_uppers, plugin->m_szClassName);
	for(i = 0; i < strlen(all_uppers); i++)
	{
		all_uppers[i] = toupper(all_uppers[i]);
	}
	
	if(strcmp(plugin->m_szClassType, "Generic C++ Class") == 0)
	{
		if(!plugin->m_bInline)
		{
			fprintf(fpOut, "//\n// File: %s\n", plugin->m_szImplFile);
			
			{
				gchar* username =	anjuta_preferences_get (plugin->prefs, IDENT_NAME);
				gchar* email =  anjuta_preferences_get (plugin->prefs, IDENT_EMAIL);
				
				fprintf( fpOut, "// Created by: %s <%s>\n"
				, username?username:"", email?email:"");
				SAFE_FREE(username);
				SAFE_FREE(email);
			}
			
			{
				struct tm *t = GetNowTime();
				fprintf( fpOut, "// Created on: %s//\n\n", asctime(t) );
			}
			
			fprintf
			(
				fpOut,
				"#include \"%s\"\n"
				"\n"
				"\n",
				plugin->m_szDeclFile
			);
		}
		else
		{
			fprintf
			(
				fpOut,
				"//------------------------------------------------------------------------------\n"
				"// %s Implementation\n"
				"//------------------------------------------------------------------------------\n"
				"\n\n",
				plugin->m_szClassName
			);
		}
		
		if(strlen(plugin->m_szBaseClassName) > 0)
		{
			/* output constructor with inheritence */
			fprintf
			(
				fpOut,
				"%s::%s() : %s()\n",
				plugin->m_szClassName,
				plugin->m_szClassName,
				plugin->m_szBaseClassName
			);
		}
		else
		{
			/* output constructor without inheritence */
			fprintf
			(
				fpOut,
				"%s::%s()\n",
				plugin->m_szClassName,
				plugin->m_szClassName
			);	
		}
	
		fprintf
		(
			fpOut, 
			"{\n"
			"\t// TODO: put constructor code here\n"
			"}\n"
			"\n"
			"\n"
			"%s::~%s()\n"
			"{\n"
			"\t// TODO: put destructor code here\n"
			"}\n"
			"\n"
			"\n", 
			plugin->m_szClassName, 
			plugin->m_szClassName
		);
		
		if(plugin->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	//_%s_H_\n"
				"\n",
				all_uppers
			);
		}
	}
	else if (strcmp (plugin->m_szClassType, "GTK+ Class") == 0)
	{
		if (!plugin->m_bInline)
		{
			fprintf(fpOut, "/*\n * File: %s\n", plugin->m_szDeclFile);
			
			{
				gchar* username = anjuta_preferences_get (plugin->prefs, IDENT_NAME);
				gchar* email = anjuta_preferences_get (plugin->prefs, IDENT_EMAIL);
				
				fprintf( fpOut, " * Created by: %s <%s>\n"
				  , username?username:"", email?email:"");
			}

			{
				struct tm *t = GetNowTime();
				fprintf( fpOut, " * Created on: %s */\n\n", asctime(t) );
			}
			
			fprintf
			(
				fpOut,
				"#include \"%s\"\n"
				"\n"
				"\n",
				plugin->m_szDeclFile
			);
		}
		else
		{
			/* output some nice deliniation comments */
			fprintf
			(
				fpOut,
				"/*\n"
				" * %s Implementation\n"
				" */\n"
				"\n",
				plugin->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			
			/* output constructor */
			"%s* %s_new(void)\n"
			"{\n"
			"\t%s* self;\n"
			"\tself = g_new(%s, 1);\n"
			"\tif(NULL != self)\n"
			"\t{\n"
			"\t\tif(!%s_init(self))\n"
			"\t\t{\n"
			"\t\t\tg_free(self);\n"
			"\t\t\tself = NULL;\n"
			"\t\t}\n"
			"\t}\n"
			"\treturn self;\n"
			"}\n"
			"\n"
			"\n"
			
			/* output destructor */
			"void %s_delete(%s* self)\n"
			"{\n"
			"\tg_return_if_fail(NULL != self);\n"
			"\t%s_end(self);\n"
			"\tg_free(self);\n"
			"}\n"
			"\n"
			"\n"
			
			/* output init */
			"gboolean %s_init(%s* self)\n"
			"{\n"
			"\t/* TODO: put init code here */\n"
			"\n"
			"\treturn TRUE;\n"
			"}\n"
			"\n"
			"\n"
			
			/* output deinit */
			"void %s_end(%s* self)\n"
			"{\n"
			"\t/* TODO: put deinit code here */\n"
			"}\n"
			"\n"
			"\n",
			plugin->m_szClassName, 
			plugin->m_szClassName,
			plugin->m_szClassName, 
			plugin->m_szClassName, 
			plugin->m_szClassName, 
			plugin->m_szClassName, 
			plugin->m_szClassName, 
			plugin->m_szClassName, 
			plugin->m_szClassName, 
			plugin->m_szClassName, 
			plugin->m_szClassName,
			plugin->m_szClassName
		);
		
		if(plugin->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	/*_%s_H_*/\n"
				"\n",
				all_uppers
			);
		}
	}
	free(all_uppers);
}


void
class_gen_get_strings (AnjutaClassGenPlugin *plugin)
{
	SAFE_FREE(plugin->m_szClassName);
	SAFE_FREE(plugin->m_szDeclFile);
	SAFE_FREE(plugin->m_szImplFile);
	SAFE_FREE(plugin->m_szBaseClassName);
	SAFE_FREE(plugin->m_szAccess);
	SAFE_FREE(plugin->m_szClassType);
	plugin->m_szClassName = gtk_editable_get_chars(GTK_EDITABLE(plugin->entry_class_name),0, -1);
	plugin->m_szDeclFile = gtk_editable_get_chars(GTK_EDITABLE(plugin->entry_header_file),0, -1);
	plugin->m_szImplFile = gtk_editable_get_chars(GTK_EDITABLE(plugin->entry_source_file),0, -1);
	plugin->m_szBaseClassName = gtk_editable_get_chars(GTK_EDITABLE(plugin->entry_base_class), 0, -1);
	plugin->m_szAccess = gtk_editable_get_chars(GTK_EDITABLE(plugin->combo_access_entry), 0, -1);
	plugin->m_szClassType = gtk_editable_get_chars(GTK_EDITABLE(plugin->combo_class_type_entry), 0, -1);
	plugin->m_bVirtualDestructor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(plugin->checkbutton_virtual_destructor));
	plugin->m_bInline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(plugin->checkbutton_inline));
}


void
class_gen_init (AnjutaClassGenPlugin *plugin)
{
	plugin->m_bOK = FALSE;
	plugin->m_bUserEdited = FALSE;
	plugin->m_bUserSelectedHeader = FALSE;
	plugin->m_bUserSelectedSource = FALSE;
	plugin->m_bVirtualDestructor = FALSE;
	plugin->m_bInline = FALSE;
	plugin->m_szClassName = NULL;
	plugin->m_szDeclFile= NULL;
	plugin->m_szImplFile= NULL;
	plugin->m_szBaseClassName = NULL;
	plugin->m_szAccess = NULL;
	plugin->m_szClassType = NULL;
}


void
class_gen_del(AnjutaClassGenPlugin* plugin)
{
	SAFE_FREE(plugin->m_szClassName);
	SAFE_FREE(plugin->m_szDeclFile);
	SAFE_FREE(plugin->m_szImplFile);
	SAFE_FREE(plugin->m_szBaseClassName);
	SAFE_FREE(plugin->m_szAccess);
	SAFE_FREE(plugin->m_szClassType);
}


void
class_gen_show (AnjutaClassGenPlugin* plugin)
{
	if(NULL != class_gen_create_dialog_class (plugin))
	{
		gtk_widget_show (plugin->dlgClass);
		gtk_widget_grab_focus(plugin->entry_class_name);
		gtk_widget_set_sensitive(plugin->button_browse_header_file, FALSE);
		gtk_widget_set_sensitive(plugin->button_browse_source_file, FALSE);
		gtk_widget_set_sensitive(plugin->button_finish, FALSE);
	}
}


void
class_gen_create_code_class (AnjutaClassGenPlugin *plugin)
{
	class_gen_init (plugin);
	class_gen_show (plugin);
}


GtkWidget* 
class_gen_create_dialog_class (AnjutaClassGenPlugin* plugin)
{
  plugin->combo_access_items = NULL;
  plugin->combo_class_type_items = NULL;
  plugin->header_file_selection = NULL;
  plugin->source_file_selection = NULL;
  plugin->tooltips = gtk_tooltips_new ();
	
  plugin->dlgClass = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gnome_window_icon_set_from_default((GtkWindow *) plugin->dlgClass);
  gtk_object_set_data (GTK_OBJECT (plugin->dlgClass), "dlgClass", plugin->dlgClass);
  gtk_window_set_title (GTK_WINDOW (plugin->dlgClass), _("Class Builder"));
  gtk_window_set_default_size (GTK_WINDOW (plugin->dlgClass), 640, 480);
	
  plugin->fixed = gtk_fixed_new ();
  gtk_widget_ref (plugin->fixed);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "fixed", plugin->fixed,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->fixed);
  gtk_container_add (GTK_CONTAINER (plugin->dlgClass), plugin->fixed);

  plugin->button_help = gtk_button_new_with_label (_("Help"));
  gtk_widget_ref (plugin->button_help);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "button_help", plugin->button_help,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->button_help);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->button_help, 544, 424);
  gtk_widget_set_uposition (plugin->button_help, 544, 424);
  gtk_widget_set_usize (plugin->button_help, 80, 24);

  plugin->button_cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (plugin->button_cancel);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "button_cancel", plugin->button_cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->button_cancel);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->button_cancel, 448, 424);
  gtk_widget_set_uposition (plugin->button_cancel, 448, 424);
  gtk_widget_set_usize (plugin->button_cancel, 80, 24);

  plugin->button_finish = gtk_button_new_with_label (_("Finish"));
  gtk_widget_ref (plugin->button_finish);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "button_finish", plugin->button_finish,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->button_finish);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->button_finish, 352, 424);
  gtk_widget_set_uposition (plugin->button_finish, 352, 424);
  gtk_widget_set_usize (plugin->button_finish, 80, 24);

  plugin->entry_class_name = gtk_entry_new ();
  gtk_widget_ref (plugin->entry_class_name);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "entry_class_name", plugin->entry_class_name,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->entry_class_name);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->entry_class_name, 24, 152);
  gtk_widget_set_uposition (plugin->entry_class_name, 24, 152);
  gtk_widget_set_usize (plugin->entry_class_name, 184, 24);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->entry_class_name, _("Enter the name for the class you want to add."), NULL);

  plugin->entry_header_file = gtk_entry_new ();
  gtk_widget_ref (plugin->entry_header_file);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "entry_header_file", plugin->entry_header_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->entry_header_file);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->entry_header_file, 232, 152);
  gtk_widget_set_uposition (plugin->entry_header_file, 232, 152);
  gtk_widget_set_usize (plugin->entry_header_file, 160, 24);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->entry_header_file, _("Enter the declaration file name."), NULL);

  plugin->entry_source_file = gtk_entry_new ();
  gtk_widget_ref (plugin->entry_source_file);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "entry_source_file", plugin->entry_source_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->entry_source_file);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->entry_source_file, 440, 152);
  gtk_widget_set_uposition (plugin->entry_source_file, 440, 152);
  gtk_widget_set_usize (plugin->entry_source_file, 160, 24);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->entry_source_file, _("Enter the implementation file name."), NULL);

  plugin->button_browse_header_file = gtk_button_new_with_label (_("..."));
  gtk_widget_ref (plugin->button_browse_header_file);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "button_browse_header_file", plugin->button_browse_header_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->button_browse_header_file);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->button_browse_header_file, 392, 152);
  gtk_widget_set_uposition (plugin->button_browse_header_file, 392, 152);
  gtk_widget_set_usize (plugin->button_browse_header_file, 24, 24);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->button_browse_header_file, _("Browse for the declaration file name."), NULL);

  plugin->button_browse_source_file = gtk_button_new_with_label (_("..."));
  gtk_widget_ref (plugin->button_browse_source_file);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "button_browse_source_file", plugin->button_browse_source_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->button_browse_source_file);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->button_browse_source_file, 600, 152);
  gtk_widget_set_uposition (plugin->button_browse_source_file, 600, 152);
  gtk_widget_set_usize (plugin->button_browse_source_file, 24, 24);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->button_browse_source_file, _("Browse for the implementation file name."), NULL);

  plugin->entry_base_class = gtk_entry_new ();
  gtk_widget_ref (plugin->entry_base_class);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "entry_base_class", plugin->entry_base_class,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->entry_base_class);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->entry_base_class, 24, 200);
  gtk_widget_set_uposition (plugin->entry_base_class, 24, 200);
  gtk_widget_set_usize (plugin->entry_base_class, 184, 24);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->entry_base_class, _("Enter the name of the class your new class will inherit from."), NULL);

  plugin->colormap = gtk_widget_get_colormap(plugin->dlgClass);
  plugin->gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, plugin->colormap, &plugin->mask,
						    NULL, class_logo_xpm);
  plugin->pixmap_logo = gtk_pixmap_new(plugin->gdkpixmap, plugin->mask);
  gdk_pixmap_unref(plugin->gdkpixmap);
  gdk_bitmap_unref(plugin->mask);
  gtk_widget_ref (plugin->pixmap_logo);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "pixmap_logo", plugin->pixmap_logo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->pixmap_logo);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->pixmap_logo, 568, 8);
  gtk_widget_set_uposition (plugin->pixmap_logo, 568, 8);
  gtk_widget_set_usize (plugin->pixmap_logo, 64, 64);

  plugin->hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (plugin->hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "hseparator1", plugin->hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->hseparator1);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->hseparator1, 0, 72);
  gtk_widget_set_uposition (plugin->hseparator1, 0, 72);
  gtk_widget_set_usize (plugin->hseparator1, 640, 16);

  plugin->label_description = gtk_label_new (_("This plugin will create a class of the type you specify and add it to your project. "));
  gtk_widget_ref (plugin->label_description);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_description", plugin->label_description,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_description);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_description, 32, 40);
  gtk_widget_set_uposition (plugin->label_description, 32, 40);

  plugin->label_class_name = gtk_label_new (_("Class name: "));
  gtk_widget_ref (plugin->label_class_name);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_class_name", plugin->label_class_name,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_class_name);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_class_name, 24, 136);
  gtk_widget_set_uposition (plugin->label_class_name, 24, 136);
  gtk_label_set_justify (GTK_LABEL (plugin->label_class_name), GTK_JUSTIFY_LEFT);

  plugin->label_header_file = gtk_label_new (_("Header file:  "));
  gtk_widget_ref (plugin->label_header_file);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_header_file", plugin->label_header_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_header_file);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_header_file, 232, 136);
  gtk_widget_set_uposition (plugin->label_header_file, 232, 136);
  gtk_label_set_justify (GTK_LABEL (plugin->label_header_file), GTK_JUSTIFY_LEFT);

  plugin->label_source_file = gtk_label_new (_("Source file:  "));
  gtk_widget_ref (plugin->label_source_file);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_source_file", plugin->label_source_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_source_file);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_source_file, 440, 136);
  gtk_widget_set_uposition (plugin->label_source_file, 440, 136);
  gtk_label_set_justify (GTK_LABEL (plugin->label_source_file), GTK_JUSTIFY_LEFT);

  plugin->label_base_class = gtk_label_new (_("Base class:   "));
  gtk_widget_ref (plugin->label_base_class);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_base_class", plugin->label_base_class,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_base_class);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_base_class, 24, 184);
  gtk_widget_set_uposition (plugin->label_base_class, 24, 184);
  gtk_label_set_justify (GTK_LABEL (plugin->label_base_class), GTK_JUSTIFY_LEFT);

  plugin->label_access = gtk_label_new (_("Base class inheritance: "));
  gtk_widget_ref (plugin->label_access);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_access", plugin->label_access,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_access);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_access, 232, 184);
  gtk_widget_set_uposition (plugin->label_access, 232, 184);

  plugin->hseparator3 = gtk_hseparator_new ();
  gtk_widget_ref (plugin->hseparator3);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "hseparator3", plugin->hseparator3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->hseparator3);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->hseparator3, 0, 232);
  gtk_widget_set_uposition (plugin->hseparator3, 0, 232);
  gtk_widget_set_usize (plugin->hseparator3, 640, 16);

  plugin->hseparator2 = gtk_hseparator_new ();
  gtk_widget_ref (plugin->hseparator2);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "hseparator2", plugin->hseparator2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->hseparator2);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->hseparator2, 0, 400);
  gtk_widget_set_uposition (plugin->hseparator2, 0, 400);
  gtk_widget_set_usize (plugin->hseparator2, 640, 16);

  plugin->hseparator4 = gtk_hseparator_new ();
  gtk_widget_ref (plugin->hseparator4);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "hseparator4", plugin->hseparator4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->hseparator4);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->hseparator4, 0, 456);
  gtk_widget_set_uposition (plugin->hseparator4, 0, 456);
  gtk_widget_set_usize (plugin->hseparator4, 640, 16);

  plugin->checkbutton_inline = gtk_check_button_new_with_label (_("Inline the declaration and implementation"));
  gtk_widget_ref (plugin->checkbutton_inline);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "checkbutton_inline", plugin->checkbutton_inline,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->checkbutton_inline);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->checkbutton_inline, 232, 104);
  gtk_widget_set_uposition (plugin->checkbutton_inline, 232, 104);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->checkbutton_inline, _("If checked, the plugin will generate both the declaration and implementation in the header file and will not create a source file."), NULL);

  plugin->label_author = gtk_label_new (_("By:  Dave Huseby <huseby@linuxprogrammer.org>"));
  gtk_widget_ref (plugin->label_author);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_author", plugin->label_author,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_author);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_author, 0, 464);
  gtk_widget_set_uposition (plugin->label_author, 0, 464);
  gtk_widget_set_sensitive (plugin->label_author, FALSE);
  gtk_label_set_justify (GTK_LABEL (plugin->label_author), GTK_JUSTIFY_LEFT);

  plugin->label_version = gtk_label_new (_("V 0.0.2 (June 11, 2002)"));
  gtk_widget_ref (plugin->label_version);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_version", plugin->label_version,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_version);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_version, 528, 464);
  gtk_widget_set_uposition (plugin->label_version, 528, 464);
  gtk_widget_set_sensitive (plugin->label_version, FALSE);
  gtk_label_set_justify (GTK_LABEL (plugin->label_version), GTK_JUSTIFY_RIGHT);

  plugin->label_todo = gtk_label_new (_("TODO: add the ability to declare member functions and their parameters here."));
  gtk_widget_ref (plugin->label_todo);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_todo", plugin->label_todo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_todo);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_todo, 120, 312);
  gtk_widget_set_uposition (plugin->label_todo, 120, 312);
  gtk_widget_set_sensitive (plugin->label_todo, FALSE);

  plugin->label_welcome = gtk_label_new (_("Welcome to the Anjuta Class Builder. "));
  gtk_widget_ref (plugin->label_welcome);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_welcome", plugin->label_welcome,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_welcome);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_welcome, 32, 16);
  gtk_widget_set_uposition (plugin->label_welcome, 32, 16);

  plugin->label_class_type = gtk_label_new (_("Class type:   "));
  gtk_widget_ref (plugin->label_class_type);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "label_class_type", plugin->label_class_type,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->label_class_type);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->label_class_type, 24, 88);
  gtk_widget_set_uposition (plugin->label_class_type, 24, 88);
  gtk_label_set_justify (GTK_LABEL (plugin->label_class_type), GTK_JUSTIFY_LEFT);

  plugin->combo_access = gtk_combo_new ();
  gtk_widget_ref (plugin->combo_access);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "combo_access", plugin->combo_access,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->combo_access);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->combo_access, 232, 200);
  gtk_widget_set_uposition (plugin->combo_access, 232, 200);
  gtk_widget_set_usize (plugin->combo_access, 184, 24);
  plugin->combo_access_items = g_list_append (plugin->combo_access_items, (gpointer) _("public"));
  plugin->combo_access_items = g_list_append (plugin->combo_access_items, (gpointer) _("protected"));
  plugin->combo_access_items = g_list_append (plugin->combo_access_items, (gpointer) _("private"));
  gtk_combo_set_popdown_strings (GTK_COMBO (plugin->combo_access), plugin->combo_access_items);
  g_list_free (plugin->combo_access_items);

  plugin->combo_access_entry = GTK_COMBO (plugin->combo_access)->entry;
  gtk_widget_ref (plugin->combo_access_entry);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "combo_access_entry", plugin->combo_access_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->combo_access_entry);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->combo_access_entry, _("Choose the type of inheritence from the base class."), NULL);
  gtk_entry_set_editable (GTK_ENTRY (plugin->combo_access_entry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (plugin->combo_access_entry), _("public"));

  plugin->checkbutton_virtual_destructor = gtk_check_button_new_with_label (_("Virtual destructor"));
  gtk_widget_ref (plugin->checkbutton_virtual_destructor);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "checkbutton_virtual_destructor", plugin->checkbutton_virtual_destructor,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->checkbutton_virtual_destructor);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->checkbutton_virtual_destructor, 440, 200);
  gtk_widget_set_uposition (plugin->checkbutton_virtual_destructor, 440, 200);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->checkbutton_virtual_destructor, _("If checked, the destructor of your class will be declared virtual."), NULL);

  plugin->combo_class_type = gtk_combo_new ();
  gtk_widget_ref (plugin->combo_class_type);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "combo_class_type", plugin->combo_class_type,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->combo_class_type);
  gtk_fixed_put (GTK_FIXED (plugin->fixed), plugin->combo_class_type, 24, 104);
  gtk_widget_set_uposition (plugin->combo_class_type, 24, 104);
  gtk_widget_set_usize (plugin->combo_class_type, 184, 24);
  plugin->combo_class_type_items = g_list_append (plugin->combo_class_type_items, (gpointer) _("Generic C++ Class"));
  plugin->combo_class_type_items = g_list_append (plugin->combo_class_type_items, (gpointer) _("GTK+ Class"));
  gtk_combo_set_popdown_strings (GTK_COMBO (plugin->combo_class_type), plugin->combo_class_type_items);
  g_list_free (plugin->combo_class_type_items);

  plugin->combo_class_type_entry = GTK_COMBO (plugin->combo_class_type)->entry;
  gtk_widget_ref (plugin->combo_class_type_entry);
  gtk_object_set_data_full (GTK_OBJECT (plugin->dlgClass), "combo_class_type_entry", plugin->combo_class_type_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin->combo_class_type_entry);
  gtk_tooltips_set_tip (plugin->tooltips, plugin->combo_class_type_entry, _("Select the type of class you would like to create."), NULL);
  gtk_entry_set_editable (GTK_ENTRY (plugin->combo_class_type_entry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (plugin->combo_class_type_entry), _("Generic C++ Class"));

  gtk_object_set_data (GTK_OBJECT (plugin->dlgClass), "tooltips", plugin->tooltips);

  /* setup callbacks */
  g_signal_connect (G_OBJECT (plugin->dlgClass), "delete_event",
					  GTK_SIGNAL_FUNC (on_delete_event),
					  plugin);
  g_signal_connect (G_OBJECT (plugin->dlgClass), "key-press-event",
					  GTK_SIGNAL_FUNC (on_class_gen_key_press_event),
					  plugin);
  g_signal_connect (G_OBJECT (plugin->button_browse_header_file), "clicked",
                      GTK_SIGNAL_FUNC (on_header_browse_clicked),
                      plugin);
  g_signal_connect (G_OBJECT (plugin->button_browse_source_file), "clicked",
                      GTK_SIGNAL_FUNC (on_source_browse_clicked),
                      plugin);
  g_signal_connect (G_OBJECT (plugin->button_finish), "clicked",
					  GTK_SIGNAL_FUNC (on_finish_clicked),
					  plugin);
  g_signal_connect (G_OBJECT (plugin->button_cancel), "clicked",
					  GTK_SIGNAL_FUNC (on_cancel_clicked),
					  plugin);
  g_signal_connect (G_OBJECT (plugin->button_help), "clicked",
					  GTK_SIGNAL_FUNC (on_help_clicked),
					  plugin);
  g_signal_connect (G_OBJECT (plugin->entry_class_name), "changed",
                      GTK_SIGNAL_FUNC (on_class_name_changed),
                      plugin);
  g_signal_connect (G_OBJECT (plugin->combo_class_type_entry), "changed",
                      GTK_SIGNAL_FUNC (on_class_type_changed),
                      plugin);
  g_signal_connect (G_OBJECT (plugin->checkbutton_inline), "toggled",
					  GTK_SIGNAL_FUNC (on_inline_toggled),
					  plugin);
  gtk_widget_grab_focus(plugin->entry_class_name);
  return plugin->dlgClass;
}
