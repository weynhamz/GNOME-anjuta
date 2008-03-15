/*
 *  macro_db.c (c) 2005 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "macro-db.h"
#include "macro-util.h"
#include <libxml/parser.h>
#include <libgnomevfs/gnome-vfs.h>
#include <stdlib.h>
#include <libanjuta/anjuta-debug.h>

#define PREDEFINED_MACRO_FILE PACKAGE_DATA_DIR"/macros.xml"

static gchar *
get_user_macro_path ()
{
	return g_strconcat (getenv ("HOME"), "/.anjuta/macros.xml", NULL);
}
				   
static gboolean
parse_xml_file (xmlDocPtr * doc, xmlNodePtr * cur, const gchar * filename)
{
	*doc = xmlParseFile (filename);

	if (*doc == NULL)
	{
		return FALSE;
	}
	*cur = xmlDocGetRootElement (*doc);

	if (*cur == NULL)
	{
		xmlFreeDoc (*doc);
		return FALSE;
	}

	if (xmlStrcmp ((*cur)->name, (const xmlChar *) "anjuta-macros"))
	{
		xmlFreeDoc (*doc);
		return FALSE;
	}
	return TRUE;
}

static GtkTreeIter *
find_category (GtkTreeStore * tree_store, GtkTreeIter * parent,
	       const gchar * category)
{
	GtkTreeIter *cat_item = g_new0 (GtkTreeIter, 1);
	if (!strlen (category))
	{
		return parent;
	}
	else if (gtk_tree_model_iter_children (GTK_TREE_MODEL (tree_store),
					       cat_item, parent))
	{
		do
		{
			gboolean is_category;
			gchar *cat_name;
			gtk_tree_model_get (GTK_TREE_MODEL (tree_store),
					    cat_item, MACRO_IS_CATEGORY,
					    &is_category, MACRO_NAME,
					    &cat_name, -1);
			if (is_category && !strcmp (cat_name, category))
			{
				return cat_item;
			}
		}
		while (gtk_tree_model_iter_next
		       (GTK_TREE_MODEL (tree_store), cat_item));
	}
	gtk_tree_store_append (tree_store, cat_item, parent);
	gtk_tree_store_set (tree_store, cat_item,
			    MACRO_NAME, category,
			    MACRO_IS_CATEGORY, TRUE, -1);
	return cat_item;
}

static void
macro_db_add_real (GtkTreeStore * tree_store,
		   GtkTreeIter * parent,
		   const gchar * name,
		   const gchar * category,
		   const gchar * shortcut,
		   const gchar * text, gboolean pre_defined)
{
	gchar c_shortcut;
	GtkTreeIter *cat_item;
	GtkTreeIter new_item;
	g_return_if_fail (tree_store != NULL);
	if (shortcut != NULL && strlen (shortcut))
		c_shortcut = shortcut[0];
	else
		c_shortcut = 0;
	if (category == NULL)
		category = "";
	if (name && category && text)
	{
		cat_item = find_category (tree_store, parent, category);
		gtk_tree_store_append (tree_store, &new_item, cat_item);
		gtk_tree_store_set (tree_store, &new_item,
				    MACRO_NAME, name,
				    MACRO_CATEGORY, category,
				    MACRO_SHORTCUT, c_shortcut,
				    MACRO_TEXT, text,
				    MACRO_PREDEFINED, pre_defined,
				    MACRO_IS_CATEGORY, FALSE, -1);
	}
}

static void
read_macros (xmlDocPtr doc, xmlNodePtr cur, GtkTreeStore * tree_store,
	     GtkTreeIter * iter, gboolean pre_defined)
{
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if ((!xmlStrcmp (cur->name, (const xmlChar *) "macro")))
		{
			xmlChar *name;
			xmlChar *category;
			xmlChar *shortcut;
			xmlChar *text;
			
			name = xmlGetProp(cur, (const xmlChar *)"_name");
			category = xmlGetProp(cur, (const xmlChar *)"_category");
			shortcut = xmlGetProp(cur, (const xmlChar *)"_shortcut");
			text = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
			
			macro_db_add_real (tree_store, iter, (const gchar *)name,
					   (const gchar *)category, (const gchar *)shortcut, (const gchar *)text,
					   pre_defined);
			xmlFree(name);
			xmlFree(category);
			xmlFree(shortcut);
			xmlFree(text);
		}
		cur = cur->next;
	}
}

static void
fill_predefined (GtkTreeStore * tree_store, GtkTreeIter * iter_pre)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	if (parse_xml_file (&doc, &cur, PREDEFINED_MACRO_FILE))
		read_macros (doc, cur, tree_store, iter_pre, TRUE);
	else
		DEBUG_PRINT ("Could not read predefined macros!");
}

static void
fill_userdefined (GtkTreeStore * tree_store, GtkTreeIter * iter_user)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	gchar *user_file = get_user_macro_path ();
	if (parse_xml_file (&doc, &cur, user_file))
		read_macros (doc, cur, tree_store, iter_user, FALSE);
	else
		DEBUG_PRINT ("Could not read predefined macros!");
	g_free (user_file);
}

static void
save_macro (GtkTreeModel * model, GtkTreeIter * iter, GnomeVFSHandle * handle)
{
	GnomeVFSFileSize bytes, bytes_written;
	GnomeVFSResult result;
	gchar *name;
	gchar *category;
	gchar shortcut;
	gchar *shortcut_string;
	gchar *text;
	gchar *output;
	gtk_tree_model_get (model, iter,
			    MACRO_NAME, &name,
			    MACRO_CATEGORY, &category,
			    MACRO_SHORTCUT, &shortcut, MACRO_TEXT, &text, -1);
	shortcut_string = g_strdup_printf ("%c", shortcut);
	output = g_strdup_printf ("<macro _name=\"%s\" _category=\"%s\" "
								"_shortcut=\"%s\">"
								"<![CDATA[%s]]></macro>\n",
				  				name, category, shortcut_string, text);
	g_free (shortcut_string);
	bytes = strlen (output);
	result = gnome_vfs_write (handle, output,
				  strlen (output), &bytes_written);
	g_free (name);
	g_free (category);
	g_free (text);
	if (result != GNOME_VFS_OK)
		return;
}

static gpointer parent_class;

static void
macro_db_dispose (GObject * db)
{
	DEBUG_PRINT ("Disposing MacroDB");
	macro_db_save (MACRO_DB (db));
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
macro_db_finalize (GObject * db)
{
	DEBUG_PRINT ("Disposing MacroDB");
	macro_db_save (MACRO_DB (db));
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
macro_db_class_init (MacroDBClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = macro_db_dispose;
	object_class->finalize = macro_db_finalize;
}

static void
macro_db_init (MacroDB * db)
{
	db->tree_store = gtk_tree_store_new (MACRO_N_COLUMNS,
					     G_TYPE_STRING,
					     G_TYPE_STRING,
					     G_TYPE_CHAR,
					     G_TYPE_STRING,
					     G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

	gtk_tree_store_append (db->tree_store, &db->iter_pre, NULL);
	gtk_tree_store_append (db->tree_store, &db->iter_user, NULL);
	gtk_tree_store_set (db->tree_store, &db->iter_pre,
			    MACRO_NAME, _("Anjuta macros"),
			    MACRO_IS_CATEGORY, TRUE,
			    MACRO_PREDEFINED, TRUE, -1);
	gtk_tree_store_set (db->tree_store, &db->iter_user,
			    MACRO_NAME, _("My macros"),
			    MACRO_IS_CATEGORY, TRUE,
			    MACRO_PREDEFINED, TRUE, -1);
	fill_predefined (db->tree_store, &db->iter_pre);
	fill_userdefined (db->tree_store, &db->iter_user);
}

GType
macro_db_get_type (void)
{
	static GType macro_db_type = 0;
	if (!macro_db_type)
	{
		static const GTypeInfo db_info = {
			sizeof (MacroDBClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) macro_db_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (MacroDB),
			0,
			(GInstanceInitFunc) macro_db_init,
		};
		macro_db_type =
			g_type_register_static (G_TYPE_OBJECT, "MacroDB",
						&db_info, 0);
	}
	return macro_db_type;
}

MacroDB *
macro_db_new ()
{
	return MACRO_DB (g_object_new (macro_db_get_type (), NULL));
}

void
macro_db_save (MacroDB * db)
{
	GtkTreeIter cur_cat;
	GtkTreeModel *model;
	GnomeVFSHandle *handle;
	GnomeVFSResult result;
	GnomeVFSFileSize bytes_written;
	const gchar* header = "<?xml version=\"1.0\" " 
		"encoding=\"UTF-8\"?>\n";
	const gchar *begin = "<anjuta-macros>\n";
	const gchar *end = "</anjuta-macros>\n";

	g_return_if_fail (db != NULL);

	gchar *user_file = get_user_macro_path ();
	result = gnome_vfs_create (&handle, user_file, GNOME_VFS_OPEN_WRITE,
				   FALSE, 0777);
	g_free (user_file);
	if (result != GNOME_VFS_OK)
		return;


	result = gnome_vfs_write (handle, header, strlen (header),
				  &bytes_written);
	if (result != GNOME_VFS_OK)
		return;
	result = gnome_vfs_write (handle, begin, strlen (begin),
				  &bytes_written);
	if (result != GNOME_VFS_OK)
		return;
	model = GTK_TREE_MODEL (db->tree_store);
	if (gtk_tree_model_iter_children (model, &cur_cat, &db->iter_user))
	{
		do
		{
			GtkTreeIter cur_macro;
			if (gtk_tree_model_iter_children
			    (model, &cur_macro, &cur_cat))
			{
				do
				{
					save_macro (model, &cur_macro,
						    handle);
				}
				while (gtk_tree_model_iter_next
				       (model, &cur_macro));
			}
			else
			{
				gboolean is_category;
				gtk_tree_model_get (model, &cur_cat,
						    MACRO_IS_CATEGORY,
						    &is_category, -1);
				if (!is_category)
					save_macro (model, &cur_cat, handle);
			}
		}
		while (gtk_tree_model_iter_next (model, &cur_cat));
	}
	result = gnome_vfs_write (handle, end, strlen (end), &bytes_written);
	if (result != GNOME_VFS_OK)
		return;
	gnome_vfs_close (handle);
}

void
macro_db_add (MacroDB * db,
	      const gchar * name,
	      const gchar * category,
	      const gchar * shortcut, const gchar * text)
{
	GtkTreeIter iter;
	g_return_if_fail (db != NULL);
	/* Set to userdefined macro root */
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (db->tree_store),
				       &iter);
	gtk_tree_model_iter_next (GTK_TREE_MODEL (db->tree_store), &iter);
	macro_db_add_real (GTK_TREE_STORE (db->tree_store), &iter, name,
			   category, shortcut, text, FALSE);
   	macro_db_save(db);
}

void
macro_db_change (MacroDB * db, GtkTreeIter * iter,
		 const gchar * name,
		 const gchar * category,
		 const gchar * shortcut, const gchar * text)
{
	g_return_if_fail (db != NULL);

	macro_db_remove (db, iter);
	macro_db_add (db, name, category, shortcut, text);
	macro_db_save(db);
}

void
macro_db_remove (MacroDB * db, GtkTreeIter * iter)
{
	GtkTreePath *path;
	GtkTreeIter parent;
	g_return_if_fail (db != NULL);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (db->tree_store),
					iter);
	gtk_tree_store_remove (db->tree_store, iter);
	if (!gtk_tree_path_up (path))
		return;
	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (db->tree_store),
				      &parent, path))
		return;
	if (!gtk_tree_model_iter_has_child
	    (GTK_TREE_MODEL (db->tree_store), &parent))
	{
		gboolean is_root;
		gtk_tree_model_get (GTK_TREE_MODEL (db->tree_store), &parent,
				    MACRO_PREDEFINED, &is_root, -1);
		if (!is_root)
			gtk_tree_store_remove (db->tree_store, &parent);
	}
	gtk_tree_path_free (path);
	macro_db_save(db);
}

inline GtkTreeModel *
macro_db_get_model (MacroDB * db)
{
	g_return_val_if_fail (db != NULL, NULL);
	return GTK_TREE_MODEL (db->tree_store);
}

gchar* macro_db_get_macro(MacroPlugin * plugin, MacroDB * db, GtkTreeIter* iter,
                          gint *offset)
{
	g_return_val_if_fail (db != NULL, NULL);
	g_return_val_if_fail (iter != NULL, NULL);
		
	gchar *text;
	gboolean is_category;
	gtk_tree_model_get (macro_db_get_model(db), iter,
						MACRO_TEXT, &text,
						MACRO_IS_CATEGORY, &is_category, -1);
	if (is_category)
		return NULL;
	else
	{
		gchar* buffer = expand_macro(plugin, text, offset);
		g_free (text);
		return buffer;
	}
}
