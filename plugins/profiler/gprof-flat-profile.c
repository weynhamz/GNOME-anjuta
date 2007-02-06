/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-flat-profile.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-flat-profile.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#include "gprof-flat-profile.h"

struct _GProfFlatProfilePriv
{
	GList *entries;  			/* List of all entries */
	GHashTable *lookup_table;	/* Find entries by name */
};

static void
gprof_flat_profile_init (GProfFlatProfile *self)
{
	self->priv = g_new0 (GProfFlatProfilePriv, 1);
	self->priv->lookup_table = g_hash_table_new (g_str_hash, g_str_equal);
}

static void 
gprof_flat_profile_finalize (GObject *obj)
{
	GProfFlatProfile *self;
	GList *current;
	
	self = (GProfFlatProfile *) obj;
	
	g_hash_table_destroy (self->priv->lookup_table);
	
	current = self->priv->entries;
	
	while (current)
	{
		gprof_flat_profile_entry_free (current->data);
		current = g_list_next (current);
	}
	
	g_list_free (self->priv->entries);
	g_free (self->priv);
}

static void 
gprof_flat_profile_class_init (GProfFlatProfileClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	object_class->finalize = gprof_flat_profile_finalize;
}

static gchar **
get_flat_profile_fields (gchar *buffer)
{
	gchar **string_table;
	gchar *calls_field;  /* Pointer to string that begins at the calls field */
	gint i;
	gint pos; /* Where are we in the buffer? */
	
	string_table = g_new0 (gchar *, 8);  /* NULL terminated */
	
	/* Get the first 3 fields */
	pos = 0;
	
	for (i = 0; i < 3; i++)
		string_table[i] = read_to_whitespace (&buffer[pos], &pos, pos);
	
	/* In some cases, uncalled functions may have empty calls, self ms/call, and
	 * total ms/call fields. */
	
	/* If the next field begins with a digit, we have the fields */
	calls_field = strip_whitespace (&buffer[pos]);
	
	if (g_ascii_isdigit (calls_field[0]))
	{	
		for (i = 3; i < 6; i++)
			string_table[i] = read_to_whitespace (&buffer[pos], &pos, pos);
		
		string_table[6] = strip_whitespace (&buffer[pos]);
	}
	else /* We don't have the fields; calls_field points to function name */
	{
		for (i = 3; i < 6; i++)
			string_table[i] = g_strdup ("0");
		
		string_table[6] = g_strdup (calls_field);
	}
	
	g_free (calls_field);
	
	return string_table;
}

static void
gprof_flat_profile_add_entry (GProfFlatProfile *self, 
							  GProfFlatProfileEntry *entry)
{
	self->priv->entries = g_list_append (self->priv->entries, entry);
	g_hash_table_insert (self->priv->lookup_table,
						 gprof_flat_profile_entry_get_name (entry),
						 entry);
}

GType
gprof_flat_profile_get_type ()
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfFlatProfileClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_flat_profile_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfFlatProfile),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_flat_profile_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfFlatProfile", &obj_info, 0);
	}
	return obj_type;
}

GProfFlatProfile *
gprof_flat_profile_new (FILE *stream)
{
	gchar buffer[PATH_MAX];
	size_t length;
	gchar **fields;
	GProfFlatProfile *flat_profile;
	
	flat_profile = g_object_new (GPROF_FLAT_PROFILE_TYPE, NULL);
	
	/* Read to beginning of flat profile data */
	do
	{
		/* Don't loop infinitely if we don't have any data */
		if (!fgets (buffer, PATH_MAX, stream))
			return flat_profile;
			
	} while (!strchr (buffer, '%'));
	
	/* Skip the second line of the column header */
	fgets (buffer, PATH_MAX, stream);
	
	while (fgets (buffer, PATH_MAX, stream))
	{
		/* If the first character is 12, that's the end of the flat profile. */
		if (buffer[0] == 12)
			break;
		
		/* Remove the newline from the buffer */
		length = strlen (buffer);
		buffer[length - 1] = 0;
		
		fields = get_flat_profile_fields (buffer);
		
		if (fields)
		{
			gprof_flat_profile_add_entry (flat_profile, 
										  gprof_flat_profile_entry_new (fields));
			
			g_strfreev (fields);
		}
	}
	
	return flat_profile;
}

void
gprof_flat_profile_free (GProfFlatProfile *self)
{
	g_object_unref (self);
}

GProfFlatProfileEntry *
gprof_flat_profile_get_first_entry (GProfFlatProfile *self, GList **iter)
{
	*iter = self->priv->entries;
	
	if (self->priv->entries)
		return GPROF_FLAT_PROFILE_ENTRY ((*iter)->data);
	else
		return NULL;
}

GProfFlatProfileEntry *
gprof_flat_profile_find_entry (GProfFlatProfile* self, const gchar *name)
{
	return GPROF_FLAT_PROFILE_ENTRY (g_hash_table_lookup (self->priv->lookup_table,
														  name));
}

void 
gprof_flat_profile_dump (GProfFlatProfile *self, FILE *stream)
{
	GList *current;
	GProfFlatProfileEntry *entry;
	
	current = self->priv->entries;
	
	while (current)
	{
		entry = GPROF_FLAT_PROFILE_ENTRY (current->data);
		
		fprintf (stream, "Function: %s\n", 
				 gprof_flat_profile_entry_get_name (entry));
		fprintf (stream, "Time: %2.2f\n", 
				 gprof_flat_profile_entry_get_time_perc (entry));
		fprintf (stream, "Cumulative time: %2.2f\n", 
				 gprof_flat_profile_entry_get_cum_sec (entry));
		fprintf (stream, "Current function time: %2.2f\n", 
				 gprof_flat_profile_entry_get_self_sec (entry));
		fprintf (stream, "Calls: %i\n", 
				 gprof_flat_profile_entry_get_calls (entry));
		fprintf (stream, "Average time: %2.2f\n", 
				 gprof_flat_profile_entry_get_avg_ms (entry));
		fprintf (stream, "Total time: %2.2f\n", 
				 gprof_flat_profile_entry_get_total_ms (entry));
		fprintf (stream, "\n");
		
		current = g_list_next (current);
	}
}
