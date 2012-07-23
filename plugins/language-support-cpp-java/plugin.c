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
#include <libanjuta/anjuta-pkg-config.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-glade-signal.h>
#include <libanjuta/interfaces/ianjuta-editor-tip.h>
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "plugin.h"
#include "cpp-packages.h"


/* Pixmaps */
#define ANJUTA_PIXMAP_SWAP                "anjuta-swap"
#define ANJUTA_PIXMAP_AUTOINDENT          "anjuta-indent-auto"
#define ANJUTA_STOCK_SWAP                 "anjuta-swap"
#define ANJUTA_STOCK_AUTOINDENT           "anjuta-indent"
#define ANJUTA_STOCK_COMMENT              "anjuta-comment"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-language-support-cpp-java.xml"
#define PREFS_BUILDER PACKAGE_DATA_DIR"/glade/anjuta-language-cpp-java.ui"
#define ICON_FILE "anjuta-language-cpp-java-plugin.png"

/* Preferences keys */
#define ANJUTA_PREF_SCHEMA_PREFIX "org.gnome.anjuta."
#define PREF_SCHEMA "org.gnome.anjuta.plugins.cpp"
#define PREF_USER_PACKAGES "user-packages"
#define PREF_PROJECT_PACKAGES "load-project-packages"

/* Callback generator defines */
#define C_SEPARATOR "\n"
#define C_BODY "\n{\n\n}\n"
#define C_OFFSET 4

#define CHDR_SEPARATOR " "
#define CHDR_BODY ";\n"
#define CHDR_OFFSET 1

static gpointer parent_class;

/* Glade support */

static CppFileType
get_filetype (GFile *file)
{
    if (file)
    {
        gchar* mime_type = anjuta_util_get_file_mime_type (file);
        if (mime_type)
        {
            if (g_str_equal (mime_type, "text/x-csrc"))
                return LS_FILE_C;
            else if (g_str_equal (mime_type, "text/x-chdr"))
                return LS_FILE_CHDR;
            else if (g_str_equal (mime_type, "text/x-c++src"))
                return LS_FILE_CPP;
            else if (g_str_equal (mime_type, "text/x-c++hdr"))
                return LS_FILE_CPPHDR;
            else
                return LS_FILE_OTHER;
        }
    }
    return LS_FILE_OTHER;
}

static void
init_file_type (CppJavaPlugin* lang_plugin)
{
    GFile* file = ianjuta_file_get_file (IANJUTA_FILE (lang_plugin->current_editor),
                                         NULL);

    lang_plugin->filetype = get_filetype (file);
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

static GString*
language_support_generate_c_signature (const gchar* separator,
                                       const gchar* widget,
                                       GSignalQuery query,
                                       gboolean swapped,
                                       const gchar* handler)
{
    GList* names = NULL;
    GString* str = g_string_new ("\n");
    const gchar* widget_param = language_support_get_signal_parameter (widget,
                                                                       &names);
    int i;
    g_string_append (str, g_type_name (query.return_type));
    if (!swapped)
        g_string_append_printf (str, "%s%s (%s *%s",
                                     separator, handler, widget, widget_param);
    else
        g_string_append_printf (str, "%s%s (gpointer user_data, %s *%s",
                                     separator, handler, widget, widget_param);

    for (i = 0; i < query.n_params; i++)
    {
        const gchar* type_name = g_type_name (query.param_types[i]);
        if (!type_name) continue;

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

    anjuta_util_glist_strings_free (names);

    return str;
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

static GFile*
language_support_get_header_file (IAnjutaEditor* editor)
{
    GFile *file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
    GFile *parent = g_file_get_parent (file);
    gchar *parent_uri = g_file_get_uri (parent);
    gchar *basename = g_file_get_basename (file);
    g_object_unref (file);
    g_object_unref (parent);
    gchar *ext = strstr (basename, ".");
    GFile *ret = NULL;

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
                        ret = new_file;
                        goto end;
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
                        ret = new_file;
                        goto end;
                    }
                    g_object_unref (new_file);
                }
                break;
            }
        }
    }

end:
    g_free(basename);
    g_free (parent_uri);

  return ret;
}

static IAnjutaEditor*
language_support_get_editor_from_file (CppJavaPlugin* lang_plugin,
                                                      GFile *file)
{
    IAnjutaDocumentManager *document_manager = anjuta_shell_get_interface (
                                          ANJUTA_PLUGIN (lang_plugin)->shell,
                                          IAnjutaDocumentManager,
                                          NULL);

    IAnjutaDocument *document = ianjuta_document_manager_find_document_with_file
                                                        (document_manager,
                                                         file,
                                                         NULL);

    return IANJUTA_EDITOR (document);
}

static IAnjutaIterable*
language_support_get_mark_position (IAnjutaEditor* editor, gchar* mark)
{
    IAnjutaEditorCell *search_start = IANJUTA_EDITOR_CELL (
                                        ianjuta_editor_get_start_position (editor, NULL));
    IAnjutaEditorCell *search_end = IANJUTA_EDITOR_CELL (
                                        ianjuta_editor_get_end_position (editor, NULL));
    IAnjutaEditorCell *result_start = NULL;
    IAnjutaEditorCell *result_end = NULL;

    ianjuta_editor_search_forward (IANJUTA_EDITOR_SEARCH (editor),
                                   mark, FALSE,
                                   search_start, search_end,
                                   &result_start,
                                   &result_end, NULL);

    if (result_start)
        g_object_unref (result_start);

    return result_end ? IANJUTA_ITERABLE (result_end) : NULL;
}

static IAnjutaIterable*
language_support_find_symbol (CppJavaPlugin* lang_plugin,
                              IAnjutaEditor* editor,
                              const gchar* handler)
{
    IAnjutaSymbolManager *isymbol_manager = anjuta_shell_get_interface (
                                          ANJUTA_PLUGIN (lang_plugin)->shell,
                                          IAnjutaSymbolManager,
                                          NULL);

    IAnjutaSymbolQuery *symbol_query = ianjuta_symbol_manager_create_query (
                                         isymbol_manager,
                                          IANJUTA_SYMBOL_QUERY_SEARCH_FILE,
                                          IANJUTA_SYMBOL_QUERY_DB_PROJECT,
                                          NULL);
    IAnjutaSymbolField field = IANJUTA_SYMBOL_FIELD_FILE_POS;
    ianjuta_symbol_query_set_fields (symbol_query, 1, &field, NULL);

    GFile* file = ianjuta_file_get_file (IANJUTA_FILE (editor),
                                          NULL);

    IAnjutaIterable *iter = ianjuta_symbol_query_search_file (symbol_query,
                                                               handler, file, NULL);

    g_object_unref (file);
    g_object_unref (symbol_query);

    return iter;
}

static gboolean
language_support_get_callback_strings (gchar** separator,
                                       gchar** body,
                                       gint* offset,
                                       CppFileType filetype)
{
    switch (filetype)
    {
        case LS_FILE_C:
        {
            *separator = C_SEPARATOR;
            *body = C_BODY;
            *offset = C_OFFSET;
            break;
        }
        case LS_FILE_CHDR:
        {
            *separator = CHDR_SEPARATOR;
            *body = CHDR_BODY;
            *offset = CHDR_OFFSET;
            break;
        }
        default:
            return FALSE;
    }

    return TRUE;
}

static IAnjutaIterable*
language_support_get_header_editor_and_mark (CppJavaPlugin* lang_plugin,
                                             IAnjutaEditor* editor,
                                             gchar* mark,
                                             IAnjutaEditor** header_editor)
{
    IAnjutaIterable* mark_position = NULL;
    GFile *header_file = language_support_get_header_file (editor);

    /* Yes, we have a header */
    if (header_file)
    {
        *header_editor = language_support_get_editor_from_file (lang_plugin,
                                                                header_file);
        /* We'll now search for the mark */

        mark_position = language_support_get_mark_position (*header_editor, mark);

        g_object_unref (header_file);
    }

    return mark_position;
}

static void
language_support_add_c_callback (CppJavaPlugin* lang_plugin,
                                    IAnjutaEditor* editor,
                                    IAnjutaIterable* position,
                                 GStrv split_signal_data,
                                 CppFileType filetype)
{
    GSignalQuery query;

    gchar* separator;
    gchar* body;
    gint offset;

    const gchar* widget = split_signal_data[0];
    const gchar* signal = split_signal_data[1];
    const gchar* handler = split_signal_data[2];
    //const gchar* user_data = split_signal_data[3]; // Currently unused
    gboolean swapped = g_str_equal (split_signal_data[4], "1");

    GType type = g_type_from_name (widget);
    guint id = g_signal_lookup (signal, type);

    g_signal_query (id, &query);


    if (!language_support_get_callback_strings (&separator, &body, &offset, filetype))
        return;



    GString* str = language_support_generate_c_signature (separator, widget,
                                                          query, swapped, handler);

    g_string_append (str, body);

    ianjuta_editor_insert (editor, position,
                           str->str, -1, NULL);

    /* Code was inserted, we'll now check if we should add a prototype to the header */
    if (filetype == LS_FILE_C)
    {
        IAnjutaEditor* header_editor;
        IAnjutaIterable* mark_position;
        mark_position = language_support_get_header_editor_and_mark (lang_plugin,
                                                                     editor,
                                                                     "/* Callbacks */",
                                                                     &header_editor);
        if (mark_position)
        {
            /* Check if there's a the prototype to the header */
            IAnjutaIterable* symbol;
            symbol = language_support_find_symbol (lang_plugin, header_editor, handler);

            if (symbol)
            {
                g_object_unref (symbol);
            } else {
                /* Add prototype to the header */
                language_support_add_c_callback (lang_plugin, header_editor, mark_position,
                                                 split_signal_data, LS_FILE_CHDR);

            }

            g_object_unref (mark_position);
        }
    }

    gchar *string = g_string_free (str, FALSE);
    /* Emit code-added signal, so symbols will be updated */
    g_signal_emit_by_name (G_OBJECT (editor), "code-added", position, string);

    if (string) g_free (string);

    /* Will now set the caret position offset */
    ianjuta_editor_goto_line (editor,
                              ianjuta_editor_get_line_from_position (
                                            editor, position, NULL) + offset, NULL);
}

static void
on_glade_drop (IAnjutaEditor* editor,
               IAnjutaIterable* iterator,
               const gchar* signal_data,
               CppJavaPlugin* lang_plugin)
{
    GStrv split_signal_data = g_strsplit(signal_data, ":", 5);
    char *handler = split_signal_data[2];
    /**
     * Split signal data format:
     * widget = split_signaldata[0];
     * signal = split_signaldata[1];
     * handler = split_signaldata[2];
     * user_data = split_signaldata[3];
     * swapped = g_str_equal (split_signaldata[4], "1");
     */

    IAnjutaIterable *iter;
    iter = language_support_find_symbol (lang_plugin,
                                         IANJUTA_EDITOR (editor),
                                         handler);
    if (iter == NULL)
    {
        GFile *file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
        CppFileType filetype = get_filetype (file);
        language_support_add_c_callback (lang_plugin, editor, iterator, split_signal_data, filetype);
    } else {
        /* Symbol found, going there */
        ianjuta_editor_goto_line (editor, ianjuta_symbol_get_int (
                                                            IANJUTA_SYMBOL (iter),
                                                            IANJUTA_SYMBOL_FIELD_FILE_POS,
                                                            NULL), NULL);
        g_object_unref (iter);
    }
    g_strfreev (split_signal_data);
}

static gchar* generate_widget_member_decl_str (gchar* widget_name)
{
       return g_strdup_printf ("\n\tGtkWidget* %s;", widget_name);
}

static gchar* generate_widget_member_init_str (gchar* widget_name)
{
       return g_strdup_printf ("\n\tpriv->%s = GTK_WIDGET ("
                               "gtk_builder_get_object(builder, \"%s\"));", widget_name, widget_name);
}

static gboolean insert_after_mark (IAnjutaEditor* editor, gchar* mark,
                                   gchar* code_to_add, CppJavaPlugin* lang_plugin)
{
       IAnjutaIterable* mark_position;
       mark_position = language_support_get_mark_position (editor, mark);
       if (!mark_position)
        return FALSE;

       ianjuta_editor_insert (editor, mark_position, code_to_add, -1, NULL);

       /* Emit code-added signal, so symbols will be updated */
       g_signal_emit_by_name (G_OBJECT (editor), "code-added", mark_position, code_to_add);
       g_object_unref (mark_position);

       return TRUE;
}

static gchar*
generate_widget_member_decl_marker (gchar* ui_filename)
{
    return g_strdup_printf ("/* ANJUTA: Widgets declaration for %s - DO NOT REMOVE */", ui_filename);
}

static gchar*
generate_widget_member_init_marker (gchar* ui_filename)
{
    return g_strdup_printf ("/* ANJUTA: Widgets initialization for %s - DO NOT REMOVE */", ui_filename);
}

static gboolean
glade_widget_member_of_scope (gchar *widget_name, IAnjutaIterable *members)
{
       do {
              IAnjutaSymbol *symbol = IANJUTA_SYMBOL (members);
              const gchar *member_name = ianjuta_symbol_get_string (symbol, IANJUTA_SYMBOL_FIELD_NAME, NULL);
              /* Checks if member already exists... */
              if (g_strcmp0 (member_name, widget_name) == 0) {
                     return TRUE;
              }
       } while (ianjuta_iterable_next (members, NULL));

       return FALSE;
}

static gboolean
glade_widget_already_in_scope (IAnjutaEditor* editor, gchar* widget_name,
                               gchar* mark, CppJavaPlugin* lang_plugin)
{
       gboolean ret = FALSE;
       IAnjutaIterable *mark_position;
       IAnjutaSymbolQuery *query_scope;
       IAnjutaIterable *scope;
       IAnjutaSymbolQuery *query_members;
       mark_position = language_support_get_mark_position (editor, mark);

       if (!mark_position)
              return FALSE;

       int line = ianjuta_editor_get_line_from_position (editor, mark_position, NULL);
       g_object_unref(mark_position);


       IAnjutaSymbolManager *symbol_manager =
              anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell, IAnjutaSymbolManager, NULL);

       query_scope = ianjuta_symbol_manager_create_query (symbol_manager,
                                                          IANJUTA_SYMBOL_QUERY_SEARCH_SCOPE,
                                                          IANJUTA_SYMBOL_QUERY_DB_PROJECT, NULL);
       if (!query_scope)
              return FALSE;

       GFile *file = ianjuta_file_get_file (IANJUTA_FILE(editor), NULL);
       gchar* path = g_file_get_path (file);

       scope = ianjuta_symbol_query_search_scope (query_scope, path, line, NULL);
       g_object_unref(query_scope);

       if (!scope)
              return FALSE;

       query_members = ianjuta_symbol_manager_create_query (symbol_manager,
                                                            IANJUTA_SYMBOL_QUERY_SEARCH_MEMBERS,
                                                            IANJUTA_SYMBOL_QUERY_DB_PROJECT,
                                                            NULL);

       if (query_members)
       {
              IAnjutaIterable *members = ianjuta_symbol_query_search_members (
                                                   query_members,
                                                   IANJUTA_SYMBOL(scope), NULL);
              g_object_unref(query_members);

              if (members)
              {
                     ret = glade_widget_member_of_scope (widget_name, members);
                     g_object_unref(members);
              }
       }

       g_object_unref(scope);

       return ret;
}

static void insert_member_decl_and_init (IAnjutaEditor* editor, gchar* widget_name,
                                         gchar* ui_filename, CppJavaPlugin* lang_plugin)
{
       AnjutaStatus* status;
       gchar* member_decl = generate_widget_member_decl_str(widget_name);
       gchar* member_init = generate_widget_member_init_str(widget_name);

       gchar* member_decl_marker = generate_widget_member_decl_marker (ui_filename);
       gchar* member_init_marker = generate_widget_member_init_marker (ui_filename);

       status = anjuta_shell_get_status (ANJUTA_PLUGIN (lang_plugin)->shell, NULL);

       if (!glade_widget_already_in_scope (editor, widget_name, member_decl_marker, lang_plugin))
       if (insert_after_mark (editor, member_decl_marker, member_decl, lang_plugin))
       {
              insert_after_mark (editor, member_init_marker, member_init, lang_plugin);
              anjuta_status_set (status, _("Code added for widget."));
       }

       g_free (member_decl);
       g_free (member_init);

       g_free (member_decl_marker);
       g_free (member_init_marker);
}

static void
on_glade_member_add (IAnjutaEditor* editor, gchar* widget_typename,
                     gchar* widget_name, gchar* path, CppJavaPlugin* lang_plugin)
{
    GFile* ui_file = g_file_new_for_path (path);
    gchar* ui_filename = g_file_get_basename (ui_file);

    insert_member_decl_and_init (editor, widget_name, ui_filename, lang_plugin);
}

static void
on_glade_callback_add (IAnjutaEditor* editor,
                       gchar *widget_typename,
                       gchar *signal_name,
                       gchar *handler_name,
                       gchar *object,
                       gboolean swap,
                       gboolean after,
                       gchar* path,
                       CppJavaPlugin* lang_plugin)
{
    GFile* ui_file = g_file_new_for_path (path);
    gchar* ui_filename = g_file_get_basename (ui_file);

    /* Using marker to search for compatibility */
    gchar *mark = generate_widget_member_init_marker (ui_filename);
    IAnjutaIterable* mark_position;
    mark_position = language_support_get_mark_position (editor, mark);
    if (mark_position)
    {
        IAnjutaIterable* end = ianjuta_editor_get_end_position (editor, NULL);

        /* String format: widgettypename:signalname:handler_name:object:swap:after */
        gchar *signal_data = g_strdup_printf("%s:%s:%s:%s:%s:%s",
                                             widget_typename,
                                             signal_name,
                                             handler_name,
                                             object,
                                             swap?"1":"0",
                                             after?"1":"0");

        on_glade_drop (editor, end, signal_data, lang_plugin);

        g_free(signal_data);
    }
    g_free(mark);
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

    if (!(lang_plugin->current_language &&
        (g_str_equal (lang_plugin->current_language, "C")
         || g_str_equal (lang_plugin->current_language, "C++")
         || g_str_equal (lang_plugin->current_language, "Vala")
         || g_str_equal (lang_plugin->current_language, "Java"))))
    {
        return;
    }

    init_file_type (lang_plugin);
	
    if (g_str_equal (lang_plugin->current_language, "C" ) ||
        g_str_equal (lang_plugin->current_language, "C++"))
    {
        if (IANJUTA_IS_EDITOR_GLADE_SIGNAL (lang_plugin->current_editor))
        {
            g_signal_connect (lang_plugin->current_editor,
                              "drop-possible", G_CALLBACK (on_glade_drop_possible),
                              lang_plugin);
            g_signal_connect (lang_plugin->current_editor,
                              "drop", G_CALLBACK (on_glade_drop),
                              lang_plugin);
        }

        g_signal_connect (lang_plugin->current_editor,
                          "glade-callback-add",
                          G_CALLBACK (on_glade_callback_add),
                          lang_plugin);

        g_signal_connect (lang_plugin->current_editor,
                          "glade-member-add",
                          G_CALLBACK (on_glade_member_add),
                          lang_plugin);

        lang_plugin->packages = cpp_packages_new (ANJUTA_PLUGIN (lang_plugin));
        cpp_packages_load(lang_plugin->packages, FALSE);
    }

    lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (CppJavaPlugin *lang_plugin)
{
    if (!lang_plugin->support_installed)
        return;

    g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
                                          on_glade_drop_possible, lang_plugin);
    g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
                                          on_glade_drop, lang_plugin);
    g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
                                          G_CALLBACK (on_glade_member_add),
                                          lang_plugin);
    g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
                                          G_CALLBACK (on_glade_callback_add),
                                          lang_plugin);

    if (lang_plugin->packages)
    {
        g_object_unref (lang_plugin->packages);
        lang_plugin->packages = NULL;
    }
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

static void
on_swap_activate (GtkAction* action, gpointer data)
{
    GFile* file;
    CppJavaPlugin *lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (data);
    IAnjutaDocumentManager* docman =
        anjuta_shell_get_interface (ANJUTA_PLUGIN(lang_plugin)->shell,
                                    IAnjutaDocumentManager,
                                    NULL);
    if (!lang_plugin->current_editor || !docman)
        return;

//    file = ianjuta_file_get_file (IANJUTA_FILE (lang_plugin->current_editor),
//                                  NULL);

    file = language_support_get_header_file (
                                        IANJUTA_EDITOR (lang_plugin->current_editor));

    if (g_file_query_exists (file, NULL))
    {
        ianjuta_document_manager_goto_file_line (docman,
                                                 file,
                                                 -1,
                                                 NULL);
        g_object_unref (file);
    }
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
    g_object_unref (plugin->editor_settings);

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
    plugin->settings = g_settings_new (PREF_SCHEMA);
    plugin->editor_settings = g_settings_new (ANJUTA_PREF_SCHEMA_PREFIX IANJUTA_EDITOR_PREF_SCHEMA);
    plugin->packages = NULL;
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

#define PREF_WIDGET_PACKAGES "preferences:load-project-packages"
#define PREF_WIDGET_PKG_CONFIG "pkg_config_chooser1"

static void
cpp_java_plugin_select_user_packages (CppJavaPlugin* plugin,
                                      AnjutaPkgConfigChooser* chooser)
{
    gchar* user_packages = g_settings_get_string (plugin->settings,
                                                  PREF_USER_PACKAGES);
    GStrv pkgs = g_strsplit (user_packages, ";", -1);
    gchar** pkg;
    GList* packages = NULL;
    for (pkg = pkgs; *pkg != NULL; pkg++)
    {
        packages = g_list_append (packages, *pkg);
    }
    anjuta_pkg_config_chooser_set_active_packages (chooser,
                                                   packages);
    g_strfreev (pkgs);
    g_free (user_packages);
    g_list_free (packages);
}

static void
on_project_packages_toggled (GtkToggleButton* button,
                             CppJavaPlugin* plugin)
{
    GtkWidget* pkg_config;
    gboolean active = gtk_toggle_button_get_active (button);
    pkg_config = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_PKG_CONFIG));

    gtk_widget_set_sensitive (pkg_config, !active);

    anjuta_pkg_config_chooser_set_active_packages (ANJUTA_PKG_CONFIG_CHOOSER (pkg_config),
                                                   NULL);
    if (!active)
    {
        anjuta_pkg_config_chooser_show_active_only (ANJUTA_PKG_CONFIG_CHOOSER (pkg_config),
                                                    FALSE);
        cpp_java_plugin_select_user_packages (plugin, ANJUTA_PKG_CONFIG_CHOOSER (pkg_config));
        cpp_packages_load (plugin->packages, TRUE);
    }
    else
    {
        anjuta_pkg_config_chooser_set_active_packages (ANJUTA_PKG_CONFIG_CHOOSER (pkg_config),
                                                       NULL);
        anjuta_pkg_config_chooser_show_active_only (ANJUTA_PKG_CONFIG_CHOOSER (pkg_config),
                                                    TRUE);
    }
}

static void
cpp_java_plugin_update_user_packages (CppJavaPlugin* plugin,
                                      AnjutaPkgConfigChooser* chooser)
{
    GList* pkg;
    GList* packages = anjuta_pkg_config_chooser_get_active_packages (chooser);
    GString* pkg_string = g_string_new (NULL);

    for (pkg = packages; pkg != NULL; pkg = g_list_next (pkg))
    {
        if (strlen (pkg_string->str))
        {
                pkg_string = g_string_append_c (pkg_string, ';');
        }
        pkg_string = g_string_append (pkg_string, pkg->data);
    }
    g_settings_set_string (plugin->settings, PREF_USER_PACKAGES,
                           pkg_string->str);
    g_string_free (pkg_string, TRUE);
}

static void
on_package_activated (AnjutaPkgConfigChooser *self, const gchar* package,
                      gpointer data)
{
    CppJavaPlugin* plugin;

    plugin = ANJUTA_PLUGIN_CPP_JAVA (data);

    g_message ("Activate package");

    cpp_java_plugin_update_user_packages (plugin, self);
    cpp_packages_load (plugin->packages, TRUE);
}

static void
on_package_deactivated (AnjutaPkgConfigChooser *self, const gchar* package,
                        gpointer data)
{
    CppJavaPlugin* plugin;
    IAnjutaSymbolManager *isymbol_manager;
    gchar* version;

    plugin = ANJUTA_PLUGIN_CPP_JAVA (data);

    DEBUG_PRINT ("deactivated %s", package);

    isymbol_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
                                                IAnjutaSymbolManager,
                                                NULL);
    version = anjuta_pkg_config_get_version (package);
    if (version)
    {
        ianjuta_symbol_manager_deactivate_package (isymbol_manager,
                                                   package,
                                                   version,
                                                   NULL);
    }
    g_free (version);

    cpp_java_plugin_update_user_packages (plugin, self);
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
                                         "preferences", _("API Tags (C/C++)"),
                                         ICON_FILE);
    toggle = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_PACKAGES));
    g_signal_connect (toggle, "toggled", G_CALLBACK (on_project_packages_toggled),
                      plugin);
    on_project_packages_toggled (GTK_TOGGLE_BUTTON (toggle), plugin);

    pkg_config = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_PKG_CONFIG));
    anjuta_pkg_config_chooser_show_active_column (ANJUTA_PKG_CONFIG_CHOOSER (pkg_config),
                                                  TRUE);

    g_signal_connect (G_OBJECT (pkg_config), "package-activated",
                      G_CALLBACK (on_package_activated), plugin);

    g_signal_connect (G_OBJECT (pkg_config), "package-deactivated",
                      G_CALLBACK (on_package_deactivated), plugin);

    if (!g_settings_get_boolean (plugin->settings,
                                 PREF_PROJECT_PACKAGES))
        cpp_java_plugin_select_user_packages (plugin, ANJUTA_PKG_CONFIG_CHOOSER (pkg_config));
	
    gtk_widget_show (pkg_config);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
                      GError** e)
{
    CppJavaPlugin* plugin = ANJUTA_PLUGIN_CPP_JAVA (ipref);
    anjuta_preferences_remove_page(prefs, _("API Tags (C/C++)"));
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
