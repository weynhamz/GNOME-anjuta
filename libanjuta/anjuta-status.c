/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-status.c
 * Copyright (C) 2004 Naba Kumar  <naba@gnome.org>
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 */

/**
 * SECTION:anjuta-status
 * @short_description: Program status such as status message, progress etc. 
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-status.h
 * 
 */

#include <config.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/e-splash.h>

struct _AnjutaStatusPriv
{
	GHashTable *default_status_items;
	gint busy_count;
	GHashTable *widgets;
	
	/* Status bar */
	GtkWidget *status_bar;
	guint status_message;
	guint push_message;
	GList *push_values;
	
	/* Progress bar */
	GtkWidget *progress_bar;
	gint total_ticks;
	gint current_ticks;
	
	/* Splash */
	GtkWidget *splash;
	gboolean disable_splash;
	gchar *splash_file;
	gint splash_progress_position;
	
	/* Title window */
	GtkWindow *window;
};

enum {
	BUSY,
	LAST_SIGNAL
};

static gpointer parent_class = NULL;
static guint status_signals[LAST_SIGNAL] = { 0 };

static void on_widget_destroy (AnjutaStatus *status, GObject *widget);

static void
anjuta_status_finalize (GObject *widget)
{
	g_free(ANJUTA_STATUS(widget)->priv);
	G_OBJECT_CLASS (parent_class)->finalize (widget);
}

static void
foreach_widget_unref (gpointer key, gpointer value, gpointer data)
{
	g_object_weak_unref (G_OBJECT (key), (GWeakNotify) on_widget_destroy, data);
}

static void
anjuta_status_dispose (GObject *widget)
{
	AnjutaStatus *status;
	
	status = ANJUTA_STATUS (widget);
	
	if (status->priv->default_status_items)
	{
		g_hash_table_destroy (status->priv->default_status_items);
		status->priv->default_status_items = NULL;
	}
	if (status->priv->splash != NULL) {
		gtk_widget_destroy (status->priv->splash);
		status->priv->splash = NULL;
	}
	if (status->priv->splash_file)
	{
		g_free (status->priv->splash_file);
		status->priv->splash_file = NULL;
	}
	if (status->priv->push_values)
	{
		g_list_free (status->priv->push_values);
		status->priv->push_values = NULL;
	}
	if (status->priv->widgets)
	{
		g_hash_table_foreach (status->priv->widgets,
							  foreach_widget_unref, widget);
		g_hash_table_destroy (status->priv->widgets);
		status->priv->widgets = NULL;
	}
	if (status->priv->window)
	{
		g_object_remove_weak_pointer (G_OBJECT (status->priv->window),
						   (gpointer*)(gpointer)&status->priv->window);
		status->priv->window = NULL;
	}
	if (status->priv->progress_bar)
	{
		g_object_remove_weak_pointer (G_OBJECT (status->priv->progress_bar), 
							(gpointer)&status->priv->progress_bar);
		gtk_widget_destroy (status->priv->progress_bar);
		status->priv->progress_bar = NULL;
	}
	if (status->priv->status_bar)
	{
		g_object_remove_weak_pointer (G_OBJECT (status->priv->status_bar), 
							(gpointer)&status->priv->status_bar);
		gtk_widget_destroy (status->priv->status_bar);
		status->priv->status_bar = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (widget);
}

static void
anjuta_status_instance_init (AnjutaStatus *status)
{	
	status->priv = g_new0 (AnjutaStatusPriv, 1);
	status->priv->progress_bar = gtk_progress_bar_new ();
	gtk_box_pack_start (GTK_BOX (status), status->priv->progress_bar, FALSE, TRUE, 0);
	gtk_widget_show (status->priv->progress_bar);
	g_object_add_weak_pointer (G_OBJECT (status->priv->progress_bar),
							   (gpointer)&status->priv->progress_bar);
	status->priv->status_bar = gtk_statusbar_new ();
	gtk_box_pack_start (GTK_BOX (status), status->priv->status_bar, TRUE, TRUE, 0);
	gtk_widget_show (status->priv->status_bar);
	g_object_add_weak_pointer (G_OBJECT(status->priv->status_bar), 
							   (gpointer)&status->priv->status_bar);
	status->priv->status_message = gtk_statusbar_get_context_id (GTK_STATUSBAR (status->priv->status_bar),
																 "status-message");
	status->priv->push_message = gtk_statusbar_get_context_id (GTK_STATUSBAR (status->priv->status_bar),
															   "push-message");
	status->priv->push_values = NULL;
	status->priv->splash_file = NULL;
	status->priv->splash_progress_position = 0;
	status->priv->disable_splash = FALSE;
	status->priv->total_ticks = 0;
	status->priv->current_ticks = 0;
	status->priv->splash = NULL;
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
	
	status_signals[BUSY] =
		g_signal_new ("busy",
				  ANJUTA_TYPE_STATUS,
				  G_SIGNAL_RUN_LAST,
				  G_STRUCT_OFFSET (AnjutaStatusClass, busy),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__BOOLEAN,
				  G_TYPE_NONE, 1,
				  G_TYPE_BOOLEAN);
}

GtkWidget *
anjuta_status_new (void)
{
	GtkWidget *status;

	status = GTK_WIDGET (g_object_new (ANJUTA_TYPE_STATUS, NULL));
	return status;
}

void
anjuta_status_set (AnjutaStatus *status, const gchar * mesg, ...)
{
	gchar* message;
	va_list args;

	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (mesg != NULL);
	
	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gtk_statusbar_pop (GTK_STATUSBAR (status->priv->status_bar),
					   status->priv->status_message);
	gtk_statusbar_push (GTK_STATUSBAR (status->priv->status_bar),
						status->priv->status_message, message);
	g_free(message);
}

void
anjuta_status_push (AnjutaStatus *status, const gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	guint value;

	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (mesg != NULL);
	
	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	value = gtk_statusbar_push (GTK_STATUSBAR (status->priv->status_bar),
								status->priv->push_message, message);
	status->priv->push_values = g_list_prepend (status->priv->push_values,
												GUINT_TO_POINTER (value));
	g_free(message);
}

void
anjuta_status_pop (AnjutaStatus *status)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	
	gtk_statusbar_pop (GTK_STATUSBAR (status->priv->status_bar),
					   status->priv->push_message);

	status->priv->push_values = g_list_remove_link (status->priv->push_values,
													status->priv->push_values);
}

void
anjuta_status_clear_stack (AnjutaStatus *status)
{
	GList *l;
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	
	for (l = status->priv->push_values; l != NULL; l = g_list_next (l))
	{
		guint value = GPOINTER_TO_UINT (l->data);
		gtk_statusbar_remove (GTK_STATUSBAR (status->priv->status_bar),
							  status->priv->push_message, value);
	}
	g_list_free (status->priv->push_values);
	status->priv->push_values = NULL;
}

static void
foreach_widget_set_cursor (gpointer widget, gpointer value, gpointer cursor)
{
	if (GTK_WIDGET (widget)->window)
		gdk_window_set_cursor (GTK_WIDGET (widget)->window, (GdkCursor*)cursor);
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
	if (status->priv->widgets)
		g_hash_table_foreach (status->priv->widgets,
							  foreach_widget_set_cursor, cursor);
	gdk_cursor_unref (cursor);
	gdk_flush ();
	g_signal_emit_by_name (G_OBJECT (status), "busy", TRUE);
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
	if (status->priv->widgets)
		g_hash_table_foreach (status->priv->widgets,
							  foreach_widget_set_cursor, NULL);
	g_signal_emit_by_name (G_OBJECT (status), "busy", FALSE);
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
	anjuta_status_set (status, status_str, NULL);
	g_free (status_str);
}

static void
on_widget_destroy (AnjutaStatus *status, GObject *widget)
{
	if (g_hash_table_lookup (status->priv->widgets, widget))
		g_hash_table_remove (status->priv->widgets, widget);
}

void
anjuta_status_add_widget (AnjutaStatus *status, GtkWidget *widget)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	
	if (status->priv->widgets == NULL)
		status->priv->widgets =
			g_hash_table_new (g_direct_hash, g_direct_equal);
	
	g_hash_table_insert (status->priv->widgets, widget, widget);
	g_object_weak_ref (G_OBJECT (widget),
					   (GWeakNotify) (on_widget_destroy), status);
}

void
anjuta_status_set_splash (AnjutaStatus *status, const gchar *splash_file,
						  gint splash_progress_position)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (splash_file != NULL);
	g_return_if_fail (splash_progress_position >= 0);
	if (status->priv->splash_file)
		g_free (status->priv->splash_file);
	status->priv->splash_file = g_strdup (splash_file);
	status->priv->splash_progress_position = splash_progress_position;
}

void
anjuta_status_disable_splash (AnjutaStatus *status,
							  gboolean disable_splash)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));

	status->priv->disable_splash = disable_splash;
	if (status->priv->splash)
	{
		gtk_widget_destroy (status->priv->splash);
		status->priv->splash = NULL;
		anjuta_status_progress_add_ticks (status, 0);
	}
}

void
anjuta_status_progress_add_ticks (AnjutaStatus *status, gint ticks)
{
	gfloat percentage;
	
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (ticks >= 0);
	
	status->priv->total_ticks += ticks;
	if (!GTK_WIDGET_REALIZED (status))
	{
		if (status->priv->splash == NULL &&
			status->priv->splash_file &&
			!status->priv->disable_splash)
		{
			status->priv->splash = e_splash_new (status->priv->splash_file, 100);
			if (status->priv->splash)
					gtk_widget_show (status->priv->splash);
		}
	}
	percentage = ((gfloat)status->priv->current_ticks)/status->priv->total_ticks;
	if (status->priv->splash)
	{
		e_splash_set (E_SPLASH(status->priv->splash), NULL, NULL, NULL,
					  percentage);
		while (g_main_context_iteration(NULL, FALSE));
	}
	else
	{	
		if (status->priv->progress_bar && status->priv->status_bar)
		{
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (status->priv->progress_bar),
										   percentage);
			gtk_widget_queue_draw (GTK_WIDGET (status->priv->status_bar));
			gtk_widget_queue_draw (GTK_WIDGET (status->priv->progress_bar));
			if (GTK_WIDGET(status->priv->progress_bar)->window != NULL &&
				GDK_IS_WINDOW(GTK_WIDGET(status->priv->progress_bar)->window))
				gdk_window_process_updates (GTK_WIDGET(status->priv->progress_bar)->window, TRUE);
			if (GTK_WIDGET(status->priv->status_bar)->window != NULL &&
				GDK_IS_WINDOW(GTK_WIDGET(status->priv->status_bar)->window))
				gdk_window_process_updates (GTK_WIDGET(status->priv->status_bar)->window, TRUE);
		}
	}
}

void
anjuta_status_progress_pulse (AnjutaStatus *status, const gchar *text)
{
	GtkProgressBar *progressbar;
	GtkWidget *statusbar;
	
	progressbar = GTK_PROGRESS_BAR (status->priv->progress_bar);
	statusbar = status->priv->status_bar;
	
	if (text)
		anjuta_status_set (status, "%s", text);
	
	gtk_progress_bar_pulse (progressbar);
	
	gtk_widget_queue_draw (GTK_WIDGET (statusbar));
	gtk_widget_queue_draw (GTK_WIDGET (progressbar));
	if (GTK_WIDGET(progressbar)->window != NULL &&
		GDK_IS_WINDOW(GTK_WIDGET(progressbar)->window))
		gdk_window_process_updates (GTK_WIDGET(progressbar)->window, TRUE);
	if (GTK_WIDGET(statusbar)->window != NULL &&
		GDK_IS_WINDOW(GTK_WIDGET(statusbar)->window))
		gdk_window_process_updates (GTK_WIDGET(statusbar)->window, TRUE);
}

void
anjuta_status_progress_tick (AnjutaStatus *status,
							 GdkPixbuf *icon, const gchar *text)
{
	gfloat percentage;
		
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (status->priv->total_ticks != 0);
	
	status->priv->current_ticks++;
	percentage = ((gfloat)status->priv->current_ticks)/status->priv->total_ticks;
	
	if (status->priv->splash)
	{
		e_splash_set (E_SPLASH(status->priv->splash), icon, text, NULL, percentage);
	}
	else
	{
		GtkProgressBar *progressbar;
		GtkWidget *statusbar;
		
		if (text)
			anjuta_status_set (status, "%s", text);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (status->priv->progress_bar),
									   percentage);
		progressbar = GTK_PROGRESS_BAR (status->priv->progress_bar);
		statusbar = status->priv->status_bar;
		gtk_widget_queue_draw (GTK_WIDGET (statusbar));
		gtk_widget_queue_draw (GTK_WIDGET (progressbar));
		if (GTK_WIDGET(progressbar)->window != NULL &&
			GDK_IS_WINDOW(GTK_WIDGET(progressbar)->window))
			gdk_window_process_updates (GTK_WIDGET(progressbar)->window, TRUE);
		if (GTK_WIDGET(statusbar)->window != NULL &&
			GDK_IS_WINDOW(GTK_WIDGET(statusbar)->window))
			gdk_window_process_updates (GTK_WIDGET(statusbar)->window, TRUE);
	}
	if (status->priv->current_ticks >= status->priv->total_ticks)
		anjuta_status_progress_reset (status);
}

void
anjuta_status_progress_increment_ticks (AnjutaStatus *status, gint ticks,
										const gchar *text)
{
	gfloat percentage;
		
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (status->priv->total_ticks != 0);
	
	status->priv->current_ticks += ticks;
	percentage = ((gfloat)status->priv->current_ticks)/status->priv->total_ticks;
	
	GtkProgressBar *progressbar;
	GtkWidget *statusbar;
	
	if (text)
		anjuta_status_set (status, "%s", text);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (status->priv->progress_bar),
									   percentage);
	progressbar = GTK_PROGRESS_BAR (status->priv->progress_bar);
	statusbar = status->priv->status_bar;
	gtk_widget_queue_draw (GTK_WIDGET (statusbar));
	gtk_widget_queue_draw (GTK_WIDGET (progressbar));
	if (GTK_WIDGET(progressbar)->window != NULL &&
		GDK_IS_WINDOW(GTK_WIDGET(progressbar)->window))
		gdk_window_process_updates (GTK_WIDGET(progressbar)->window, TRUE);
	if (GTK_WIDGET(statusbar)->window != NULL &&
		GDK_IS_WINDOW(GTK_WIDGET(statusbar)->window))
		gdk_window_process_updates (GTK_WIDGET(statusbar)->window, TRUE);
	
	if (status->priv->current_ticks >= status->priv->total_ticks)
		anjuta_status_progress_reset (status);
}

void
anjuta_status_progress_reset (AnjutaStatus *status)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	
	if (status->priv->splash)
	{
		gtk_widget_destroy (status->priv->splash);
		status->priv->splash = NULL;
	}
	status->priv->current_ticks = 0;
	status->priv->total_ticks = 0;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (status->priv->progress_bar), 0);
	anjuta_status_clear_stack (status);
}

static gboolean
anjuta_status_timeout (AnjutaStatus *status)
{
	anjuta_status_pop (status);
	return FALSE;
}

/* Display message in status until timeout (secondes) */
void
anjuta_status (AnjutaStatus *status, const gchar *mesg, gint timeout)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (mesg != NULL);
	anjuta_status_push (status, "%s", mesg);
	g_timeout_add_seconds (timeout, (void*) anjuta_status_timeout, status);
}

void
anjuta_status_set_title_window (AnjutaStatus *status, GtkWidget *window)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	g_return_if_fail (GTK_IS_WINDOW (window));
	status->priv->window = GTK_WINDOW (window);
	g_object_add_weak_pointer (G_OBJECT (window),
							   (gpointer*)(gpointer)&status->priv->window);
}

void
anjuta_status_set_title (AnjutaStatus *status, const gchar *title)
{
	g_return_if_fail (ANJUTA_IS_STATUS (status));
	
	if (!status->priv->window)
		return;
	
	const gchar *app_name = g_get_application_name();
	if (title)
	{
		gchar* str = g_strconcat (title, " - ", app_name, NULL);
		gtk_window_set_title (status->priv->window, str);
		g_free (str);
	}
	else
	{
		gtk_window_set_title (status->priv->window, app_name);
	}
}

ANJUTA_TYPE_BEGIN(AnjutaStatus, anjuta_status, GTK_TYPE_HBOX);
ANJUTA_TYPE_END;
