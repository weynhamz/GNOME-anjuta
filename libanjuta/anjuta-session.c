/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-session.c
 * Copyright (c) 2005 Naba Kumar  <naba@gnome.org>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:anjuta-session
 * @short_description: Program session
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-session.h
 * 
 */

#include <stdlib.h>
#include <string.h>

#include "anjuta-session.h"
#include "anjuta-utils.h"
 
struct _AnjutaSessionPriv {
	gchar *dir_path;
	GKeyFile *key_file;
};

static gpointer *parent_class = NULL;

static void
anjuta_session_finalize (GObject *object)
{
	AnjutaSession *cobj;
	cobj = ANJUTA_SESSION (object);
	
	g_free (cobj->priv->dir_path);
	g_key_file_free (cobj->priv->key_file);
	g_free (cobj->priv);

	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
anjuta_session_class_init (AnjutaSessionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = anjuta_session_finalize;
}

static void
anjuta_session_instance_init (AnjutaSession *obj)
{
	obj->priv = g_new0 (AnjutaSessionPriv, 1);
	obj->priv->dir_path = NULL;
}

/**
 * anjuta_session_new:
 * @session_directory: Directory where session is loaded from/saved to.
 * 
 * Created a new session object. @session_directory is the directory
 * where session information will be stored or loaded in case of existing
 * session.
 * 
 * Returns: an #AnjutaSession Object
 */
AnjutaSession*
anjuta_session_new (const gchar *session_directory)
{
	AnjutaSession *obj;
	gchar *filename;
	
	g_return_val_if_fail (session_directory != NULL, NULL);
	g_return_val_if_fail (g_path_is_absolute (session_directory), NULL);
	
	obj = ANJUTA_SESSION (g_object_new (ANJUTA_TYPE_SESSION, NULL));
	obj->priv->dir_path = g_strdup (session_directory);
	
	obj->priv->key_file = g_key_file_new ();
	
	filename = anjuta_session_get_session_filename (obj);
	g_key_file_load_from_file (obj->priv->key_file, filename,
							   G_KEY_FILE_NONE, NULL);
	g_free (filename);
	 
	return obj;
}

ANJUTA_TYPE_BOILERPLATE (AnjutaSession, anjuta_session, G_TYPE_OBJECT)

/**
 * anjuta_session_get_session_directory:
 * @session: an #AnjutaSession object
 * 
 * Returns the directory corresponding to this session object.
 * 
 * Returns: session directory
 */
const gchar*
anjuta_session_get_session_directory (AnjutaSession *session)
{
	return session->priv->dir_path;
}

/**
 * anjuta_session_get_session_filename:
 * @session: an #AnjutaSession object
 * 
 * Gets the session filename corresponding to this session object.
 * 
 * Returns: session (absolute) filename
 */
gchar*
anjuta_session_get_session_filename (AnjutaSession *session)
{
	g_return_val_if_fail (ANJUTA_IS_SESSION (session), NULL);
	
	return g_build_filename (session->priv->dir_path,
							 "anjuta.session", NULL);
}

/**
 * anjuta_session_sync:
 * @session: an #AnjutaSession object
 * 
 * Synchronizes session object with session file
 */
void
anjuta_session_sync (AnjutaSession *session)
{
	gchar *filename, *data;
	
	g_return_if_fail (ANJUTA_IS_SESSION (session));
	
	filename = anjuta_session_get_session_filename (session);
	data = g_key_file_to_data (session->priv->key_file, NULL, NULL);
	g_file_set_contents (filename, data, -1, NULL);
	
	g_free (filename);
	g_free (data);
}

/**
 * anjuta_session_clear:
 * @session: an #AnjutaSession object
 * 
 * Clears the session.
 */
void
anjuta_session_clear (AnjutaSession *session)
{
	gchar *cmd;
	gint ret;
	
	g_return_if_fail (ANJUTA_IS_SESSION (session));
	
	g_key_file_free (session->priv->key_file);
	session->priv->key_file = g_key_file_new ();
	
	anjuta_session_sync (session);
	
	cmd = g_strconcat ("mkdir -p ", session->priv->dir_path, NULL);
	ret = system (cmd);
	g_free (cmd);
	
	cmd = g_strconcat ("rm -fr ", session->priv->dir_path, "/*", NULL);
	ret = system (cmd);
	g_free (cmd);
}

/**
 * anjuta_session_clear_section:
 * @session: an #AnjutaSession object.
 * @section: Section to clear.
 * 
 * Clears the given section in session object.
 */
void
anjuta_session_clear_section (AnjutaSession *session,
							  const gchar *section)
{	
	g_return_if_fail (ANJUTA_IS_SESSION (session));
	g_return_if_fail (section != NULL);

	g_key_file_remove_group (session->priv->key_file, section, NULL);
}
 
/**
 * anjuta_session_set_int:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * @value: Key value
 * 
 * Set an integer @value to @key in given @section.
 */
void
anjuta_session_set_int (AnjutaSession *session, const gchar *section,
						const gchar *key, gint value)
{
	g_return_if_fail (ANJUTA_IS_SESSION (session));
	g_return_if_fail (section != NULL);
	g_return_if_fail (key != NULL);
	
	if (!value)
	{
		g_key_file_remove_key (session->priv->key_file, section, key, NULL);
		return;
	}
	
	g_key_file_set_integer (session->priv->key_file, section, key, value);
}

/**
 * anjuta_session_set_float:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * @value: Key value
 * 
 * Set a float @value to @key in given @section.
 */
void
anjuta_session_set_float (AnjutaSession *session, const gchar *section,
						  const gchar *key, gfloat value)
{
	g_return_if_fail (ANJUTA_IS_SESSION (session));
	g_return_if_fail (section != NULL);
	g_return_if_fail (key != NULL);
	
	if (!value)
	{
		g_key_file_remove_key (session->priv->key_file, section, key, NULL);
		return;
	}
	
	g_key_file_set_double (session->priv->key_file, section, key, value);
}

/**
 * anjuta_session_set_string:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * @value: Key value
 * 
 * Set a string @value to @key in given @section.
 */
void
anjuta_session_set_string (AnjutaSession *session, const gchar *section,
						   const gchar *key, const gchar *value)
{
	g_return_if_fail (ANJUTA_IS_SESSION (session));
	g_return_if_fail (section != NULL);
	g_return_if_fail (key != NULL);
	
	if (!value)
	{
		g_key_file_remove_key (session->priv->key_file, section, key, NULL);
		return;
	}
	
	g_key_file_set_string (session->priv->key_file, section, key, value);
}

/**
 * anjuta_session_set_string_list:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * @value: Key value
 * 
 * Set a list of strings @value to @key in given @section.
 */
void
anjuta_session_set_string_list (AnjutaSession *session,
								const gchar *section,
								const gchar *key, GList *value)
{
	gchar *value_str;
	GString *str;
	GList *node;
	gboolean first_item = TRUE;
	
	g_return_if_fail (ANJUTA_IS_SESSION (session));
	g_return_if_fail (section != NULL);
	g_return_if_fail (key != NULL);
	
	if (!value)
	{
		g_key_file_remove_key (session->priv->key_file, section, key, NULL);
		return;
	}
	
	str = g_string_new ("");
	node = value;
	while (node)
	{
		if (node->data && strlen (node->data) > 0)
		{
			if (first_item)
				first_item = FALSE;
			else
				g_string_append (str, "%%%");
			g_string_append (str, node->data);
		}
		node = g_list_next (node);
	}
	
	value_str = g_string_free (str, FALSE);
	g_key_file_set_string (session->priv->key_file, section, key, value_str);
	
	g_free (value_str);
}

/**
 * anjuta_session_get_int:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * 
 * Get an integer @value of @key in given @section.
 * 
 * Returns: Key value
 */
gint
anjuta_session_get_int (AnjutaSession *session, const gchar *section,
						const gchar *key)
{
	gint value;
	
	g_return_val_if_fail (ANJUTA_IS_SESSION (session), 0);
	g_return_val_if_fail (section != NULL, 0);
	g_return_val_if_fail (key != NULL, 0);
	
	value = g_key_file_get_integer (session->priv->key_file, section, key, NULL);
	
	return value;
}

/**
 * anjuta_session_get_float:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * 
 * Get a float @value of @key in given @section.
 * 
 * Returns: Key value
 */
gfloat
anjuta_session_get_float (AnjutaSession *session, const gchar *section,
						  const gchar *key)
{
	gfloat value;
	
	g_return_val_if_fail (ANJUTA_IS_SESSION (session), 0);
	g_return_val_if_fail (section != NULL, 0);
	g_return_val_if_fail (key != NULL, 0);
	
	value = (float)g_key_file_get_double (session->priv->key_file, section, key, NULL);
	
	return value;
}

/**
 * anjuta_session_get_string:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * 
 * Get a string @value of @key in given @section.
 * 
 * Returns: Key value
 */
gchar*
anjuta_session_get_string (AnjutaSession *session, const gchar *section,
						   const gchar *key)
{
	gchar *value;
	
	g_return_val_if_fail (ANJUTA_IS_SESSION (session), NULL);
	g_return_val_if_fail (section != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);
	
	value = g_key_file_get_string (session->priv->key_file, section, key, NULL);
	
	return value;
}

/**
 * anjuta_session_get_string_list:
 * @session: an #AnjutaSession object
 * @section: Section.
 * @key: Key name.
 * 
 * Get a list of strings @value of @key in given @section.
 * 
 * Returns: Key value
 */
GList*
anjuta_session_get_string_list (AnjutaSession *session,
								const gchar *section,
								const gchar *key)
{
	gchar *val, **str, **ptr;
	GList *value;
	
	g_return_val_if_fail (ANJUTA_IS_SESSION (session), NULL);
	g_return_val_if_fail (section != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);
	
	val = g_key_file_get_string (session->priv->key_file, section, key, NULL);
	
	
	value = NULL;
	if (val)
	{
		str = g_strsplit (val, "%%%", -1);
		if (str)
		{
			ptr = str;
			while (*ptr)
			{
				if (strlen (*ptr) > 0)
					value = g_list_prepend (value, g_strdup (*ptr));
				ptr++;
			}
			g_strfreev (str);
		}
		g_free (val);
	}
	
	return g_list_reverse (value);
}
