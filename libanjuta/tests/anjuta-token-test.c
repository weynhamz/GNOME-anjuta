/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * test-token.c
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

#include "config.h"

#include "libanjuta/anjuta-token.h"
#include "libanjuta/anjuta-debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


/* Check token functions
 *---------------------------------------------------------------------------*/

int
main(int argc, char *argv[])
{
	AnjutaToken *list;
	AnjutaToken *token;
	gchar *value;
	gboolean ok;

	/* Initialize program */
	g_type_init ();

	anjuta_debug_init ();

	ok = TRUE;

	value = anjuta_token_evaluate (NULL);
	ok = ok && (value == NULL);
	fprintf(stdout, "%s %d\n", value == NULL ? "(NULL)" : value, ok);
	value = anjuta_token_evaluate_name (NULL);
	ok = ok && (value == NULL);
	fprintf(stdout, "%s %d\n", value == NULL ? "(NULL)" : value, ok);

	list = anjuta_token_new_static (ANJUTA_TOKEN_NAME, "tip");
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "tip") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tip") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	token = anjuta_token_insert_after (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "top"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "tip") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tip") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_merge (list, anjuta_token_next (list));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "tiptop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tiptop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_insert_after (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "bap"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "tiptop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tiptop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_merge (token, anjuta_token_next (token));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "tiptopbap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tiptopbap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_append_child (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "tup"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "tup") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tiptopbap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_append_child (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "tap"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "tuptap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tiptopbap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	list = anjuta_token_insert_before (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "bip"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "bip") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "bip") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_merge (list, anjuta_token_next (list));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "biptuptap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "biptiptopbap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_insert_after (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "bop"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "biptuptap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "biptiptopbap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_merge (list, anjuta_token_next (anjuta_token_last (list)));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "biptuptapbop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "biptiptopbapbop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	token = anjuta_token_insert_after (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "bup"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "biptuptapbop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "biptiptopbapbop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_merge (anjuta_token_last (list), token);
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "biptuptapbopbup") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "biptiptopbapbopbup") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	anjuta_token_append_child (anjuta_token_last (list), anjuta_token_new_static (ANJUTA_TOKEN_NAME, "pap"));
	value = anjuta_token_evaluate (list);
	ok = ok && (strcmp(value, "biptuptapbop") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "biptiptopbapbopbup") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	// Check bug in anjuta_token_merge
	list = anjuta_token_new_static (ANJUTA_TOKEN_NAME, "tip");
	anjuta_token_merge (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "tup"));
	anjuta_token_merge (list, anjuta_token_new_static (ANJUTA_TOKEN_NAME, "tap"));
	value = anjuta_token_evaluate_name (list);
	ok = ok && (strcmp(value, "tiptuptap") == 0);
	fprintf(stdout, "%s %d\n", value, ok);
	g_free (value);

	return ok ? 0 : 1;
}
