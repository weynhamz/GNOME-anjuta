/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-status.c Copyright (C) 2004 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-utils.h>

struct _AnjutaStatusPriv
{
	GHashTable *default_status_items;
	gint busy_count;
};

static gpointer parent_class = NULL;

static void
anjuta_status_finalize (GObject *widget)
{
	GNOME_CALL_PARENT(G_OBJECT_CLASS, finalize, (widget));
}

static void
anjuta_status_dispose (GObject *widget)
{
	AnjutaStatus *status;
	
	status = ANJUTA_STATUS (widget);
	
	if (status->priv->default_status_items)
		g_hash_table_destroy (status->priv->default_status_items);
	status->priv->default_status_items = NULL;
	
	GNOME_CALL_PARENT(G_OBJECT_CLASS, dispose, (widget));
}

static void
anjuta_status_instance_init (AnjutaStatus *status)
{
	status->priv = g_new0 (AnjutaStatusPriv, 1);
	
	status->priv->default_status_items =
		g_hash_table_new_full (g_str_hash, g_str_equal,
							   g_free, g_free);
}

static void
anjuta_status_class_init (AnjutaStatusClass *class)
{
	GObjectClass *object_class;
	
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = anjuta_status_finalize;
	object_class->dispose = anjuta_status_dispose;
}

GtkWidget *
anjuta_status_new (void)
{
	GtkWidget *status;

	status = GTK_WIDGET (g_object_new (ANJUTA_TYPE_STATUS,
						 "has-progress", FALSE, "has-status", TRUE,
						 "interactivity", GNOME_PREFERENCES_NEVER, NULL));
	return status;
}

void
anjuta_status_set (AnjutaStatus *status, gchar * mesg, ...)
{
	gchar* message;
	va_list args;

	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (mesg != NULL);
	
	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gnome_appbar_set_status (GNOME_APPBAR (status), message);
	g_free(message);
}

void
anjuta_status_push (AnjutaStatus *status, gchar * mesg, ...)
{
	gchar* message;
	va_list args;

	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (mesg != NULL);
	
	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gnome_appbar_push (GNOME_APPBAR (status), message);
	g_free(message);
}

void
anjuta_status_busy_push (AnjutaStatus *status)
{
	GtkWidget *top;
	GdkCursor *cursor;
	
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	
	top = gtk_widget_get_toplevel (GTK_WIDGET (status));
	if (top == NULL)
		return;
	
	status->priv->busy_count++;
	if (status->priv->busy_count > 1)
		return;
	cursor = gdk_cursor_new (GDK_WATCH);
	if (GTK_WIDGET (top)->window)
		gdk_window_set_cursor (GTK_WIDGET (top)->window, cursor);
	gdk_cursor_unref (cursor);
	gdk_flush ();
}

void
anjuta_status_busy_pop (AnjutaStatus *status)
{
	GtkWidget *top;
	
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	
	top = gtk_widget_get_toplevel (GTK_WIDGET (status));
	if (top == NULL)
		return;

	status->priv->busy_count--;
	if (status->priv->busy_count > 0)
		return;
	
	status->priv->busy_count = 0;
	if (GTK_WIDGET (top)->window)
		gdk_window_set_cursor (GTK_WIDGET (top)->window, NULL);
}

static void
foreach_hash (gpointer key, gpointer value, gpointer userdata)
{
	GString *str = (GString*)(userdata);
	const gchar *divider = ": ";
	const gchar *separator = "    ";
	
	g_string_append (str, separator);
	g_string_append (str, (const gchar*)key);
	g_string_append (str, divider);
	g_string_append (str, (const gchar*)value);
}

void
anjuta_status_set_default (AnjutaStatus *status, const gchar *label,
						   const gchar *value_format, ...)
{
	GString *str;
	gchar *status_str;
	
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (label != NULL);

	if (value_format)
	{
		gchar* value;
		va_list args;
		
		va_start (args, value_format);
		value = g_strdup_vprintf (value_format, args);
		va_end (args);
		g_hash_table_replace (status->priv->default_status_items,
							  g_strdup (label), value);
	}
	else
	{
		if (g_hash_table_lookup (status->priv->default_status_items, label))
		{
			g_hash_table_remove (status->priv->default_status_items, label);
		}
	}
	
	/* Update default status */
	str = g_string_new (NULL);
	g_hash_table_foreach (status->priv->default_status_items, foreach_hash, str);
	status_str = g_string_free (str, FALSE);
	gnome_appbar_set_default (GNOME_APPBAR (status), status_str);
	g_free (status_str);
}

ANJUTA_TYPE_BEGIN(AnjutaStatus, anjuta_status, GNOME_TYPE_APPBAR);
ANJUTA_TYPE_END;
