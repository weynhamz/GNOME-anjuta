/*
 * clsGen.h Copyright (C) 2002  Dave Huseby
 * GOCbuilder Copyright (C) 2004  Aaron Bockover <aaron@aaronbock.net>
 * class_gen.c Copyright (C) 2005 Massimo Cora' [porting to Anjuta 2.x plugin style]
 * 				
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
/* #include "class_logo.xpm" */

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
#include <libanjuta/interfaces/ianjuta-vcs.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>


/*
 *------------------------------------------------------------------------------
 * Foward Declarations
 *------------------------------------------------------------------------------
 */


static struct tm* GetNowTime(void);
static void gen_cpp_generate_header (ClassGenData* data, gboolean is_inline, FILE* fpOut);
static void gen_cpp_generate_source (ClassGenData *data, gboolean is_inline, const gchar* header_file, FILE* fpOut);



gchar *
browse_for_file (gchar *title)
{
	GtkWidget *dialog;
	gchar *filename = NULL;
	
	dialog = gtk_file_chooser_dialog_new (title,
		NULL,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	
	gtk_widget_destroy(dialog);
	
	return filename;
}



/*
 *------------------------------------------------------------------------------
 * GObject Builder
 *------------------------------------------------------------------------------
 */

gboolean
gobject_class_create_code (ClassGenData* data) {
	
	GtkWidget *classgen_widget;	
	gchar *trans_table[8];
	gboolean a, b;
	const gchar *base_class = FETCH_STRING (data->gxml, "go_base_class");
	const gchar *gtype_name = FETCH_STRING (data->gxml, "go_type_name");
	const gchar *gtype_prefix = FETCH_STRING (data->gxml, "go_type_prefix");
	const gchar *class_function_prefix = FETCH_STRING (data->gxml, "go_class_func_prefix");
	const gchar *source_file = FETCH_STRING (data->gxml, "go_source_file");
	const gchar *header_file = FETCH_STRING (data->gxml, "go_header_file");
	const gchar *author_name = FETCH_STRING (data->gxml, "go_author_name");
	const gchar *author_email = FETCH_STRING (data->gxml, "go_author_email");
	gboolean date_output = FETCH_BOOLEAN (data->gxml, "go_date_output");
	gboolean add_to_project = FETCH_BOOLEAN (data->gxml, "add_to_project_check");
	gboolean add_to_repository = FETCH_BOOLEAN (data->gxml, "add_to_repository_check");
	
	GtkWidget* license_widget = glade_xml_get_widget (data->gxml, "license_combo");
	gint license_output = gtk_combo_box_get_active (GTK_COMBO_BOX (license_widget));
	
	gchar *header_file_base; 
	gchar *header_define;
	gchar *t; 
	AnjutaClassGenPlugin *plugin;
	IAnjutaFileLoader *loader;
	
	plugin = data->plugin;
	
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaFileLoader, NULL);

	g_return_val_if_fail (loader != NULL, FALSE);

	classgen_widget = glade_xml_get_widget (data->gxml, "classgen_main");	
	
	/* check whether all required fields are filled or not */
	if ( g_str_equal (base_class, "") || g_str_equal (gtype_name, "") ||
		  g_str_equal (gtype_prefix, "") || g_str_equal (class_function_prefix, "") ||
		  g_str_equal (source_file, "") || g_str_equal (header_file, "") ) {
		anjuta_util_dialog_error (NULL, _("Please check your required fields."));
		return FALSE;
	}
		
	header_file_base = g_path_get_basename(header_file);
	header_define = cstr_replace_all(header_file_base, "-", "_");
	t = header_define;
	header_define = cstr_replace_all(t, ".", "_");
	g_free(t);		
	
	t = g_ascii_strup (header_define, strlen(header_define));
	g_free (header_define);
	header_define = t;
	
	trans_table[0] = (gchar *)base_class;
	trans_table[1] = (gchar *)gtype_name;
	trans_table[2] = (gchar *)gtype_prefix;
	trans_table[3] = (gchar *)class_function_prefix;
	trans_table[4] = header_file_base;
	trans_table[5] = header_define;

	/* act with templates... */
	a = transform_file(CLASS_TEMPLATE"/"CLASS_GOC_HEADER_TEMPLATE, 
					header_file, trans_table, author_name, author_email, 
					date_output, license_output);
					
	b = transform_file(CLASS_TEMPLATE"/"CLASS_GOC_SOURCE_TEMPLATE, 
					source_file, trans_table, author_name, author_email, 
					date_output, license_output);

	gtk_widget_hide (classgen_widget);
	
	if(a && b) {
		if (add_to_project) {
			IAnjutaProjectManager *pm;
			pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);
			
			g_return_val_if_fail (pm != NULL, FALSE);			
			
			ianjuta_project_manager_add_source (pm, source_file, source_file, NULL);
			ianjuta_project_manager_add_source (pm, header_file, header_file, NULL);
		}
		
		if (add_to_repository) {
			IAnjutaVcs *vcs;
			vcs = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
										IAnjutaVcs, NULL);

			g_return_val_if_fail (vcs != NULL, FALSE);
			ianjuta_vcs_add (vcs, source_file, NULL);
			ianjuta_vcs_add (vcs, header_file, NULL);
		}
		
		/* let's open the files in new a pair of new editors */
		ianjuta_file_loader_load (loader, source_file, FALSE, NULL);
		ianjuta_file_loader_load (loader, header_file, FALSE, NULL);
		
	} else {
		anjuta_util_dialog_error (NULL,
							  _("An error occurred when trying to write GObject Class Template. Check file permissions."));
	}
	
	g_free(header_file_base);
	g_free(header_define);	
	return TRUE;
}

gchar *
cstr_replace_all(gchar *search, const gchar *find, const gchar *replace)
{
	gint searchLen,
		findLen,
		replaceLen,
		zBufSize,
		skip;
	gchar *buffer = 0,
		*scan = 0,
		*curstr = 0;

	if(search == NULL)
		return NULL;
	
	searchLen = strlen(search);
	findLen = strlen(find);
	replaceLen = strlen(replace);

	zBufSize = searchLen + searchLen * replaceLen + 1;
	buffer = (gchar *)malloc(zBufSize);
	scan = buffer;

	if(buffer == 0)
		return 0;

	*scan = 0;

	while(1) {
		curstr = strstr(search, find);
		if(curstr == 0) {
			strcat(scan, search);
			break;
		} else {
			skip = curstr - search;
			memcpy(scan, search, skip);
			memcpy(scan + skip, replace, replaceLen);
			search = curstr + findLen;
			scan = scan + skip + replaceLen;
			*scan = 0;
		}
	}

	return (gchar *)realloc(buffer, strlen(buffer) + 1);
}


gboolean 
transform_file(const gchar *input_file, const gchar *output_file, 
	gchar **replace_table, const gchar *author, const gchar *email, 
	gboolean date_output, gint license_output)
{
	gchar *contents;
	gchar *working;
	gsize length;
	gint i, st_size;
	FILE *out;
	
	gchar *search_table[] = {
		"{{BASE_CLASS}}", 
		"{{GTYPE_NAME}}",
		"{{GTYPE_PREFIX}}",
		"{{FUNCTION_PREFIX}}",
		"{{HEADER_FILE_NAME}}",
		"{{HEADER_DEFINE}}",
		NULL
	};
	
	if((out = fopen(output_file, "w+")) == NULL) {
		g_printerr("Could not open %s for writing\n", output_file);
		return FALSE;
	}
	
	for(st_size = 0; search_table[st_size] != NULL; st_size++); 
	
	if(!g_file_get_contents(input_file, &contents, &length, NULL)) {
		g_printerr("Could not read %s\n", input_file);
		return FALSE;
	}

	for (i = 0; replace_table[i] != NULL; i++) {
		if(i >= st_size)
			break;
			
		working = cstr_replace_all(contents, search_table[i], replace_table[i]);
		g_free(contents);
		contents = working;
	}
	
	if (date_output) {
		gchar *basename = g_path_get_basename(output_file);
		gchar buf[128], year[5];
		time_t curtime = time(NULL);
		struct tm *lt = localtime(&curtime);
		
		strftime(buf, sizeof(buf), "%a %b %e %T %Y", lt);
		strftime(year, sizeof(year), "%Y", lt);
		
		fputs("/***************************************************************************\n", out);
		fprintf(out, "*            %s\n", basename);
		fputs("*\n", out);
		fprintf(out, "*  %s\n", buf);
		fprintf(out, "*  Copyright  %s  %s\n", year, author);
		fprintf(out, "*  %s\n", email);
		fputs("****************************************************************************/\n\n", out);
		
		g_free(basename);
	}
	
	
	switch (license_output) {
		case GPL:
			fputs(GPL_HEADING, out);
			break;
		
		case LGPL:
			fputs(LGPL_HEADING, out);
			break;
		
		case NO_LICENSE: 
		default:
			break;
	}

	fputs(contents, out);
	fclose(out);
	
	g_free(contents);
	
	return TRUE;
}



/*
 *------------------------------------------------------------------------------
 * Generic C++ Class
 *------------------------------------------------------------------------------
 */

/*------------------------------------------------------------------------------
 * returns TRUE: if the main widget can be closed, FALSE: if it should be taken 
 * opened.
 */

gboolean
generic_cpp_class_create_code (ClassGenData *data) {
	AnjutaClassGenPlugin *plugin;	
	const gchar *source_file = FETCH_STRING (data->gxml, "cc_source_file");
	const gchar *header_file = FETCH_STRING (data->gxml, "cc_header_file");
	const gchar *class_name = FETCH_STRING (data->gxml, "cc_class_name");
	gboolean is_inline = FETCH_BOOLEAN (data->gxml, "cc_inline");
	gboolean add_to_project = FETCH_BOOLEAN (data->gxml, "add_to_project_check");
	gboolean add_to_repository = FETCH_BOOLEAN (data->gxml, "add_to_repository_check");
	FILE* header_fd;
	FILE* source_fd;
	gboolean bOK = FALSE;
	IAnjutaProjectManager *pm;
	IAnjutaFileLoader *loader;
	
	plugin = data->plugin;
	
	/* check whether all required fields are filled or not */
	if ( g_str_equal (source_file, "") || g_str_equal (header_file, "") ||
		  g_str_equal (class_name, "")) {
		anjuta_util_dialog_error (NULL, _("Please fill required fields."));
		return FALSE;
	}

	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaFileLoader, NULL);

	g_return_val_if_fail (loader != NULL, FALSE);
	
	if (!is_inline) {	/* not inlined */
		header_fd = fopen (header_file, "at");
		if (header_fd != NULL) {
			gen_cpp_generate_header (data, is_inline, header_fd);
			fflush (header_fd);
			bOK = !ferror (header_fd);
			fclose (header_fd);
			header_fd = NULL;
		}
		
		source_fd = fopen (source_file, "at");
		if (source_fd != NULL) {
			gen_cpp_generate_source (data, is_inline, header_file, source_fd);
			fflush (source_fd);
			bOK = !ferror (source_fd);
			fclose (source_fd);
			source_fd = NULL;
		}
	}
	else 	{				/* inlined */
		header_fd = fopen (header_file, "at");
		if (header_fd != NULL) {
			gen_cpp_generate_header (data, is_inline, header_fd);
			gen_cpp_generate_source (data, is_inline, header_file, header_fd);
			fflush (header_fd);
			bOK = !ferror (header_fd);
			fclose (header_fd);
			header_fd = NULL;
		}
	}
	
	if (bOK) {
		/* add the files to the project */
		if (add_to_project) {
			pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);
			
			g_return_val_if_fail (pm != NULL, FALSE);
			
			if (!is_inline) 
				ianjuta_project_manager_add_source (pm, source_file, source_file, NULL);
			ianjuta_project_manager_add_source (pm, header_file, header_file, NULL);
		}
		
		if (add_to_repository) {
			IAnjutaVcs *vcs;
			vcs = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
										IAnjutaVcs, NULL);

			g_return_val_if_fail (vcs != NULL, FALSE);
			ianjuta_vcs_add (vcs, source_file, NULL);
			ianjuta_vcs_add (vcs, header_file, NULL);
		}
		
		/* let's open the files */
		ianjuta_file_loader_load (loader, source_file, FALSE, NULL);
		ianjuta_file_loader_load (loader, header_file, FALSE, NULL);
	}
	else
		anjuta_util_dialog_error (NULL, _("Error in writing files"));

	return TRUE;
}


static struct tm*
GetNowTime(void)	
{
	time_t l_time;

	l_time = time(NULL);
	return localtime(&l_time);
}


static void
gen_cpp_generate_header (ClassGenData *data, gboolean is_inline, FILE* fpOut) {
	const gchar *base_class = FETCH_STRING (data->gxml, "cc_base_class");	
	const gchar *class_name = FETCH_STRING (data->gxml, "cc_class_name");
	const gchar *author_name = FETCH_STRING (data->gxml, "cc_author_name");
	const gchar *author_email = FETCH_STRING (data->gxml, "cc_author_email");
	gboolean is_virtual_destructor = FETCH_BOOLEAN (data->gxml, "cc_virtual_destructor");
	gboolean date_output = FETCH_BOOLEAN (data->gxml, "cc_date_output");
	
	GtkWidget* license_widget = glade_xml_get_widget (data->gxml, "license_combo");
	gint license_output = gtk_combo_box_get_active (GTK_COMBO_BOX (license_widget));
	
	GtkWidget *combo = glade_xml_get_widget (data->gxml, "cc_inheritance");
	gint access_inheritance_index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
	const gchar *access_inheritance;
	
	gchar* class_name_all_uppers = g_utf8_strup (class_name, strlen (class_name));
	struct tm *t = GetNowTime();
		
	/* obtain the type of inheritance */
	switch (access_inheritance_index) {
		case INHERIT_PUBLIC:
			access_inheritance = "public";
			break;
		
		case INHERIT_PROTECTED:
			access_inheritance = "protected";
			break;
		
		case INHERIT_PRIVATE:
			access_inheritance = "private";
			break;
		
		default:
			access_inheritance = "";
			break;
	}
	
	/* output a C++ header */
	switch (license_output) {
		case GPL:
			fprintf(fpOut, "%s\n", GPL_HEADING);
			break;
		
		case LGPL:
			fprintf(fpOut, "%s\n", LGPL_HEADING);
			break;
		
		case NO_LICENSE: 
		default:
			break;
	}
	
	
	fprintf (fpOut, "//\n// Class: %s\n", class_name);
	if (date_output) {
		fprintf (fpOut, "// Created by: %s <%s>\n", author_name, author_email);
		fprintf (fpOut, "// Created on: %s//\n\n", asctime(t));
	}
		
	fprintf (fpOut,
				"#ifndef _%s_H_\n"
				"#define _%s_H_\n"
				"\n"
				"\n",
				class_name_all_uppers,
				class_name_all_uppers
			  );
		
	if (is_inline) {
		/* output some nice deliniation comments */
		fprintf
		(
			fpOut,
			"//------------------------------------------------------------------------------\n"
			"// %s Declaration\n"
			"//------------------------------------------------------------------------------\n"
			"\n"
			"\n",
			class_name
		);
	}
		
	if (strlen (base_class) > 0)	{
		/* output class with inheritance */
		fprintf
		(
			fpOut, 
			"class %s : %s %s\n"
			"{\n"
			"\tpublic:\n"
			"\t\t%s();\n",
			class_name, 
			access_inheritance,
			base_class,
			class_name
		);
	}
	else {
		/* output class without inheritance */
		fprintf
		(
			fpOut, 
			"class %s\n"
			"{\n"
			"\tpublic:\n"
			"\t\t%s();\n",
			class_name, 
			class_name
		);
	}
		
	if (is_virtual_destructor)
	{
		/* output virtual destructor */
		fprintf
		(
			fpOut,
			"\t\tvirtual ~%s();\n",
			class_name
		);
	}
	else
	{
		/* output non-virtual destructor */
		fprintf
		(
			fpOut,
			"\t\t ~%s();\n",
			class_name
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
		class_name,
		class_name
	);
		
	if(!is_inline)
	{
		fprintf
		(
			fpOut,
			"#endif	//_%s_H_\n"
			"\n",
			class_name_all_uppers
		);
	}
	
	g_free (class_name_all_uppers);
}

static void
gen_cpp_generate_source (ClassGenData *data, gboolean is_inline, const gchar* header_file, FILE* fpOut)
{
	/* GtkWidget *combo; */
	const gchar *base_class = FETCH_STRING (data->gxml, "cc_base_class");	
	const gchar *class_name = FETCH_STRING (data->gxml, "cc_class_name");
	const gchar *author_name = FETCH_STRING (data->gxml, "cc_author_name");
	const gchar *author_email = FETCH_STRING (data->gxml, "cc_author_email");
	/* gboolean is_virtual_destructor = FETCH_BOOLEAN (data->gxml, "cc_virtual_destructor"); */
	gboolean date_output = FETCH_BOOLEAN (data->gxml, "cc_date_output");
	struct tm *t = GetNowTime();	

	GtkWidget* license_widget = glade_xml_get_widget (data->gxml, "license_combo");
	gint license_output = gtk_combo_box_get_active (GTK_COMBO_BOX (license_widget));
	
	gchar* class_name_all_uppers = g_utf8_strup (class_name, strlen (class_name));

	switch (license_output) {
		case GPL:
			fprintf(fpOut, "%s\n", GPL_HEADING);
			break;
		
		case LGPL:
			fprintf(fpOut, "%s\n", LGPL_HEADING);
			break;
		
		case NO_LICENSE: 
		default:
			break;
	}	
	
	if(!is_inline)	{
		fprintf (fpOut, "//\n// Class: %s\n//\n", class_name);
		if (date_output) {
			fprintf (fpOut, "// Created by: %s <%s>\n", author_name, author_email);
			fprintf (fpOut, "// Created on: %s//\n\n", asctime(t) );
		}
		
		fprintf (
				fpOut,
				"#include \"%s\"\n"
				"\n"
				"\n",
				header_file
			);
	}
	else 	{
		fprintf
		(
			fpOut,
			"//------------------------------------------------------------------------------\n"
			"// %s Implementation\n"
			"//------------------------------------------------------------------------------\n"
			"\n\n",
			class_name
		);
	}
	
	if (strlen (base_class) > 0)	{
		/* output constructor with inheritence */
		fprintf
		(
			fpOut,
			"%s::%s() : %s()\n",
			class_name,
			class_name,
			base_class
		);
	}
	else	{
		/* output constructor without inheritence */
		fprintf
		(
			fpOut,
			"%s::%s()\n",
			class_name,
			class_name
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
		class_name, 
		class_name
	);
	
	if (is_inline)	{
		fprintf
		(
			fpOut,
			"#endif	//_%s_H_\n"
			"\n",
			class_name_all_uppers
		);
	}
	g_free (class_name_all_uppers);
}
