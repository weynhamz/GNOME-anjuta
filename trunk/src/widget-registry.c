#include <gnome.h>

#include "widget-registry.h"

static GHashTable *registry = NULL;

static GtkWidget *default_submenu = NULL;

void an_register_submenu(const gchar *name, GtkWidget *submenu)
{
	g_return_if_fail(submenu);

	if (NULL == registry)
		registry = g_hash_table_new(g_str_hash, g_str_equal);
	if (NULL == name)
		default_submenu = submenu;
	else
	{
		gchar *lname = g_strdup(name);
		g_strdown(lname);
		if (NULL == g_hash_table_lookup(registry, lname))
			g_hash_table_insert(registry, (gpointer) lname, submenu);
		else
			g_warning("an_register_submenu: %s is already registered"
			  , name);
	}
}

GtkWidget *an_get_submenu(const gchar *name)
{
	GtkWidget *submenu;

	if (NULL == name)
		return default_submenu;
	else
	{
		gchar *lname = g_strdup(name);
		g_strdown(lname);
		submenu = g_hash_table_lookup(registry, lname);
		g_free(lname);
		if (NULL == submenu)
			submenu = default_submenu;
		return submenu;
	}
}
