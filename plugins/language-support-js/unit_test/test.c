#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "../ijs-symbol.h"
#include "../database-symbol.h"
#include "../util.h"
#include "../plugin.h"

DatabaseSymbol *symdb = NULL;

struct TestCase1
{
	const gchar *filename;
	const gchar *var;
	const gchar *res;
};

struct TestCase2
{
	const gchar *filename;
	const gchar *res[100];
};

struct TestCase3
{
	const gchar *filename;
	const gchar *var;
	const gchar *res[100];
};

struct TestCase4
{
	const gchar *filename;
	gint line;
	const gchar *res[100];
};

#define TEST1_COL 5

struct TestCase1 tests[] = {
		{"./u1.js", "m", "Parenizor"},
		{"./u1.js", "GLib", "imports.gi.GLib"},
		{"./u1.js", "a", "String"},
		{"./u3.js", "b", "Button"},
		{"./u5.js", "a", "Number"}
};

#define TEST2_COL 2

struct TestCase2 tests2[] = {
		{"./u2.js", {"Function.prototype.method", "Parenizor", "mazx", "aasb", "GLib", NULL}},
		{"./u3.js", {"Button", "Button.prototype", "a", "b", NULL}}
};

#define TEST3_COL 1

struct TestCase3 tests3[] = {
		{"./u3.js", "b", {"_init", "release", NULL}}
};

#define TEST4_COL 3

struct TestCase4 tests4[] = {
		{"./u4.js", 8, {"m", "a", "GLib", "var1", NULL}},
		{"./u4.js", 9, {"m", "a", "GLib", NULL}},
		{"./u6.js", 4, {"abc", "max", NULL}},
		{"./u6.js", 7, {"abc", NULL}},
};

static void
var_type_test (gconstpointer data)
{
	gint n = GPOINTER_TO_INT (data);

	database_symbol_set_file (symdb, tests[n].filename);

	IJsSymbol *t = ijs_symbol_get_member (IJS_SYMBOL (symdb), tests[n].var);

	g_assert (t);
	const gchar *name;

	name = ijs_symbol_get_name (t);
//	printf ("\n%s == %s\n", name, tests[n].res);

	g_assert (strcmp (name, tests[n].res) == 0);
}

static void
var_list_test (gconstpointer data)
{
	gint n = GPOINTER_TO_INT (data);

	database_symbol_set_file (symdb, tests2[n].filename);

	IJsSymbol *sym = ijs_symbol_get_member (IJS_SYMBOL (symdb), "");

	g_assert (sym);

	GList *res = ijs_symbol_list_member (sym);
	GList *i;
	const gchar **k = tests2[n].res;
	for (i = res; i; i = g_list_next (i), k++)
	{
		g_assert ( *k != NULL);
//		printf ("\n%s == %s\n", *k, i->data);
		g_assert ( strcmp (*k, (gchar*)i->data) == 0);
	}
	g_assert ( *k == NULL);
}

static void
var_list2_test (gconstpointer data)
{
	gint n = GPOINTER_TO_INT (data);

	database_symbol_set_file (symdb, tests4[n].filename);

	GList *res = database_symbol_list_local_member (symdb, tests4[n].line);
	GList *i;
	const gchar **k = tests4[n].res;
	for (i = res; i; i = g_list_next (i), k++)
	{
//puts ((gchar*)i->data);
		g_assert ( *k != NULL);
//		printf ("\n%s == %s\n", *k, i->data);
		g_assert ( strcmp (*k, (gchar*)i->data) == 0);
	}
	g_assert ( *k == NULL);
}

static void
var_list_member_test (gconstpointer data)
{
	gint n = GPOINTER_TO_INT (data);

	database_symbol_set_file (symdb, tests3[n].filename);

	IJsSymbol *t = ijs_symbol_get_member (IJS_SYMBOL (symdb), tests3[n].var);

	g_assert (t);

	GList *res = ijs_symbol_list_member (t);

	GList *i;
	const gchar **k = tests3[n].res;
	for (i = res; i; i = g_list_next (i), k++)
	{
		g_assert ( *k != NULL);
//printf ("\n%s == %s\n", *k, i->data);
		g_assert ( strcmp (*k, (gchar*)i->data) == 0);
	}
	g_assert ( *k == NULL);
}

int
main (int argc, char *argv[])
{
	gint i;

	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	JSLang *lang = g_new (JSLang, 1);
	symdb = database_symbol_new ();
	lang->symbol = IJS_SYMBOL (symdb);

	setPlugin (lang);

	for (i = 0; i < TEST1_COL; i++)
		g_test_add_data_func ("/parser/var_type", GINT_TO_POINTER (i), var_type_test);
	for (i = 0; i < TEST2_COL; i++)
		g_test_add_data_func ("/parser/var_list", GINT_TO_POINTER (i), var_list_test);
	for (i = 0; i < TEST3_COL; i++)
		g_test_add_data_func ("/parser/list_member", GINT_TO_POINTER (i), var_list_member_test);
	for (i = 0; i < TEST4_COL; i++)
		g_test_add_data_func ("/parser/var_list2", GINT_TO_POINTER (i), var_list2_test);
	return g_test_run();
}
