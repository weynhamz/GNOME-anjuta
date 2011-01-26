/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>
#include <ctype.h>
#include <stdlib.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-pkg-config-chooser.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-glade-signal.h>
#include <libanjuta/interfaces/ianjuta-editor-tip.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "plugin.h"
#include "cpp-java-utils.h"
#include "cpp-java-indentation.h"
#include "cpp-packages.h"

/* Pixmaps */
#define ANJUTA_PIXMAP_SWAP                "anjuta-swap"
#define ANJUTA_PIXMAP_AUTOINDENT          "anjuta-indent-auto"
#define ANJUTA_STOCK_SWAP                 "anjuta-swap"
#define ANJUTA_STOCK_COMPLETE         	  "anjuta-complete"
#define ANJUTA_STOCK_AUTOINDENT           "anjuta-indent"
#define ANJUTA_STOCK_COMMENT              "anjuta-comment"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-language-support-cpp-java.xml"
#define PREFS_BUILDER PACKAGE_DATA_DIR"/glade/anjuta-language-cpp-java.ui"
#define ICON_FILE "anjuta-language-cpp-java-plugin.png"

#define FIXME_DEFAULT_PACKAGE_VERSION		"1.0"

/* Preferences keys */

#define PREF_SCHEMA "org.gnome.anjuta.cpp"
#define PREF_INDENT_AUTOMATIC "cpp-indent-automatic"
#define PREF_INDENT_MODELINE "cpp-indent-modeline"
#define PREF_PROJECT_PACKAGES "cpp-load-project-packages"

static gpointer parent_class;

static void
set_indentation_param_emacs (CppJavaPlugin* plugin, const gchar *param,
					   const gchar *value)
{
	//DEBUG_PRINT ("Setting indent param: %s = %s", param, value);
	if (strcasecmp (param, "indent-tabs-mode") == 0)
	{
		if (strcasecmp (value, "t") == 0)
		{
			plugin->param_use_spaces = 0;
			ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
										   FALSE, NULL);
		}
		else if (strcasecmp (value, "nil") == 0)
		{
			plugin->param_use_spaces = 1;
			ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
										   TRUE, NULL);
		}
	}
	else if (strcasecmp (param, "c-basic-offset") == 0)
	{
		plugin->param_statement_indentation = atoi (value);
	}
	else if (strcasecmp (param, "tab-width") == 0)
	{
		plugin->param_tab_size = atoi (value);
		ianjuta_editor_set_tabsize (IANJUTA_EDITOR (plugin->current_editor),
									plugin->param_tab_size, NULL);
	}
}

static void
set_indentation_param_vim (CppJavaPlugin* plugin, const gchar *param,
					   const gchar *value)
{
	//DEBUG_PRINT ("Setting indent param: %s = %s", param, value);
	if (g_str_equal (param, "expandtab") ||
		g_str_equal (param, "et"))
	{
			plugin->param_use_spaces = 1;
			ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
										   TRUE, NULL);
	}
	else if (g_str_equal (param, "noexpandtabs") ||
			 g_str_equal (param, "noet"))
	{
	  	plugin->param_use_spaces = 0;
			ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
										   FALSE, NULL);
	}
	if (!value)
		return;
	else if (g_str_equal (param, "shiftwidth") ||
			 g_str_equal (param, "sw"))
	{
		plugin->param_statement_indentation = atoi (value);
	}
	else if (g_str_equal (param, "softtabstop") ||
			 g_str_equal (param, "sts") ||
			 g_str_equal (param, "tabstop") ||
			 g_str_equal (param, "ts"))
	{
		plugin->param_tab_size = atoi (value);
		ianjuta_editor_set_tabsize (IANJUTA_EDITOR (plugin->current_editor),
									plugin->param_tab_size, NULL);
	}
}

static void
parse_mode_line_emacs (CppJavaPlugin *plugin, const gchar *modeline)
{
	gchar **strv, **ptr;
	
	strv = g_strsplit (modeline, ";", -1);
	ptr = strv;
	while (*ptr)
	{
		gchar **keyval;
		keyval = g_strsplit (*ptr, ":", 2);
		if (keyval[0] && keyval[1])
		{
			g_strstrip (keyval[0]);
			g_strstrip (keyval[1]);
			set_indentation_param_emacs (plugin, g_strchug (keyval[0]),
                                   g_strchug (keyval[1]));
		}
		g_strfreev (keyval);
		ptr++;
	}
	g_strfreev (strv);
}

static void
parse_mode_line_vim (CppJavaPlugin *plugin, const gchar *modeline)
{
	gchar **strv, **ptr;
	
	strv = g_strsplit_set (modeline, " \t:", -1);
	ptr = strv;
	while (*ptr)
	{
		gchar **keyval;
		keyval = g_strsplit (*ptr, "=", 2);
		if (keyval[0])
		{
			g_strstrip (keyval[0]);
			if (keyval[1])
			{
				g_strstrip (keyval[1]);
				set_indentation_param_vim (plugin, g_strchug (keyval[0]),
				                           g_strchug (keyval[1]));
			}
			else
				set_indentation_param_vim (plugin, g_strchug (keyval[0]),
				                           NULL);        
		}
		g_strfreev (keyval);
		ptr++;
	}
	g_strfreev (strv);
}

static gchar *
extract_mode_line (const gchar *comment_text, gboolean* vim)
{
	/* Search for emacs-like modelines */
	gchar *begin_modeline, *end_modeline;
	begin_modeline = strstr (comment_text, "-*-");
	if (begin_modeline)
	{
		begin_modeline += 3;
		end_modeline = strstr (begin_modeline, "-*-");
		if (end_modeline)
		{
		  *vim = FALSE;
				return g_strndup (begin_modeline, end_modeline - begin_modeline);
		}
	}
	/* Search for vim-like modelines */
	begin_modeline = strstr (comment_text, "vim:");
	if (begin_modeline)
	{
		begin_modeline += strlen ("vim:");
		end_modeline = strstr (begin_modeline, "*/");
		/* Check for escape characters */
		while (end_modeline)
		{
			 if (!g_str_equal ((end_modeline - 1), "\\"))
				break;
			end_modeline++;
			end_modeline = strstr (end_modeline, "*/");
		}
		if (end_modeline)
		{
			gchar* vim_modeline = g_strndup (begin_modeline, end_modeline - begin_modeline);
			*vim = TRUE;
			return vim_modeline;
		}
	}
	return NULL;
}

#define MINI_BUFFER_SIZE 3
static void
initialize_indentation_params (CppJavaPlugin *plugin)
{
	IAnjutaIterable *iter;
	GString *comment_text;
	gboolean comment_begun = FALSE;
	gboolean line_comment = FALSE;
	gchar mini_buffer[MINI_BUFFER_SIZE] = {0};
	
	plugin->smart_indentation = g_settings_get_boolean (plugin->settings, PREF_INDENT_AUTOMATIC);
	/* Disable editor intern auto-indent if smart indentation is enabled */
	ianjuta_editor_set_auto_indent (IANJUTA_EDITOR(plugin->current_editor),
								    !plugin->smart_indentation, NULL);

	/* Initialize indentation parameters */
	plugin->param_tab_size = -1;
	plugin->param_statement_indentation = -1;
	plugin->param_brace_indentation = -1;
	plugin->param_case_indentation = -1;
	plugin->param_label_indentation = -1;
	plugin->param_use_spaces = -1;

	if (g_settings_get_boolean (plugin->settings,
	                            PREF_INDENT_MODELINE))
	{
		/* Find the first comment text in the buffer */
		comment_text = g_string_new (NULL);
		iter = ianjuta_editor_get_start_position (IANJUTA_EDITOR (plugin->current_editor),
		                                          NULL);
		do
		{
			gboolean shift_buffer = TRUE;
			gint i;
			gchar ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
			                                         0, NULL);

			for (i = 0; i < MINI_BUFFER_SIZE - 1; i++)
			{
				if (mini_buffer[i] == '\0')
				{
					mini_buffer[i] = ch;
					shift_buffer = FALSE;
					break;
				}
			}
			if (shift_buffer == TRUE)
			{
				/* Shift buffer and add */
				for (i = 0; i < MINI_BUFFER_SIZE - 1; i++)
					mini_buffer [i] = mini_buffer[i+1];
				mini_buffer[i] = ch;
			}

			if (!comment_begun && strncmp (mini_buffer, "/*", 2) == 0)
			{
				comment_begun = TRUE;
				/* Reset buffer */
				mini_buffer[0] = mini_buffer[1] = '\0';
			}
			else if (!comment_begun && strncmp (mini_buffer, "//", 2) == 0)
			                                    {
													comment_begun = TRUE;
													line_comment = TRUE;
												}
			                                    else if (!comment_begun && mini_buffer[1] != '\0')
			                                    {
													/* The buffer doesn't begin with a comment */
													break;
												}
			                                    else if (comment_begun)
			                                    {
													if ((line_comment && ch == '\n') ||
													    (!line_comment && strncmp (mini_buffer, "*/", 2) == 0))
													{
														break;
													}
												}

			                                    if (comment_begun)
			                                    g_string_append_c (comment_text, ch);

		                                    }
		while (ianjuta_iterable_next (iter, NULL));

		/* DEBUG_PRINT ("Comment text: %s", comment_text->str);*/
		if (comment_text->len > 0)
		{

			/* First comment found */
			gboolean vim;
			gchar *modeline = extract_mode_line (comment_text->str, &vim);
			if (modeline)
			{
				if (!vim)
					parse_mode_line_emacs (plugin, modeline);
				else
					parse_mode_line_vim (plugin, modeline);
				g_free (modeline);
			}
		}
		g_string_free (comment_text, TRUE);
		g_object_unref (iter);
	}
}

/* Glade support */

static void
init_file_type (CppJavaPlugin* lang_plugin)
{
	GFile* file = ianjuta_file_get_file (IANJUTA_FILE (lang_plugin->current_editor),
	                                     NULL);

	if (file)
	{
		gchar* mime_type = anjuta_util_get_file_mime_type (file);
		if (mime_type)
		{
			if (g_str_equal (mime_type, "text/x-csrc"))
				lang_plugin->filetype = LS_FILE_C;
			else if (g_str_equal (mime_type, "text/x-chdr"))
				lang_plugin->filetype = LS_FILE_CHDR;
			else if (g_str_equal (mime_type, "text/x-c++src"))
				lang_plugin->filetype = LS_FILE_CPP;
			else if (g_str_equal (mime_type, "text/x-c++hdr"))
				lang_plugin->filetype = LS_FILE_CPPHDR;
			else
				lang_plugin->filetype = LS_FILE_OTHER;
			return;
		}
	}
	lang_plugin->filetype = LS_FILE_OTHER;
}

static gboolean
on_glade_drop_possible (IAnjutaEditor* editor,
                        IAnjutaIterable* iterator,
                        CppJavaPlugin* lang_plugin)
{
	switch (lang_plugin->filetype)
	{
		case LS_FILE_C:
		case LS_FILE_CHDR:
			return TRUE;
		default:
			return FALSE;
	}
}

static gchar*
language_support_check_param_name (const gchar* name,
                                   GList** names)
{
	gint index = 0;
	GString* real_name = g_string_new (name);
	while (g_list_find_custom (*names, real_name->str, (GCompareFunc) strcmp))
	{
		g_string_free (real_name, TRUE);
		real_name = g_string_new (name);
		g_string_append_printf (real_name, "%d", ++index);
	}
	*names = g_list_append (*names, real_name->str);
	return g_string_free (real_name, FALSE);
}

static const gchar*
language_support_get_signal_parameter (const gchar* type_name, GList** names)
{
	const gchar* c;
	const gchar* param_name = NULL;
	GString* param_string;
	gchar* real_name;
	/* Search for the second upper character */
	for (c = type_name + 1; *c != '\0'; c++)
	{
		if (g_ascii_isupper (*c))
		{
			param_name = c;
			break;
		}
	}
	if (param_name && strlen (param_name))
	{
		param_string = g_string_new (param_name);
		g_string_down (param_string);
	}
	else
	{
		param_string = g_string_new ("arg");
	}
	real_name = language_support_check_param_name (g_string_free (param_string, FALSE), names);

	return real_name;
}

static void
on_glade_drop (IAnjutaEditor* editor,
               IAnjutaIterable* iterator,
               const gchar* signal_data,
               CppJavaPlugin* lang_plugin)
{
	GSignalQuery query;
	GType type;
	guint id;
	
	const gchar* widget;
	const gchar* signal;
	const gchar* handler;
	const gchar* user_data;
	gboolean swapped;

	GStrv data = g_strsplit(signal_data, ":", 5);
	
	widget = data[0];
	signal = data[1];
	handler = data[2];
	user_data = data[3];
	swapped = g_str_equal (data[4], "1");
	
	type = g_type_from_name (widget);
	id = g_signal_lookup (signal, type);

	g_signal_query (id, &query);
	
	switch (lang_plugin->filetype)
	{
		case LS_FILE_C:
		{
			GList* names = NULL;
			GString* str = g_string_new ("\nstatic ");
			const gchar* widget_param = language_support_get_signal_parameter (widget,
			                                                                   &names);
			int i;
			g_string_append (str, g_type_name (query.return_type));
			if (!swapped)
				g_string_append_printf (str, "\n%s (%s *%s", handler, widget, widget_param);
			else
				g_string_append_printf (str, "\n%s (gpointer user_data, %s *%s", handler, widget, widget_param);				
			for (i = 0; i < query.n_params; i++)
			{
				const gchar* type_name = g_type_name (query.param_types[i]);
				const gchar* param_name = language_support_get_signal_parameter (type_name,
				                                                                 &names);
				
				if (query.param_types[i] <= G_TYPE_DOUBLE)
				{	                                                                
					g_string_append_printf (str, ", %s %s", type_name, param_name);
				}
				else
				{	                                                                
					g_string_append_printf (str, ", %s *%s", type_name, param_name);
				}
			}
			if (!swapped)
				g_string_append (str, ", gpointer user_data)");
			else
				g_string_append (str, ")");

			g_string_append (str, "\n{\n\n}\n");

			ianjuta_editor_insert (editor, iterator,
			                       str->str, -1, NULL);
			g_string_free (str, TRUE);
			anjuta_util_glist_strings_free (names);
			break;
		}
		case LS_FILE_CHDR:
		default:
			break;
	}
	g_strfreev (data);
}

/* Enable/Disable language-support */

static void
install_support (CppJavaPlugin *lang_plugin)
{	
	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell,
									IAnjutaLanguage, NULL);
	
	if (!lang_manager)
		return;
	
	if (lang_plugin->support_installed)
		return;
	
	lang_plugin->current_language = 
		ianjuta_language_get_name_from_editor (lang_manager, 
											   IANJUTA_EDITOR_LANGUAGE (lang_plugin->current_editor), NULL);
	
	DEBUG_PRINT("Language support installed for: %s",
				lang_plugin->current_language);
	
	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "C")
		|| g_str_equal (lang_plugin->current_language, "C++")
		|| g_str_equal (lang_plugin->current_language, "Vala")))
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (cpp_indentation),
						  lang_plugin);
	}
	else if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Java")))
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (java_indentation),
						  lang_plugin);
	}
	else
	{
		return;
	}
	
	initialize_indentation_params (lang_plugin);
	init_file_type (lang_plugin);

	
	if (!g_str_equal (lang_plugin->current_language, "Vala"))
	{
		CppJavaAssist *assist;

		g_assert (lang_plugin->assist == NULL);

		assist = cpp_java_assist_new (IANJUTA_EDITOR (lang_plugin->current_editor),
					anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell,
												IAnjutaSymbolManager,
												NULL),
					lang_plugin->settings);
		lang_plugin->assist = assist;


		if (IANJUTA_IS_EDITOR_GLADE_SIGNAL (lang_plugin->current_editor))
		{
			g_signal_connect (lang_plugin->current_editor,
			                  "drop-possible", G_CALLBACK (on_glade_drop_possible),
			                  lang_plugin);
			g_signal_connect (lang_plugin->current_editor,
			                  "drop", G_CALLBACK (on_glade_drop),
			                  lang_plugin);
		}
	}	
		
	lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (CppJavaPlugin *lang_plugin)
{
	if (!lang_plugin->support_installed)
		return;
	
	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "C")
		|| g_str_equal (lang_plugin->current_language, "C++")
		|| g_str_equal (lang_plugin->current_language, "Vala")))
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (cpp_indentation),
									lang_plugin);
	}
	else if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Java")))
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (java_indentation),
									lang_plugin);
	}
	
	if (lang_plugin->assist)
	{	
		g_object_unref (lang_plugin->assist);
		lang_plugin->assist = NULL;
	}

	g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
	                                      on_glade_drop_possible, lang_plugin);
	g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
	                                      on_glade_drop, lang_plugin);
	
	
	lang_plugin->support_installed = FALSE;
}

static void
on_editor_language_changed (IAnjutaEditor *editor,
							const gchar *new_language,
							CppJavaPlugin *plugin)
{
	uninstall_support (plugin);
	install_support (plugin);
}

static void
on_value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
							   const GValue *value, gpointer data)
{
	CppJavaPlugin *lang_plugin;
	IAnjutaDocument* doc = IANJUTA_DOCUMENT(g_value_get_object (value));
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
	if (IANJUTA_IS_EDITOR(doc))
		lang_plugin->current_editor = G_OBJECT(doc);
	else
	{
		lang_plugin->current_editor = NULL;
		return;
	}
	if (IANJUTA_IS_EDITOR(lang_plugin->current_editor))
		install_support (lang_plugin);
	g_signal_connect (lang_plugin->current_editor, "language-changed",
					  G_CALLBACK (on_editor_language_changed),
					  plugin);
}

static void
on_value_removed_current_editor (AnjutaPlugin *plugin, const gchar *name,
								 gpointer data)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
	if (lang_plugin->current_editor)
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
										  G_CALLBACK (on_editor_language_changed),
										  plugin);
	if (IANJUTA_IS_EDITOR(lang_plugin->current_editor))
		uninstall_support (lang_plugin);
	lang_plugin->current_editor = NULL;
}

const gchar* SOURCE_EXT[] =
{
	".c",
	".cc",
	".C",
	".cpp",
	".cxx",
	".ccg",
	NULL
};

const gchar* HEADER_EXT[] =
{
	".h",
	".hh",
	".H",
	".hpp",
	".hxx",
	".hg",
	NULL
};

static void
on_swap_activate (GtkAction* action, gpointer data)
{
	GFile* file;
	GFile* parent;
	gchar* parent_uri;
	gchar* basename;
	gchar* ext;
	CppJavaPlugin *lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (data);
	IAnjutaDocumentManager* docman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(lang_plugin)->shell,
									IAnjutaDocumentManager,
									NULL);
	if (!lang_plugin->current_editor || !docman)
		return;
	
	file = ianjuta_file_get_file (IANJUTA_FILE (lang_plugin->current_editor),
								  NULL);
	parent = g_file_get_parent (file);
	parent_uri = g_file_get_uri (parent);
	basename = g_file_get_basename (file);
	g_object_unref (file);
	g_object_unref (parent);
	ext = strstr (basename, ".");
	if (ext)
	{
		int i;
		for (i = 0; SOURCE_EXT[i] != NULL; i++)
		{
			if (g_str_equal (ext, SOURCE_EXT[i]))
			{
				int j;
				for (j = 0; HEADER_EXT[j] != NULL; j++)
				{
					gchar* filename;
					gchar* uri;
					GFile* new_file;
					*ext = '\0';
					filename = g_strdup_printf ("%s%s", basename, HEADER_EXT[j]);
					uri = g_build_filename (parent_uri, filename, NULL);
					new_file = g_file_new_for_uri (uri);
					g_free (uri);
					g_free(filename);
					if (g_file_query_exists (new_file, NULL))
					{
						ianjuta_document_manager_goto_file_line (docman,
																 new_file,
																 -1,
																 NULL);
						g_object_unref (new_file);
						break;
					}
					g_object_unref (new_file);
				}
				break;
			}
			if (g_str_equal (ext, HEADER_EXT[i]))
			{
				int j;
				for (j = 0; SOURCE_EXT[j] != NULL; j++)
				{
					gchar* filename;
					gchar* uri;
					GFile* new_file;
					*ext = '\0';
					filename = g_strdup_printf ("%s%s", basename, SOURCE_EXT[j]);
					uri = g_build_filename (parent_uri, filename, NULL);
					new_file = g_file_new_for_uri (uri);
					g_free (uri);
					g_free(filename);
					if (g_file_query_exists (new_file, NULL))
					{
						ianjuta_document_manager_goto_file_line (docman,
																 new_file,
																 -1,
																 NULL);
						g_object_unref (new_file);
						break;
					}
					g_object_unref (new_file);
				}
				break;
			}
		}
	}
	g_free(basename);
	g_free (parent_uri);
}

static void
on_auto_indent (GtkAction *action, gpointer data)
{	
	CppJavaPlugin *lang_plugin;
	IAnjutaEditor *editor;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (data);
	editor = IANJUTA_EDITOR (lang_plugin->current_editor);

	cpp_auto_indentation (editor, lang_plugin);
}

/* Automatic comments */

static gboolean
is_commented_multiline (IAnjutaEditor *editor,
						IAnjutaIterable *start,
						IAnjutaIterable *end)
{
	gchar *text;
	gboolean is_commented = TRUE;

	text = ianjuta_editor_get_text (editor, start, end, NULL);
	while (is_commented && !g_str_has_prefix (text, "/*"))
	{
		if (!ianjuta_iterable_previous (start, NULL))
			is_commented = FALSE;
		g_free (text);
		text = ianjuta_editor_get_text (editor, start, end, NULL);
		if (g_str_has_prefix (text, "*/"))
			is_commented = FALSE;
	}
	while (is_commented && !g_str_has_suffix (text, "*/"))
	{
		if (!ianjuta_iterable_next (end, NULL))
			is_commented = FALSE;
		g_free (text);
		text = ianjuta_editor_get_text (editor, start, end, NULL);
		if (g_str_has_suffix (text, "/*"))
			is_commented = FALSE;
	}

	g_free (text);
	return is_commented;
}

static void
toggle_comment_multiline (IAnjutaEditor *editor,
					      IAnjutaIterable *start,
					      IAnjutaIterable *end)
{
	IAnjutaIterable *start_copy, *end_copy;
	gchar *text;
	gboolean is_commented;

	start_copy = ianjuta_iterable_clone (start, NULL);
	end_copy = ianjuta_iterable_clone (end, NULL);
	is_commented = is_commented_multiline (editor, start_copy, end_copy);
	text = ianjuta_editor_get_text (editor, start_copy, end_copy, NULL);	

	if (is_commented)
	{
		ianjuta_editor_erase (editor, start_copy, end_copy, NULL);
		ianjuta_editor_insert (editor, start_copy, text + 2,
							   (strlen (text) - 4), NULL);
	}
	else
	{
		ianjuta_editor_insert (editor, end, "*/", -1, NULL);
		ianjuta_editor_insert (editor, start, "/*", -1, NULL);
	}

	g_object_unref (start_copy);
	g_object_unref (end_copy);
	g_free (text);
}

static void
toggle_comment_singleline (CppJavaPlugin *plugin, IAnjutaEditor *editor,
						   gint line)
{
	IAnjutaIterable *begin, *end, *begin_copy, *end_copy;
	gchar *text, *text_stripped, **text_diff = NULL;

	begin = ianjuta_editor_get_line_begin_position (editor, line, NULL);
	end = ianjuta_editor_get_line_end_position (editor, line, NULL);
	begin_copy = ianjuta_iterable_clone (begin, NULL);
	end_copy = ianjuta_iterable_clone (end, NULL);

	if (is_commented_multiline (editor, begin_copy, end_copy))
	{
		toggle_comment_multiline (editor, begin_copy, end_copy);
		g_object_unref (begin);
		g_object_unref (end);
		g_object_unref (begin_copy);
		g_object_unref (end_copy);
		return;
	}
	g_object_unref (begin_copy);
	g_object_unref (end_copy);

	text = ianjuta_editor_get_text (editor, begin, end, NULL);
	text_stripped = g_strstrip (g_strdup (text));
	text_diff = g_strsplit (text, text_stripped, 2);

	if (plugin->current_language &&
		(g_str_equal (plugin->current_language, "C")))
	{
		if (g_str_has_prefix (text_stripped, "/*") &&
			g_str_has_suffix (text_stripped, "*/"))
		{
			ianjuta_editor_erase (editor, begin, end, NULL);
			ianjuta_editor_insert (editor, begin, text_stripped + 2,
								   (strlen (text_stripped) - 4), NULL);
			if (text_diff != NULL)
				ianjuta_editor_insert (editor, begin, *text_diff, -1, NULL);
		}
		else
		{
			ianjuta_editor_insert (editor, end, "*/", -1, NULL);
			ianjuta_editor_insert (editor, begin, "/*", -1, NULL);
		}
	}
	else
	{
		if (g_str_has_prefix (text_stripped, "//"))
		{
			ianjuta_editor_erase (editor, begin, end, NULL);
			ianjuta_editor_insert (editor, begin, text_stripped + 2, -1, NULL);
			if (text_diff != NULL)
				ianjuta_editor_insert (editor, begin, *text_diff, -1, NULL);
		}
		else
		{
			ianjuta_editor_insert (editor, begin, "//", -1, NULL);
		}
	}

	g_object_unref (begin);
	g_object_unref (end);
	g_free (text);
	g_free (text_stripped);
	g_strfreev (text_diff);
}


static void
on_toggle_comment (GtkAction *action, gpointer data)
{
	gint line;
	gboolean has_selection;
	
	CppJavaPlugin *lang_plugin;
	IAnjutaEditor *editor;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (data);
	editor = IANJUTA_EDITOR (lang_plugin->current_editor);
	
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);
	
	has_selection = ianjuta_editor_selection_has_selection
						(IANJUTA_EDITOR_SELECTION (editor), NULL);
	if (has_selection)
	{
		IAnjutaIterable *sel_start, *sel_end;
		sel_start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor),
														NULL);
		sel_end = ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (editor),
													NULL);
		toggle_comment_multiline (editor, sel_start, sel_end);
		g_object_unref (sel_start);
		g_object_unref (sel_end);
	}
	else
	{
		line = ianjuta_editor_get_lineno (IANJUTA_EDITOR(editor), NULL);
		toggle_comment_singleline (lang_plugin, editor, line);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
}

/* Plugin */

static GtkActionEntry actions[] = {
	{
		"ActionMenuEdit",
		NULL, N_("_Edit"),
		NULL, NULL, NULL
	},
	{
		"ActionEditAutoindent",
		ANJUTA_STOCK_AUTOINDENT,
		N_("Auto-Indent"), "<control>i",
		N_("Auto-indent current line or selection based on indentation settings"),
		G_CALLBACK (on_auto_indent)
	},
	{
		"ActionEditToggleComment",
		ANJUTA_STOCK_COMMENT,
		N_("Comment/Uncomment"), "<control>m",
		N_("Comment or uncomment current selection"),
		G_CALLBACK (on_toggle_comment)
	},
	{   "ActionFileSwap", 
		ANJUTA_STOCK_SWAP, 
		N_("Swap .h/.c"), NULL,
		N_("Swap C header and source files"),
		G_CALLBACK (on_swap_activate)
	}
};

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_SWAP, ANJUTA_STOCK_SWAP);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_AUTOINDENT, ANJUTA_STOCK_AUTOINDENT);
	END_REGISTER_ICON;
}

static gboolean
cpp_java_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	CppJavaPlugin *lang_plugin;
	static gboolean initialized = FALSE;
	
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
	
	DEBUG_PRINT ("%s", "AnjutaLanguageCppJavaPlugin: Activating plugin ...");

	if (!initialized)
	{
		register_stock_icons (plugin);
	}

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	lang_plugin->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupCppJavaAssist",
											_("C++/Java Assistance"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	lang_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	lang_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								  IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 on_value_added_current_editor,
								 on_value_removed_current_editor,
								 plugin);

	cpp_packages_load (lang_plugin);
	
	initialized = FALSE;
	return TRUE;
}

static gboolean
cpp_java_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	CppJavaPlugin *lang_plugin;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
	
	anjuta_plugin_remove_watch (plugin,
								lang_plugin->editor_watch_id,
								TRUE);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, lang_plugin->uiid);
	anjuta_ui_remove_action_group (ui, lang_plugin->action_group);
	
	lang_plugin->action_group = NULL;
	lang_plugin->uiid = 0;
	DEBUG_PRINT ("%s", "AnjutaLanguageCppJavaPlugin: Deactivated plugin.");
	return TRUE;
}

static void
cpp_java_plugin_finalize (GObject *obj)
{
	/* CppJavaPlugin* plugin = ANJUTA_PLUGIN_CPP_JAVA (obj); */

	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
cpp_java_plugin_dispose (GObject *obj)
{
	CppJavaPlugin* plugin = ANJUTA_PLUGIN_CPP_JAVA (obj);
	/* Disposition codes */

	g_object_unref (plugin->settings);
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
cpp_java_plugin_instance_init (GObject *obj)
{
	CppJavaPlugin *plugin = ANJUTA_PLUGIN_CPP_JAVA (obj);
	plugin->action_group = NULL;
	plugin->current_editor = NULL;
	plugin->current_language = NULL;
	plugin->editor_watch_id = 0;
	plugin->uiid = 0;
	plugin->assist = NULL;
	plugin->settings = g_settings_new (PREF_SCHEMA);
}

static void
cpp_java_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = cpp_java_plugin_activate_plugin;
	plugin_class->deactivate = cpp_java_plugin_deactivate_plugin;
	klass->finalize = cpp_java_plugin_finalize;
	klass->dispose = cpp_java_plugin_dispose;
}

#define PREF_WIDGET_SPACE "preferences_toggle:bool:1:1:cpp-completion-space-after-func"
#define PREF_WIDGET_BRACE "preferences_toggle:bool:1:1:cpp-completion-brace-after-func"
#define PREF_WIDGET_AUTO "preferences_toggle:bool:1:1:cpp-completion-enable"
#define PREF_WIDGET_PACKAGES "preferences_toggle:bool:1:1:cpp-load-project-packages"
#define PREF_WIDGET_PKG_CONFIG "pkg_config_chooser1"

static void
on_autocompletion_toggled (GtkToggleButton* button,
                           GtkBuilder* bxml)
{
	GtkWidget* widget;
	gboolean sensitive = gtk_toggle_button_get_active (button);

	widget = GTK_WIDGET (gtk_builder_get_object (bxml, PREF_WIDGET_SPACE));
	gtk_widget_set_sensitive (widget, sensitive);
	widget = GTK_WIDGET (gtk_builder_get_object (bxml, PREF_WIDGET_BRACE));
	gtk_widget_set_sensitive (widget, sensitive);
}

static void
on_project_packages_toggled (GtkToggleButton* button,
                             GtkBuilder* bxml)
{
	GtkWidget* pkg_config;
	gboolean sensitive = !gtk_toggle_button_get_active (button);
	pkg_config = GTK_WIDGET (gtk_builder_get_object (bxml, PREF_WIDGET_PKG_CONFIG));

	gtk_widget_set_sensitive (pkg_config, sensitive);
	anjuta_pkg_config_chooser_show_active_only (ANJUTA_PKG_CONFIG_CHOOSER (pkg_config),
	                                            !sensitive);
}

static void
on_package_activated (AnjutaPkgConfigChooser *self, const gchar* package,
    				  gpointer data)
{
	CppJavaPlugin* plugin;
	IAnjutaSymbolManager *isymbol_manager;

	plugin = ANJUTA_PLUGIN_CPP_JAVA (data);

	DEBUG_PRINT ("activated %s", package);
	isymbol_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												IAnjutaSymbolManager,
												NULL);
	
	ianjuta_symbol_manager_activate_package (isymbol_manager, 
	    									 package, 
    							  			 FIXME_DEFAULT_PACKAGE_VERSION,
    							  			 NULL);
	
}

static void
on_package_deactivated (AnjutaPkgConfigChooser *self, const gchar* package,
    					gpointer data)
{
	CppJavaPlugin* plugin;
	IAnjutaSymbolManager *isymbol_manager;

	plugin = ANJUTA_PLUGIN_CPP_JAVA (data);

	DEBUG_PRINT ("deactivated %s", package);

	isymbol_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												IAnjutaSymbolManager,
												NULL);
	ianjuta_symbol_manager_deactivate_package (isymbol_manager, 
	    									 package, 
    							  			 FIXME_DEFAULT_PACKAGE_VERSION,
    							  			 NULL);
}

static void
ipreferences_merge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					GError** e)
{
	GError* error = NULL;
	CppJavaPlugin* plugin = ANJUTA_PLUGIN_CPP_JAVA (ipref);
	plugin->bxml = gtk_builder_new ();
	GtkWidget* toggle;
	GtkWidget* pkg_config;
		
	/* Add preferences */
	if (!gtk_builder_add_from_file (plugin->bxml, PREFS_BUILDER, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	anjuta_preferences_add_from_builder (prefs,
	                                     plugin->bxml, plugin->settings,
	                                     "preferences", _("C/C++/Java/Vala"),
	                                     ICON_FILE);
	toggle = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_AUTO));
	g_signal_connect (toggle, "toggled", G_CALLBACK (on_autocompletion_toggled),
	                  plugin->bxml);
	on_autocompletion_toggled (GTK_TOGGLE_BUTTON (toggle), plugin->bxml);

	toggle = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_PACKAGES));
	g_signal_connect (toggle, "toggled", G_CALLBACK (on_project_packages_toggled),
	                  plugin->bxml);
	on_autocompletion_toggled (GTK_TOGGLE_BUTTON (toggle), plugin->bxml);
	on_project_packages_toggled (GTK_TOGGLE_BUTTON (toggle), plugin->bxml);
	
	pkg_config = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_PKG_CONFIG));
	anjuta_pkg_config_chooser_show_active_column (ANJUTA_PKG_CONFIG_CHOOSER (pkg_config), 
	    										  TRUE);
	g_signal_connect (G_OBJECT (pkg_config), "package-activated",
					  G_CALLBACK (on_package_activated), plugin);

	g_signal_connect (G_OBJECT (pkg_config), "package-deactivated",
					  G_CALLBACK (on_package_deactivated), plugin);
	
	gtk_widget_show_all (pkg_config);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					  GError** e)
{
	CppJavaPlugin* plugin = ANJUTA_PLUGIN_CPP_JAVA (ipref);
	anjuta_preferences_remove_page(prefs, _("C/C++/Java/Vala"));
	g_object_unref (plugin->bxml);
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (CppJavaPlugin, cpp_java_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (CppJavaPlugin, cpp_java_plugin);
