/*
 * clsGen.h Copyright (C) 2002  Dave Huseby
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



#define GPL_HEADING "/*\n" \
" *  This program is free software; you can redistribute it and/or modify\n" \
" *  it under the terms of the GNU General Public License as published by\n" \
" *  the Free Software Foundation; either version 2 of the License, or\n" \
" *  (at your option) any later version.\n" \
" *\n" \
" *  This program is distributed in the hope that it will be useful,\n" \
" *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
" *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
" *  GNU Library General Public License for more details.\n" \
" *\n" \
" *  You should have received a copy of the GNU General Public License\n" \
" *  along with this program; if not, write to the Free Software\n" \
" *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n" \
" */\n\n" 

/*
 *------------------------------------------------------------------------------
 * Foward Declarations
 *------------------------------------------------------------------------------
 */


static struct tm* GetNowTime(void);
static void generate_header (ClassGenData* data, gboolean is_inline, FILE* fpOut);
static void generate_source (ClassGenData *data, gboolean is_inline, const gchar* header_file, FILE* fpOut);



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
	gboolean gpl_output = FETCH_BOOLEAN (data->gxml, "go_gpl_output");

	gchar *header_file_base; 
	gchar *header_define;
	gchar *t; 
	AnjutaClassGenPlugin *plugin;
	IAnjutaProjectManager *pm;

	plugin = data->plugin;
	
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);

	classgen_widget = glade_xml_get_widget (data->gxml, "classgen_main");	
	
	/* check whether all required fields are filled or not */
	if ( g_str_equal (base_class, "") || g_str_equal (gtype_name, "") ||
		  g_str_equal (gtype_prefix, "") || g_str_equal (class_function_prefix, "") ||
		  g_str_equal (source_file, "") || g_str_equal (header_file, "") ) {
		anjuta_util_dialog_error (NULL, _("Please check your fields."));
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
					date_output, gpl_output);
					
	b = transform_file(CLASS_TEMPLATE"/"CLASS_GOC_SOURCE_TEMPLATE, 
					source_file, trans_table, author_name, author_email, 
					date_output, gpl_output);

	gtk_widget_hide (classgen_widget);
	
	if(a && b) {
		ianjuta_project_manager_add_source (pm, source_file, source_file, NULL);
		ianjuta_project_manager_add_source (pm, header_file, header_file, NULL);
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
	gboolean date_output, gboolean gpl_output)
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

	for(i = 0; replace_table[i] != NULL; i++) {
		if(i >= st_size)
			break;
			
		working = cstr_replace_all(contents, search_table[i], replace_table[i]);
		g_free(contents);
		contents = working;
	}
	
	if(date_output) {
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
	
	
			   
	if(gpl_output) {
		fputs(GPL_HEADING, out);
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
	FILE* header_fd;
	FILE* source_fd;
	gboolean bOK = FALSE;
	IAnjutaProjectManager *pm;

	plugin = data->plugin;
	
	/* check whether all required fields are filled or not */
	if ( g_str_equal (source_file, "") || g_str_equal (header_file, "") ||
		  g_str_equal (class_name, "")) {
		anjuta_util_dialog_error (NULL, _("Please check your fields."));
		return FALSE;
	}
	
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);
	
	g_return_val_if_fail(pm != NULL, FALSE);
	
	if (!is_inline) {	/* not inlined */
		header_fd = fopen (header_file, "at");
		if (header_fd != NULL) {
			generate_header (data, is_inline, header_fd);
			fflush (header_fd);
			bOK = !ferror (header_fd);
			fclose (header_fd);
			header_fd = NULL;
		}
		
		source_fd = fopen (source_file, "at");
		if (source_fd != NULL) {
			generate_source (data, is_inline, header_file, source_fd);
			fflush (source_fd);
			bOK = !ferror (source_fd);
			fclose (source_fd);
			source_fd = NULL;
		}
	}
	else 	{				/* inlined */
		header_fd = fopen (header_file, "at");
		if (header_fd != NULL) {
			generate_header (data, is_inline, header_fd);
			generate_source (data, is_inline, header_file, header_fd);
			fflush (header_fd);
			bOK = !ferror (header_fd);
			fclose (header_fd);
			header_fd = NULL;
		}
	}
	
	if (bOK) {
		if (!is_inline) 
			ianjuta_project_manager_add_source (pm, source_file, source_file, NULL);
		ianjuta_project_manager_add_source (pm, header_file, header_file, NULL);
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
generate_header (ClassGenData *data, gboolean is_inline, FILE* fpOut) {
	
	GtkWidget *combo;
	const gchar *base_class = FETCH_STRING (data->gxml, "cc_base_class");	
	const gchar *class_name = FETCH_STRING (data->gxml, "cc_class_name");
	const gchar *author_name = FETCH_STRING (data->gxml, "cc_author_name");
	const gchar *author_email = FETCH_STRING (data->gxml, "cc_author_email");
	gboolean is_virtual_destructor = FETCH_BOOLEAN (data->gxml, "cc_virtual_destructor");
	
	combo = glade_xml_get_widget (data->gxml, "cc_inheritance");
	const gchar *access_inheritance = gtk_editable_get_chars (GTK_EDITABLE (combo), 0, -1);
	
	gchar* class_name_all_uppers = g_utf8_strup (class_name, strlen (class_name));
	struct tm *t = GetNowTime();
		
	/* output a C++ header */
	fprintf (fpOut, "//\n// Class: %s\n", class_name);
	fprintf (fpOut, "// Created by: %s <%s>\n", author_name, author_email);
	fprintf (fpOut, "// Created on: %s//\n\n", asctime(t));
		
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
generate_source (ClassGenData *data, gboolean is_inline, const gchar* header_file, FILE* fpOut)
{
	GtkWidget *combo;
	const gchar *base_class = FETCH_STRING (data->gxml, "cc_base_class");	
	const gchar *class_name = FETCH_STRING (data->gxml, "cc_class_name");
	const gchar *author_name = FETCH_STRING (data->gxml, "cc_author_name");
	const gchar *author_email = FETCH_STRING (data->gxml, "cc_author_email");
	gboolean is_virtual_destructor = FETCH_BOOLEAN (data->gxml, "cc_virtual_destructor");
	struct tm *t = GetNowTime();	
	
	gchar* class_name_all_uppers = g_utf8_strup (class_name, strlen (class_name));

	if(!is_inline)	{
		fprintf (fpOut, "//\n// Class: %s\n", class_name);
		fprintf (fpOut, "// Created by: %s <%s>\n", author_name, author_email);
		fprintf (fpOut, "// Created on: %s//\n\n", asctime(t) );
		
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
