/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  transform.c
 *	Copyright (C) 2006 Armin Burgmeier
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU Library General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "transform.h"

#include <string.h>
#include <ctype.h>

typedef struct _CgTransformParamGuess CgTransformParamGuess;
struct _CgTransformParamGuess
{
	const gchar *gtype;
	const gchar *paramspec;
};

typedef struct _CgTransformGTypeGuess CgTransformGTypeGuess;
struct _CgTransformGTypeGuess
{
	const gchar *ctype;
	const gchar *gtype_prefix;
	const gchar *gtype_name;
};

/* This function looks up a flag with the given abbrevation (which has not
 * to be null-terminated) in the given flag list. */
static const CgElementEditorFlags *
cg_transform_lookup_flag (const CgElementEditorFlags *flags,
                          gchar *abbrevation,
                          size_t len)
{
	const CgElementEditorFlags *flag;
	for (flag = flags; flag->name != NULL; ++ flag)
	{
		if (strncmp (flag->abbrevation, abbrevation, len) == 0)
		{
			if(flag->abbrevation[len] == '\0')
			{
				return flag;
			}
		}
	}
	
	return NULL;
}

/* This function tries to convert a native C Type like int, float or
 * unsigned long into a GType like G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_ULONG. */
gboolean
cg_transform_default_c_type_to_g_type (const gchar *c_type,
                                       const gchar **g_type_prefix,
                                       const gchar **g_type_name)
{
	static const CgTransformGTypeGuess DEFAULT_TYPES[] =
	{
		{ "int", "G", "INT" },
		{ "gint", "G", "INT" },
		{ "unsigned int", "G", "UINT" },
		{ "guint", "G", "UINT" },
		{ "char", "G", "CHAR" },
		{ "gchar", "G", "CHAR" },
		{ "unsigned char", "G", "UCHAR" },
		{ "guchar", "G", "UCHAR" },
		{ "long", "G", "LONG" },
		{ "glong", "G", "LONG" },
		{ "unsigned long", "G", "ULONG" },
		{ "gulong", "G", "ULONG" },
		{ "gint64", "G", "INT64" },
		{ "guint64", "G", "UINT64" },
		{ "float", "G", "FLOAT" },
		{ "double", "G", "DOUBLE" },
		{ "char*", "G", "STRING" },
		{ "gchar*", "G", "STRING" },
		{ "char *", "G", "STRING" },
		{ "gchar *", "G", "STRING" },
		{ "const char*", "G", "STRING" },
		{ "const gchar*", "G", "STRING" },
		{ "const char *", "G", "STRING" },
		{ "const gchar *", "G", "STRING" },
		{ "gpointer", "G", "POINTER" },
		{ "void*", "G", "POINTER" },
		{ "void *", "G", "POINTER" },
		{ "gconstpointer", "G", "POINTER" },
		{ "const void*", "G", "POINTER" },
		{ "const void *", "G", "POINTER" },
		{ "void", "G", "NONE" },
		{ "gboolean", "G", "BOOLEAN" },
		{ "GParamSpec*", "G", "PARAM" },
		{ "GParamSpec *", "G", "PARAM" },
		{ NULL, NULL, NULL }
	};
	
	const CgTransformGTypeGuess *guess;
	
	for (guess = DEFAULT_TYPES; guess->ctype != NULL; ++ guess)
	{
		if (strcmp (guess->ctype, c_type) == 0)
		{
			*g_type_prefix = guess->gtype_prefix;
			*g_type_name = guess->gtype_name;
			return TRUE;
		}
	}
	
	return FALSE;
}

/* This function tries to convert a custom c_type like GtkTreeIter to
 * a gobject type like GTK_TYPE_TREE_ITER. It does this by parsing the C type.
 * The code is mostly borrowed from old action-callbacks.c by Dave
 * Huseby and Massimo Cora'. */
void
cg_transform_custom_c_type_to_g_type (const gchar *c_type,
                                      gchar **g_type_prefix,
                                      gchar **g_type_name,
                                      gchar **g_func_prefix)
{
	size_t name_len;
	gboolean first;
	gboolean prefix;
	GString *str_type_prefix;
	GString *str_type_name;
	GString *str_func_prefix;
	
	name_len = strlen (c_type);

	if (g_type_prefix != NULL) str_type_prefix = g_string_sized_new (name_len);
	if (g_type_name != NULL) str_type_name = g_string_sized_new (name_len);
	if (g_type_prefix != NULL) str_func_prefix = g_string_sized_new (name_len);
	
	first = TRUE;
	prefix = TRUE;
	
	for (; *c_type != '\0'; ++ c_type)
	{
		if (first == TRUE)
		{
			if (g_func_prefix != NULL)
				g_string_append_c (str_func_prefix, tolower (*c_type));
			if (g_type_prefix != NULL)
				g_string_append_c (str_type_prefix, toupper (*c_type));

			first = FALSE;
		}
		else
		{
			if (isupper (*c_type))
			{
				if (g_func_prefix != NULL)
					g_string_append_c (str_func_prefix, '_');

				prefix = FALSE;
			}

			if (g_func_prefix != NULL)
				g_string_append_c (str_func_prefix, tolower (*c_type));

			if (prefix == TRUE)
			{
				if (g_type_prefix != NULL)
					g_string_append_c (str_type_prefix, toupper (*c_type));
			}
			else
			{
				if (g_type_name != NULL)
				{
					if (isupper (*c_type) && str_type_name->len > 0)
						g_string_append_c (str_type_name, '_');

					g_string_append_c (str_type_name, toupper (*c_type));
				}
			}
		}
	}
	
	if (g_type_prefix != NULL)
		*g_type_prefix = g_string_free (str_type_prefix, FALSE);

	if (g_type_name != NULL)
		*g_type_name = g_string_free (str_type_name, FALSE);

	if (g_func_prefix != NULL)
		*g_func_prefix = g_string_free (str_func_prefix, FALSE);
}

/* This function tries to convert any possible C type to its corresponding
 * GObject type. First, it looks whether the C type is a default type. If not,
 * it strips leading const or a trailing * away (because it does not matter
 * whether we have const GtkTreeIter* or GtkTreeIter, it all results in the
 * same gobject type, namely GTK_TYPE_TREE_ITER) and calls
 * cg_transform_custom_c_type_to_g_type. */
void
cg_transform_c_type_to_g_type (const gchar *c_type,
                               gchar **g_type_prefix,
                               gchar **g_type_name)
{
	const gchar *default_prefix;
	const gchar *default_name;
	gchar *plain_c_type;
	gboolean result;
	size_t len;
	
	result = cg_transform_default_c_type_to_g_type (c_type, &default_prefix,
	                                                &default_name);

	if (result == TRUE)
	{
		*g_type_prefix = g_strdup (default_prefix);
		*g_type_name = g_strdup (default_name);
	}
	else
	{
		if (strncmp (c_type, "const ", 6) == 0)
			plain_c_type = g_strdup (c_type + 6);
		else
			plain_c_type = g_strdup (c_type);

		len = strlen (plain_c_type);
		if (plain_c_type[len - 1] == '*')
		{
			plain_c_type[len - 1] = '\0';
			g_strchomp (plain_c_type);
		}

		cg_transform_custom_c_type_to_g_type (plain_c_type, g_type_prefix,
		                                      g_type_name, NULL);

		g_free (plain_c_type);
	}
}

/* Looks up the given index in the hash table and removes enclosing quotes,
 * if any. Those are added again by the autogen template. */
void
cg_transform_string (GHashTable *table,
                     const gchar *index)
{
	gchar *str;
	gchar *unescaped;
	size_t len;

	str = g_hash_table_lookup (table, index);

	if (str != NULL)
	{
		len = strlen (str);
		if (len >= 2 && str[0] == '\"' && str[len - 1] == '\"')
		{
			/* Unescape string because it was most likely already escaped
			 * by the user because s/he also added quotes around it. */
			str = g_strndup (str + 1, len - 2);
			unescaped = g_strcompress (str);
			g_free (str);

			g_hash_table_insert (table, (gpointer) index, unescaped);
		}
	}
}

/* Looks up the given index in the hash table which is assumed to be a string
 * with '|'-separated abbrevations of the given flags. This function replaces
 * the abbrevations by the full names. If no flags are set, it produces "0". */
void
cg_transform_flags (GHashTable *table,
                    const gchar *index,
                    const CgElementEditorFlags *flags)
{
	const CgElementEditorFlags *flag;
	GString *res_str;
	gchar *flags_str;
	gchar *prev;
	gchar *pos;

	flags_str = g_hash_table_lookup (table, index);
	res_str = g_string_sized_new (128);

	if (flags_str != NULL)
	{
		prev = flags_str;
		pos = flags_str;
		
		while (*prev != '\0')
		{
			while (*pos != '|' && *pos != '\0')
				++ pos;

			flag = cg_transform_lookup_flag (flags, prev, pos - prev);
			g_assert (flag != NULL);
			
			if (res_str->len > 0) g_string_append (res_str, " | ");
			g_string_append (res_str, flag->name);

			if (*pos != '\0') ++ pos;
			prev = pos;
		}
	}
	
	if (res_str->len == 0) g_string_append_c (res_str, '0');
	g_hash_table_insert (table, (gpointer) index,
	                     g_string_free (res_str, FALSE));
}

/* Looks up the given param_index in the hash table. If it contains
 * guess_entry, the value is replaced by guessing the param spec from
 * the GObject type stored in type_index. */
void
cg_transform_guess_paramspec (GHashTable *table,
                              const gchar *param_index,
                              const gchar *type_index,
                              const gchar *guess_entry)
{
	const CgTransformParamGuess GUESS_TABLE[] =
	{
		{ "G_TYPE_BOOLEAN", "g_param_spec_boolean" },
		{ "G_TYPE_BOXED", "g_param_spec_boxed" },
		{ "G_TYPE_CHAR", "g_param_spec_char" },
		{ "G_TYPE_DOUBLE", "g_param_spec_double" },
		{ "G_TYPE_ENUM", "g_param_spec_enum" },
		{ "G_TYPE_FLAGS", "g_param_spec_flags" },
		{ "G_TYPE_FLOAT", "g_param_spec_float" },
		{ "G_TYPE_INT", "g_param_spec_int" },
		{ "G_TYPE_INT64", "g_param_spec_int64" },
		{ "G_TYPE_LONG", "g_param_spec_long" },
		{ "G_TYPE_OBJECT", "g_param_spec_object" },
		{ "G_TYPE_PARAM", "g_param_spec_param" },
		{ "G_TYPE_POINTER", "g_param_spec_pointer" },
		{ "G_TYPE_STRING", "g_param_spec_string" },
		{ "G_TYPE_UCHAR", "g_param_spec_uchar" },
		{ "G_TYPE_UINT", "g_param_spec_uint" },
		{ "G_TYPE_UINT64", "g_param_spec_uint64" },
		{ "G_TYPE_ULONG", "g_param_spec_ulong" },
		{ "G_TYPE_UNICHAR", "g_param_spec_unichar" },
		{ NULL, NULL }
	};

	const CgTransformParamGuess *guess;
	gchar *paramspec;
	gchar *type;
	
	paramspec = g_hash_table_lookup (table, param_index);

	if (paramspec != NULL && strcmp (paramspec, guess_entry) == 0)
	{
		type = g_hash_table_lookup (table, type_index);
		if (type != NULL)
		{
			for (guess = GUESS_TABLE; guess->gtype != NULL; ++ guess)
			{
				if (strcmp (type, guess->gtype) == 0)
				{
					paramspec = g_strdup (guess->paramspec);
					break;
				}
			}

			/* Not in list, so assume it is an object */
			if (guess->gtype == NULL)
				paramspec = g_strdup ("g_param_spec_object");
		
			g_hash_table_insert (table, (gpointer) param_index, paramspec);
		}
	}
}

/* This function looks up index in the given hash table and encloses it by
 * surrounding parenthesis if they do not already exist and the field is
 * non-empty. If make_void is TRUE and the arguments are only "()" it makes
 * "(void)" out of it to stay ANSI C compliant. */
void
cg_transform_arguments (GHashTable *table,
                        const gchar *index,
                        gboolean make_void)
{
	gchar *arg_res;
	gchar *arguments;
	size_t len;

	arguments = g_hash_table_lookup (table, index);
	
	if (arguments != NULL)
	{
		g_strstrip (arguments);
		len = strlen (arguments);

		/* Do nothing if the field was left empty */
		if (len > 0)
		{
			/* Surround arguments with paranthesis if they do
			 * not already exist. */
			if (arguments[0] != '(' && arguments[len - 1] != ')')
				arg_res = g_strdup_printf ("(%s)", arguments);
			else if (arguments[0] != '(')
				arg_res = g_strdup_printf ("(%s", arguments);
			else if (arguments[len - 1] != ')')
				arg_res = g_strdup_printf ("%s)", arguments);
			else
				arg_res = NULL;

			/* Set arguments to the transformed result, if a transformation
			 * has happend. */
			if (arg_res != NULL)
				arguments = arg_res;

			if (make_void == TRUE)
			{
				/* Make "(void)" out of "()" if make_void is set. If this is
				 * the case, we do not need to store arg_res in the hash
				 * table lateron, so we delete it right here. */
				if (strcmp (arguments, "()") == 0)
				{
					g_hash_table_insert (table, (gpointer) index,
					                     g_strdup ("(void)"));

					g_free (arg_res);
					arg_res = NULL;
				}
			}

			if (arg_res != NULL)
				g_hash_table_insert (table, (gpointer) index, arg_res);
		}
	}
}

/* This function makes a valid C identifier out of a string. It does this
 * by ignoring anything but digits, letters, hyphens and underscores. Digits
 * at the beginning of the string are also ignored. Hpyhens are transformed
 * to underscores. */
void
cg_transform_string_to_identifier (GHashTable *table,
                                   const gchar *string_index,
                                   const gchar *identifier_index)
{
	gchar *name;
	gchar *identifier_name;
	size_t name_len;
	size_t i, j;

	name = g_hash_table_lookup (table, "Name");
	if (name != NULL)
	{
		name_len = strlen (name);
		identifier_name = g_malloc ((name_len + 1) * sizeof(gchar));

		for (i = 0, j = 0; i < name_len; ++ i)
		{
			if (isupper (name[i]) || islower (name[i]))
				identifier_name[j ++] = name[i];
			else if (isdigit (name[i]) && j > 0)
				identifier_name[j ++] = name[i];
			else if (isspace (name[i]) || name[i] == '-' || name[i] == '_')
				identifier_name[j ++] = '_';
		}

		identifier_name[j] = '\0';

		g_hash_table_insert (table, (gpointer) identifier_index,
		                     identifier_name);

		/* Ownership is given to hash table, so no g_free here. */
	}
}

/* This function looks up the given index in the hash table and expects an
 * argument list like cg_transform_arguments generates it. It then checks
 * whether the first argument is of the given type. If it is, it does
 * nothing, otherwise it adds the first argument to the argument list
 * and writes the result back into the hash table. */
void
cg_transform_first_argument (GHashTable *table,
                             const gchar *index,
                             const gchar *type)
{
	gchar *arguments;
	guint pointer_count;
	guint arg_pointer_count;
	size_t type_len;
	const gchar *type_pos;
	gchar *arg_pos;
	gchar *pointer_str;
	gboolean arg_present;
	guint i;

	arguments = g_hash_table_lookup (table, index);

	/* Count the length of the basic type */
	type_len = 0;
	while (isalnum (type[type_len])) ++ type_len;
	type_pos = type + type_len;

	/* Count pointer indicators */
	pointer_count = 0;
	for (type_pos = type + type_len; *type_pos != '\0'; ++ type_pos)
		if (*type_pos == '*') ++ pointer_count;

	/* Build a string of all pointer indicators that we can append to the
	 * basic type to get the final type. */
	pointer_str = g_malloc ((pointer_count + 2) * sizeof(gchar));
	pointer_str[0] = ' ';
	pointer_str[pointer_count + 1] = '\0';
	for (i = 0; i < pointer_count; ++ i) pointer_str[i + 1] = '*';
	
	/* We do not just prepend type to the argument string because then
	 * we would have to fiddle around where spaces and pointer indicators
	 * belong. We can now always just build a string like
	 * BasicType+PointerStr+Name to get a fully qualified parameter like
	 * GtkTreeIter *iter, gint the_int or gchar **dest. */

	if (arguments == NULL || *arguments == '\0')
	{
		arguments = g_strdup_printf ("(%.*s%sself)", (int)type_len,
		                             type, pointer_str);

		g_hash_table_insert (table, (gpointer) index, arguments);
	}
	else
	{
		g_assert (arguments[0] == '(');

		/* Step over '(' and any leading whitespace */
		++ arguments;
		while (isspace (*arguments)) ++ arguments;

		arg_present = FALSE;
		if (strncmp (arguments, type, type_len) == 0)
		{
			/* We cannot just check (via string comparison) whether arguments
			 * begins with the whole type because the pointer indicator might
			 * be directly behind the basic type, but there might as well exist
			 * an arbitrary amount of spaces inbetween.
			 * GtkTreeIter* self vs. GtkTreeIter *self vs. 
			 * GtkTreeIter     *self. */
			arg_pointer_count = 0;
			for (arg_pos = arguments + type_len;
			     isspace (*arg_pos) || *arg_pos == '*';
			     ++ arg_pos)
			{
				if (*arg_pos == '*') ++ arg_pointer_count;
			}

			/* Type matches */
			if (arg_pointer_count == pointer_count)
				arg_present = TRUE;
		}

		/* The argument list does not contain the specified type as first
		 * argument, so add it. */
		if (arg_present == FALSE)
		{
			arguments = g_strdup_printf ("(%.*s%sself, %s", (int) type_len,
			                             type, pointer_str, arguments);
			g_hash_table_insert (table, (gpointer) index, arguments);
		}
	}
	
	g_free (pointer_str);
}

/* This function looks up the given arguments_index in the hash table and
 * expects an argument list with at least one argument like
 * cg_transform_first_argument generates it. Then, it makes a new
 * comma-separated list of the corresponding GTypes and writes it to
 * gtypes_index. It returns the amount of arguments. The first
 * argument is not part of the output.
 *
 * Additionally, if the arguments field was left empty, it makes (void) out
 * of it. Normally, the arguments field may be left empty to indicate that
 * the whole thing is a variable rather than a function. However, if someone
 * requires a list of gtypes of the arguments, it is most likely a function
 * and only a function. */
guint
cg_transform_arguments_to_gtypes (GHashTable *table,
                                  const gchar *arguments_index,
                                  const gchar *gtypes_index)
{
	guint arg_count;
	GString *arg_str;
	gchar *arguments;

	gchar *arg_prev;
	gchar *arg_pos;
	gchar *arg_type;
	
	gchar *argtype_prefix;
	gchar *argtype_suffix;

	arg_count = 0;
	arg_str = g_string_sized_new (128);
	arguments = g_hash_table_lookup (table, arguments_index);
	
	g_assert (arguments != NULL && *arguments != '\0');

	/* Step over '(' */
	arg_prev = arguments + 1;

	/* Ignore first argument */
	while (*arg_prev != ',' && *arg_prev != ')') ++ arg_prev;

	/* Step over ',' */
	if (*arg_prev == ',') ++ arg_prev;

	while (isspace (*arg_prev)) ++ arg_prev;
	arg_pos = arg_prev;

	while (*arg_prev != ')')
	{
		++ arg_count;

		/* Advance to end of this argument. */
		while (*arg_pos != ',' && *arg_pos != ')')
			++ arg_pos;

		/* Try to find argument type by going back the last identifier
		 * which should be the argument name. */
		if (arg_pos > arg_prev)
		{
			arg_type = arg_pos - 1;
			while (isspace (*arg_type)) -- arg_type;
		}

		while ((isalnum (*arg_type) || *arg_type == '_') &&
		      arg_type > arg_prev)
		{
			-- arg_type;
		}

		/* If the name is everything in this arguments this is most
		 * probably the type and the name has been omitted. If a type was
		 * given that ends on a special character (i.e. '*' because it is
		 * a pointer) we also want to get that character. */
		if (arg_type == arg_prev || !isspace (*arg_type))
			arg_type = arg_pos;

		/* Go back any whitespace to find end of type. Note that
		 * *(arg_type - 1) is always valid because arg_prev is at
		 * a character that is not a whitespace and arg_type is
		 * always >= arg_prev. */
		if (arg_type > arg_prev)
			while (isspace(*(arg_type - 1)))
				-- arg_type;

		/* The arguments type should now be enclosed by arg_prev and
		 * arg_type. */
		arg_type = g_strndup (arg_prev, arg_type - arg_prev);
		cg_transform_c_type_to_g_type (arg_type, &argtype_prefix,
		                               &argtype_suffix);
		g_free (arg_type);
		
		if (arg_str->len > 0) g_string_append (arg_str, ", ");
		g_string_append (arg_str, argtype_prefix);
		g_string_append (arg_str, "_TYPE_");
		g_string_append (arg_str, argtype_suffix);
		
		g_free (argtype_prefix);
		g_free (argtype_suffix);

		if (*arg_pos != ')')
		{
			/* Step over comma and following whitespace */
			++ arg_pos;
			while (isspace (*arg_pos)) ++ arg_pos;
		}

		arg_prev = arg_pos;
	}

	g_hash_table_insert (table, (gpointer) gtypes_index,
	                     g_string_free (arg_str, FALSE));

	return arg_count;
}
