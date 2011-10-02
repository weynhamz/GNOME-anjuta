/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * am-scanner.l
 * Copyright (C) Sébastien Granjoux 2009 <seb.sfo@free.fr>
 *
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

%{

#include "am-scanner.h"
#include "am-parser.h"
#include "amp-node.h"
#include "amp-target.h"

#include "libanjuta/anjuta-debug.h"
#include "libanjuta/anjuta-token-stream.h"

#include <stdlib.h>
#include <string.h>


#define YY_INPUT(buffer, result, max_size) result = anjuta_token_stream_read (yyextra->stream, buffer, max_size)

#define YY_EXTRA_TYPE  AmpAmScanner*

#define YY_DECL static int am_yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,yyscan_t yyscanner)

//#define YY_USER_INIT {yy_flex_debug = 1;}

static int amp_am_scanner_parse_end (AmpAmScanner *scanner);

#define RETURN(tok) *yylval = anjuta_token_stream_tokenize (yyextra->stream, tok, yyleng); \
                    return tok

struct _AmpAmScanner
{
	yyscan_t scanner;

	AnjutaTokenStream *stream;

	AmpProject *project;
	AmpGroupNode *group;
	GHashTable *orphan_properties;
	GList *am_variables;
	GList *variables;
	gboolean eof;		/* TRUE to emit EOF at the end */
	gboolean expansion;		/* Expand variables */
};

struct _AmpVariableDepend
{
	GList *token;
	GList *depend;
	const gchar *name;
	gboolean evaluated;
};

typedef struct _AmpVariableDepend AmpVariableDepend;


%}

%option reentrant noyywrap yylineno

/* Remove some warnings */
%option nounput noinput noyy_pop_state noyy_top_state

%option prefix="amp_am_yy"

%option bison-bridge bison-locations

%option never-interactive

%option batch

%option debug

NAME          [^ \t\n\r:#=$"'`&@\\]*

%%

<INITIAL><<EOF>>					{ gint ret = amp_am_scanner_parse_end (yyextra); if (ret !=1) return ret; }

<INITIAL>\n 						{ RETURN (END_OF_LINE); }

<INITIAL>([ ]|\\\n)([ \t]|\\\n)*	{ RETURN (SPACE); }

<INITIAL>([ \t])*#.*\n 				{ RETURN (COMMENT); }

<INITIAL>\t 						{ RETURN (TAB); }

<INITIAL>@{NAME}@ 					{ RETURN (MACRO); }

<INITIAL>\$\([^ \t\n\r:#=$)]+\) 	{ RETURN (VARIABLE); }

<INITIAL>\$\{[^ \t\n\r:#=$}]+\} 	{ RETURN (VARIABLE); }

<INITIAL>\$[^ \t\n\r\(\{] 			{ RETURN (VARIABLE); }

<INITIAL>: 							{ RETURN (COLON); }

<INITIAL>:: 						{ RETURN (DOUBLE_COLON); }

<INITIAL>; 							{ RETURN (SEMI_COLON); }

<INITIAL>\| 						{ RETURN (ORDER); }

<INITIAL>\= 						{ RETURN (EQUAL); }

<INITIAL>:= 						{ RETURN (IMMEDIATE_EQUAL); }

<INITIAL>\?= 						{ RETURN (CONDITIONAL_EQUAL); }

<INITIAL>\+= 						{ RETURN (APPEND); }

<INITIAL>\\[ ] 						{ RETURN (CHARACTER); }

<INITIAL>\\: 						{ RETURN (CHARACTER); }

<INITIAL>\\= 						{ RETURN (CHARACTER); }

<INITIAL>\\# 						{ RETURN (CHARACTER); }

<INITIAL>include					{ RETURN (INCLUDE); }

<INITIAL>\-include					{ RETURN (INCLUDE); }

<INITIAL>SUBDIRS 					{ RETURN (SUBDIRS); }

<INITIAL>DIST_SUBDIRS 				{ RETURN (DIST_SUBDIRS); }

<INITIAL>{NAME}_DATA 				{ RETURN (_DATA); }

<INITIAL>{NAME}_HEADERS 			{ RETURN (_HEADERS); }

<INITIAL>{NAME}_LIBRARIES 			{ RETURN (_LIBRARIES); }

<INITIAL>{NAME}_LISP 				{ RETURN (_LISP); }

<INITIAL>{NAME}_LTLIBRARIES 		{ RETURN (_LTLIBRARIES); }

<INITIAL>{NAME}_MANS 				{ RETURN (_MANS); }

<INITIAL>{NAME}_PROGRAMS 			{ RETURN (_PROGRAMS); }

<INITIAL>{NAME}_PYTHON 				{ RETURN (_PYTHON); }

<INITIAL>{NAME}_JAVA 				{ RETURN (_JAVA); }

<INITIAL>{NAME}_SCRIPTS 			{ RETURN (_SCRIPTS); }

<INITIAL>{NAME}_SOURCES 			{ RETURN (_SOURCES); }

<INITIAL>{NAME}_TEXINFOS 			{ RETURN (_TEXINFOS); }

<INITIAL>{NAME}dir					{ RETURN (_DIR); }

<INITIAL>AM_LDFLAGS					{ RETURN (_LDFLAGS);}

<INITIAL>AM_CPPFLAGS				{ RETURN (_CPPFLAGS);}

<INITIAL>AM_CFLAGS					{ RETURN (_CFLAGS);}

<INITIAL>AM_CXXFLAGS				{ RETURN (_CXXFLAGS);}

<INITIAL>AM_JAVACFLAGS				{ RETURN (_JAVACFLAGS);}

<INITIAL>AM_VALAFLAGS				{ RETURN (_VALAFLAGS);}

<INITIAL>AM_FCFLAGS					{ RETURN (_FCFLAGS);}

<INITIAL>AM_OBJCFLAGS				{ RETURN (_OBJCFLAGS);}

<INITIAL>AM_LFLAGS					{ RETURN (_LFLAGS);}

<INITIAL>AM_YFLAGS					{ RETURN (_YFLAGS);}

<INITIAL>{NAME}_LDFLAGS				{ RETURN (TARGET_LDFLAGS);}

<INITIAL>{NAME}_CPPFLAGS			{ RETURN (TARGET_CPPFLAGS);}

<INITIAL>{NAME}_CFLAGS				{ RETURN (TARGET_CFLAGS);}

<INITIAL>{NAME}_CXXFLAGS			{ RETURN (TARGET_CXXFLAGS);}

<INITIAL>{NAME}_JAVACFLAGS			{ RETURN (TARGET_JAVACFLAGS);}

<INITIAL>{NAME}_VALAFLAGS			{ RETURN (TARGET_VALAFLAGS);}

<INITIAL>{NAME}_FCFLAGS				{ RETURN (TARGET_FCFLAGS);}

<INITIAL>{NAME}_OBJCFLAGS			{ RETURN (TARGET_OBJCFLAGS);}

<INITIAL>{NAME}_LFLAGS				{ RETURN (TARGET_LFLAGS);}

<INITIAL>{NAME}_YFLAGS				{ RETURN (TARGET_YFLAGS);}

<INITIAL>{NAME}_DEPENDENCIES		{ RETURN (TARGET_DEPENDENCIES);}

<INITIAL>{NAME}_LDADD				{ RETURN (TARGET_LDADD);}

<INITIAL>{NAME}_LIBADD				{ RETURN (TARGET_LIBADD);}

<INITIAL>{NAME} 					{ RETURN (NAME); }

<INITIAL>. 							{ RETURN (CHARACTER); }

%%

/* Private functions
 *---------------------------------------------------------------------------*/

static void
amp_am_scanner_reparse_token (AmpAmScanner *scanner, AnjutaToken *token, GFile *filename)
{
	AnjutaToken *root;
	AnjutaToken *list;

	if (token == NULL) return;

	token = anjuta_token_concat (token);

	yylex_init(&scanner->scanner);
	yyset_extra (scanner, scanner->scanner);

	root = anjuta_token_new_static (ANJUTA_TOKEN_FILE, NULL);
	list = amp_am_scanner_parse_token (scanner, root, token, filename, NULL);
	list = anjuta_token_delete_parent (list);
	if (list != NULL) anjuta_token_insert_before (token, list);
	anjuta_token_free (token);
}

/* Private functions, variable dependencies
 *---------------------------------------------------------------------------*/

static AmpVariableDepend *
amp_variable_depend_new (void)
{
	AmpVariableDepend *dep;

	dep = g_new0 (AmpVariableDepend, 1);

	return dep;
}

static void
amp_variable_depend_free (AmpVariableDepend *depend)
{
	g_list_free (depend->token);
	g_list_free (depend->depend);
}

static void
list_depend (AnjutaToken *token, gpointer user_data)
{
	GList **depend = (GList **)user_data;

	if (anjuta_token_get_type (token) == ANJUTA_TOKEN_VARIABLE)
	{
		gchar *string;
		gchar *name;
		guint length;

		string = anjuta_token_evaluate_name(token);
		length = strlen (string);
		if (length > 1)
		{
			if (string[1] == '(')
			{
				string[length - 1] = '\0';
				name = string + 2;
			}
			else
			{
				string[2] = '\0';
				name = string + 1;
			}
			name = g_strdup (name);
			*depend = g_list_prepend (*depend, name);
		}
	}
}

static void
convert_dependencies (gchar *variable_name, AmpVariableDepend *variable, gpointer user_data)
{
	GList *list;
	GHashTable *dependencies = (GHashTable *)user_data;

	variable->token = g_list_reverse (variable->token);

	list = g_list_first (variable->depend);
	if (list == NULL) variable->evaluated = TRUE;
	for (; list != NULL;)
	{
		gchar *name = (gchar *)list->data;
		AmpVariableDepend *depend;
		GList *next;

		depend =g_hash_table_lookup (dependencies, name);
		g_free (name);
		next = g_list_next (list);

		if (depend == NULL)
		{
			/* Unexisting variable, remove it from dependencies */
			variable->depend = g_list_delete_link (variable->depend, list);
		}
		else
		{
			GList *dup;

			/* Look for duplicate */
			for (dup = g_list_first (variable->depend); dup != list; dup = g_list_next (dup))
			{
				if (dup->data == depend) break;
			}

			if (dup == list)
			{
				dup->data = depend;
			}
			else
			{
				/* Find a duplicate */
				variable->depend = g_list_delete_link (variable->depend, list);
			}
		}
		list = next;
	}

}

static GHashTable *
amp_am_scanner_compute_dependencies (AmpAmScanner *scanner)
{
	GHashTable *variable_dependencies;
	GList *variable;

	/* Compute variables dependencies */
	variable_dependencies = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)amp_variable_depend_free);
	for (variable = g_list_first (scanner->variables); variable != NULL; variable = g_list_next (variable))
	{
		AnjutaToken *arg;
		AnjutaToken *value;
		gchar *name;
		GList *list = NULL;
		AmpVariableDepend *depend;

		arg = anjuta_token_first_item ((AnjutaToken *)variable->data);
		name = g_strstrip (anjuta_token_evaluate (arg));

		/* Find variable dependencies data */
		depend =(AmpVariableDepend *)g_hash_table_lookup (variable_dependencies, name);
		if (depend == NULL)
		{
			depend = amp_variable_depend_new ();
			depend->name = name;
			g_hash_table_insert (variable_dependencies, name, depend);
		}

		depend->token = g_list_prepend (depend->token, variable->data);

		/* Find dependencies */
		value = anjuta_token_last_item ((AnjutaToken *)variable->data);
		anjuta_token_foreach_token (value, list_depend, &list);
		depend->depend = g_list_concat (list, depend->depend);
	}

	/* Replace name in dependencies by pointer */
	g_hash_table_foreach (variable_dependencies, (GHFunc)convert_dependencies, variable_dependencies);

	return variable_dependencies;
}

static gint
amp_am_scanner_parse_end (AmpAmScanner *scanner)
{

   	if (scanner->stream == NULL)
   	{
		yyterminate();
	}
   	else
   	{
		if (scanner->eof)
		{
			scanner->eof = FALSE;
			return END_OF_FILE;
		}

		yypop_buffer_state(scanner->scanner);
    	scanner->stream = anjuta_token_stream_pop (scanner->stream);

		if (scanner->stream == NULL)
		{
			yyterminate();
		}
		else
		{
			scanner->eof = anjuta_token_stream_get_current_file (scanner->stream) != NULL;

			/* Continue parsing the parent file */
			return 1;
		}
	}
}


/* Parser functions
 *---------------------------------------------------------------------------*/

void
amp_am_yyerror (YYLTYPE *loc, AmpAmScanner *scanner, char const *s)
{
    AnjutaTokenFileLocation location;

    if (amp_project_get_token_location (scanner->project, &location, *loc))
    {
        g_message ("%s:%d.%d %s\n", location.filename, location.line, location.column, s);
        g_free (location.filename);
    }
    else
    {
        g_message ("%s \n", s);
    }
}

void
amp_am_scanner_set_am_variable (AmpAmScanner *scanner, AnjutaToken *variable)
{
	if (!scanner->expansion)
	{
		/* Keep variable token to expand them at the end */
		scanner->am_variables = g_list_prepend (scanner->am_variables, variable);
	}
	else
	{
	    amp_project_set_am_variable (scanner->project, scanner->group, variable, scanner->orphan_properties);
	}
}

void
amp_am_scanner_include (AmpAmScanner *scanner, AnjutaToken *list)
{
	GFile *file;
	AnjutaTokenFile *include;
	AnjutaToken *token;
	AnjutaToken *name;
	gchar *filename;

	name = anjuta_token_first_item (list);
	name = anjuta_token_next_item (name);
	filename = g_strstrip (anjuta_token_evaluate (name));
	//g_message ("read include =%s=", filename);
	file = g_file_resolve_relative_path (anjuta_token_stream_get_current_directory (scanner->stream), filename);
	g_free (filename);
	include = anjuta_token_file_new (file);
	token = anjuta_token_file_load (include, NULL);
	amp_am_scanner_parse_token (scanner, list, token, file, NULL);
	g_object_unref (file);
}

void
amp_am_scanner_update_variable (AmpAmScanner *scanner, AnjutaToken *variable)
{
	if (scanner->expansion == FALSE) scanner->variables = g_list_prepend (scanner->variables, variable);
    amp_group_node_update_variable (scanner->group, variable);
}


void
amp_am_scanner_parse_variable (AmpAmScanner *scanner, AnjutaToken *variable)
{
	AnjutaToken *value;

	anjuta_token_set_type (variable, ANJUTA_TOKEN_VARIABLE);

	value = amp_group_node_get_variable_token (scanner->group, variable);
	if (value != NULL)
	{
		amp_am_scanner_parse_token (scanner, variable, value, NULL, NULL);
	}
}


/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_am_scanner_parse_token (AmpAmScanner *scanner, AnjutaToken *root, AnjutaToken *content, GFile *filename, GError **error)
{
    AnjutaToken *first;
    AnjutaTokenStream *stream;

    stream = anjuta_token_stream_push (scanner->stream, root, content, filename);
    first = anjuta_token_stream_get_root (stream);

	scanner->eof = filename != NULL;

    if (scanner->stream != NULL)
    {
        /* Parse an included file or a expanded variable */

        scanner->stream = stream;
        yypush_buffer_state(yy_create_buffer(NULL, YY_BUF_SIZE, scanner->scanner), scanner->scanner);
    }
    else
    {
        amp_am_yypstate *ps;
        gint status;

        scanner->stream = stream;
        ps = amp_am_yypstate_new ();
        do
        {
            YYSTYPE yylval_param;
            YYLTYPE yylloc_param;
            gint yychar = am_yylex (&yylval_param, &yylloc_param, scanner->scanner);

			yylloc_param = yylval_param;
            status = amp_am_yypush_parse (ps, yychar, &yylval_param, &yylloc_param, scanner);

        } while (status == YYPUSH_MORE);
        amp_am_yypstate_delete (ps);

		/* Evaluate autotools variable and needed variables at the end */
		if (scanner->expansion == FALSE)
		{
			GHashTable *variable_dependencies;
			GList *variables;
			GList *var;
			guint evaluated;

			variable_dependencies =amp_am_scanner_compute_dependencies (scanner);
			variables = g_hash_table_get_values (variable_dependencies);

			/* Reevaluate variable having dependencies */
			scanner->expansion = TRUE;
			do
			{
				evaluated = 0;
				for (var =variables; var != NULL;)
				{
					AmpVariableDepend *depend = (AmpVariableDepend *)var->data;
					GList *next;

					next = g_list_next (var);
					if (depend->depend == NULL)
					{
						/* No need to reevaluate */
						variables = g_list_delete_link (variables, var);
					}
					else
					{
						/* Check that all dependencies are evaluated */
						GList*list;
						gboolean missing = FALSE;

						for (list = g_list_first (depend->depend); list != NULL; list = g_list_next (list))
						{
							if (((AmpVariableDepend *)list->data)->evaluated == FALSE)
							{
								missing = TRUE;
								break;
							}
						}
						if (missing == FALSE)
						{
							for (list = g_list_first (depend->token); list != NULL; list = g_list_next (list))
							{
								amp_am_scanner_reparse_token (scanner, (AnjutaToken *)list->data, filename);
							}
							variables = g_list_delete_link (variables, var);
							depend->evaluated = TRUE;
							evaluated++;
						}
					}
					var = next;
				}
			}
			while (evaluated);

			/* The remaining variables have dependencies loop, evaluate them in order */
			if (variables != NULL) g_warning ("Dependencies loop in variables");
			for (var =variables; var != NULL; var = g_list_next (var))
			{
				AmpVariableDepend *depend = (AmpVariableDepend *)var->data;
				GList *list;

				for (list = g_list_first (depend->token); list != NULL; list = g_list_next (list))
				{
					amp_am_scanner_reparse_token (scanner, (AnjutaToken *)list->data, filename);
				}
			}
			g_list_free (variables);
			g_hash_table_destroy (variable_dependencies);

			/* Evaluate autotools variables */
			scanner->am_variables = g_list_reverse (scanner->am_variables);
			for (var = g_list_first (scanner->am_variables); var != NULL; var = g_list_next (var))
			{
				AnjutaToken *token = (AnjutaToken *)var->data;

				amp_am_scanner_reparse_token (scanner, token, filename);
			}
		}
    }

    return first;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AmpAmScanner *
amp_am_scanner_new (AmpProject *project, AmpGroupNode *group)
{
	AmpAmScanner *scanner;

	scanner = g_new0 (AmpAmScanner, 1);

	scanner->project = project;
	scanner->group = group;
	scanner->eof = FALSE;

	/* Create hash table for sources list */
	scanner->orphan_properties = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)amp_target_node_free);

	/* Create hash table for variable */
	scanner->am_variables = NULL;
	scanner->expansion = FALSE;

	/* Create list of variable */
	scanner->variables = NULL;

    yylex_init(&scanner->scanner);
    yyset_extra (scanner, scanner->scanner);

	return scanner;
}

void
amp_am_scanner_free (AmpAmScanner *scanner)
{
	g_return_if_fail (scanner != NULL);

    yylex_destroy(scanner->scanner);

	/* Free unused sources files */
	g_hash_table_destroy (scanner->orphan_properties);

	g_list_free (scanner->am_variables);

	g_list_free (scanner->variables);

	g_free (scanner);
}
