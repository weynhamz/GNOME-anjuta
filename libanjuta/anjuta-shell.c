/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-shell.c
 * Copyright (C) Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:anjuta-shell
 * @title: AnjutaShell
 * @short_description: Application shell interface
 * @see_also:
 * @stability: Unstable
 * @include: libanjuta/anjuta-shell.h
 * 
 * Shell is the playground where plugins are loaded and their UI
 * widgets shown. It is also a place where plugins export objects for letting
 * other pluings to use. Plugins are loaded into shell on demand, but some
 * plugins are loaded on startup (such as help and text editor plugin).
 * Demand to load a plugin can be made by requesting for a primary inferface
 * using anjuta_shell_get_interface() or anjuta_shell_get_object().
 * 
 * Plugins can add widgets in shell with
 * anjuta_shell_add_widget() and remove with anjuta_shell_remove_widget()
 * functions.
 * 
 * In Anjuta, shell is implemented using an advanced widget docking system,
 * allowing plugin widgets to dock, undock and layout in any fashion. Dock
 * layout is also maintained internally and is transparent to plugin
 * implementations.
 * 
 * #AnjutaShell allows plugins to export arbitrary objects as <emphasis>
 * values</emphasis> in its <emphasis>Values System</emphasis>. "value_added"
 * and "value_removed" signals are emitted when a value is added to or
 * removed from the <emphasis>Values System</emphasis>, hence notifying
 * plugins of its state. However, plugins should really not connect directly
 * to these signals, because they are emitted for all values
 * and not just for the values the plugin is interested in. Instead,
 * to monitor specific <emphasis>Values</emphasis>, plugins should
 * setup watches using anjuta_plugin_add_watch().
 * 
 * <emphasis>Values</emphasis> are added, get or removed with
 * anjuta_shell_add_value() and anjuta_shell_get_value() or
 * anjuta_shell_remove_value(). There multi-valued equivalent functions
 * can be used to manipulate multiple values at once.
 * 
 * <emphasis>Values</emphasis> are identified with names. Since <emphasis>
 * Values</emphasis> are effectively variables, their names should follow
 * the standard GNOME variable naming convention and should be as descriptive
 * as possible (e.g project_root_directory, project_name etc.). It is also
 * essential that meaningful prefix be given to names so that <emphasis>
 * Values</emphasis> are easily grouped (e.g all values exported by a 
 * project manager should start with project_ prefix).
 * 
 * Plugins can find other plugins with anjuta_shell_get_object() or 
 * anjuta_shell_get_interface() based on their primary interfaces.
 */

#include <config.h>
#include <string.h>
#include <gobject/gvaluecollector.h>
#include "anjuta-shell.h"
#include "anjuta-marshal.h"
#include "anjuta-debug.h"

typedef struct {
	GtkWidget *widget;
	gchar *name;
	gchar *title;
	gchar *stock_id;
	AnjutaShellPlacement placement;
} WidgetQueueData;

static void
on_widget_data_free (WidgetQueueData *data)
{
	g_object_unref (data->widget);
	g_free (data->name);
	g_free (data->title);
	g_free (data->stock_id);
	g_free (data);
}

static void
on_widget_data_add (WidgetQueueData *data, AnjutaShell *shell)
{
	ANJUTA_SHELL_GET_IFACE (shell)->add_widget (shell, data->widget,
												data->name,
												data->title,
												data->stock_id,
												data->placement,
												NULL);
}

static void
on_destroy_widget_queue (gpointer data)
{
	GQueue *queue = (GQueue*)data;
	g_queue_foreach (queue, (GFunc)on_widget_data_free, NULL);
	g_queue_free (queue);
}

GQuark 
anjuta_shell_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("anjuta-shell-quark");
	}
	
	return quark;
}

/**
 * anjuta_shell_freeze:
 * @shell: A #AnjutaShell interface.
 * @error: Error propagation object.
 *
 * Freezes addition of any UI elements (widgets) in the shell. All widget
 * additions are queued for later additions when freeze count reaches 0.
 * Any number of this function can be called and each call will increase
 * the freeze count. anjuta_shell_thaw() will reduce the freeze count by
 * 1 and real thawing happens when the count reaches 0.
 */
void
anjuta_shell_freeze (AnjutaShell *shell, GError **error)
{
	gint freeze_count;
	
	g_return_if_fail (shell != NULL);
	freeze_count = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (shell),
													   "__freeze_count"));
	freeze_count++;
	g_object_set_data (G_OBJECT (shell), "__freeze_count",
					   GINT_TO_POINTER (freeze_count));
}

/**
 * anjuta_shell_thaw:
 * @shell: A #AnjutaShell interface.
 * @error: Error propagation object.
 *
 * Reduces the freeze count by one and performs pending widget additions
 * when the count reaches 0.
 */
void
anjuta_shell_thaw (AnjutaShell *shell, GError **error)
{
	gint freeze_count;
	
	g_return_if_fail (shell != NULL);
	freeze_count = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (shell),
													   "__freeze_count"));
	freeze_count--;
	if (freeze_count < 0)
		freeze_count = 0;
	g_object_set_data (G_OBJECT (shell), "__freeze_count",
					   GINT_TO_POINTER (freeze_count));
	
	if (freeze_count <= 0)
	{
		/* Add all pending widgets */
		/* DEBUG_PRINT ("Thawing shell ..."); */
		
		GQueue *queue;
		queue = g_object_get_data (G_OBJECT (shell), "__widget_queue");
		if (queue)
		{
			g_queue_reverse (queue);
			g_queue_foreach (queue, (GFunc)on_widget_data_add, shell);
			g_object_set_data (G_OBJECT (shell), "__widget_queue", NULL);
		}
	}
}

/**
 * anjuta_shell_add_widget:
 * @shell: A #AnjutaShell interface.
 * @widget: Then widget to add
 * @name: Name of the widget. None translated string used to identify it in 
 * the shell.
 * @stock_id: Icon stock ID. Could be null.
 * @title: Translated string which is displayed along side the widget when
 * required (eg. as window title or notebook tab label).
 * @placement: Placement of the widget in shell.
 * @error: Error propagation object.
 *
 * Adds @widget in the shell. The @placement tells where the widget should
 * appear, but generally it will be overridden by the container
 * (dock, notebook, GtkContainer etc.) saved layout.
 */
void
anjuta_shell_add_widget (AnjutaShell *shell,
			 GtkWidget *widget,
			 const char *name,
			 const char *title,
			 const char *stock_id,
			 AnjutaShellPlacement placement,
			 GError **error)
{
	GQueue *widget_queue;
	gint freeze_count;
	
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (name != NULL);
	g_return_if_fail (title != NULL);

	freeze_count = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (shell),
													   "__freeze_count"));
	if (freeze_count <= 0)
	{
		ANJUTA_SHELL_GET_IFACE (shell)->add_widget (shell, widget, name,
													title, stock_id,
													placement, error);
	}
	else
	{
		/* Queue the operation */
		WidgetQueueData *qd;
		
		widget_queue = g_object_get_data (G_OBJECT (shell), "__widget_queue");
		if (!widget_queue)
		{
			widget_queue = g_queue_new ();
			g_object_set_data_full (G_OBJECT (shell), "__widget_queue",
									widget_queue, on_destroy_widget_queue);
		}
		qd = g_new0(WidgetQueueData, 1);
		g_object_ref (G_OBJECT (widget));
		qd->widget = widget;
		qd->name = g_strdup (name);
		qd->title = g_strdup (title);
		if (stock_id)
			qd->stock_id = g_strdup (stock_id);
		qd->placement = placement;
		
		g_queue_push_head (widget_queue, qd);
	}
}

/**
 * anjuta_shell_remove_widget:
 * @shell: A #AnjutaShell interface
 * @widget: The widget to remove
 * @error: Error propagation object
 * 
 * Removes the widget from shell. The widget should have been added before
 * with #anjuta_shell_add_widget.
 */
void
anjuta_shell_remove_widget (AnjutaShell *shell,
							GtkWidget *widget,
							GError **error)
{
	GQueue *queue;
	gboolean found_in_queue;
	
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	/* If there is a queue, remove widgets from it */
	found_in_queue = FALSE;
	queue = g_object_get_data (G_OBJECT (shell), "__widget_queue");
	if (queue)
	{
		gint i;
		for (i = g_queue_get_length(queue) - 1; i >= 0; i--)
		{
			WidgetQueueData *qd;
			
			qd = g_queue_peek_nth (queue, i);
			if (qd->widget == widget)
			{
				g_queue_remove (queue, qd);
				on_widget_data_free (qd);
				found_in_queue = TRUE;
				break;
			}
		}
	}
	if (!found_in_queue)
		ANJUTA_SHELL_GET_IFACE (shell)->remove_widget (shell, widget, error);
}

/**
 * anjuta_shell_present_widget:
 * @shell: A #AnjutaShell interface
 * @widget: The widget to present
 * @error: Error propagation object
 * 
 * Make sure the widget is visible to user. If the widget is hidden, it will
 * be shown. If it is not visible to user, it will be made visible.
 */
void
anjuta_shell_present_widget (AnjutaShell *shell,
							 GtkWidget *widget,
							 GError **error)
{
	GQueue *queue;
	gboolean found_in_queue;
	
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	/* If there is a queue and the widget is in the queue, there is no
	 * way we can 'present' the widget */
	found_in_queue = FALSE;
	queue = g_object_get_data (G_OBJECT (shell), "__widget_queue");
	if (queue)
	{
		gint i;
		for (i = g_queue_get_length(queue) - 1; i >= 0; i--)
		{
			WidgetQueueData *qd;
			
			qd = g_queue_peek_nth (queue, i);
			if (qd->widget == widget)
			{
				found_in_queue = TRUE;
				break;
			}
		}
	}
	if (!found_in_queue)
		ANJUTA_SHELL_GET_IFACE (shell)->present_widget (shell, widget, error);
}

/**
 * anjuta_shell_add_value:
 * @shell: A #AnjutaShell interface
 * @name: Name of the value
 * @value: Value to add
 * @error: Error propagation object
 *
 * Sets a value in the shell with the given name. Any previous value will
 * be overridden. "value_added" signal will be emitted. Objects connecting
 * to this signal can then update their data according to the new value.
 */
void
anjuta_shell_add_value (AnjutaShell *shell,
			const char *name,
			const GValue *value,
			GError **error)
{
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (name != NULL);
	g_return_if_fail (value != NULL);

	ANJUTA_SHELL_GET_IFACE (shell)->add_value (shell, name, value, error);
}

/**
 * anjuta_shell_add_valist:
 * @shell: A #AnjutaShell interface
 * @first_name: First value name
 * @first_type: First value type
 * @var_args: First value, Second value name, Second value type ....
 * 
 * Adds a valist of values in the shell. The valist should be in the order -
 * value1, name2, type2, value2,... "value_added" signal will be emitted
 * for each of the value.
 */
void
anjuta_shell_add_valist (AnjutaShell *shell,
			 const char *first_name,
			 GType first_type,
			 va_list var_args)
{
	const char *name;
	GType type;

	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (first_name != NULL);
	
	name = first_name;
	type = first_type;

	while (name) {
		GValue value = {0, };
		GError *err = NULL;
		char *error;
		
		g_value_init (&value, type);
		
		G_VALUE_COLLECT (&value, var_args, 0, &error);

		if (error){
			g_warning ("%s: %s", G_STRLOC, error);
			g_free (error);
			break;
		}
		
		anjuta_shell_add_value (shell, name, &value, &err);

		g_value_unset (&value);

		if (err) {
			g_warning ("Could not set value: %s\n", err->message);
			g_error_free (err);
			break;
		}

		name = va_arg (var_args, char *);
		if (name) {
			type = va_arg (var_args, GType);
		}
	}
}

/**
 * anjuta_shell_add:
 * @shell: A #AnjutaShell interface
 * @first_name: First value name
 * @first_type: First value type
 * @...: First value, Second value name, Second value type .... NULL
 * 
 * Adds a list of values in the shell. The list should be NULL terminated
 * and should be in the order - name1, type1, value1, name2, type2, value2,
 * ..., NULL. "value_added" signal will be emitted for each of the value.
 */
void
anjuta_shell_add (AnjutaShell  *shell,
		  const char *first_name,
		  GType first_type,
		  ...)
{
	va_list var_args;

	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (first_name != NULL);

	va_start (var_args, first_type);
	anjuta_shell_add_valist (shell, first_name, first_type, var_args);
	va_end (var_args);
}

/**
 * anjuta_shell_get_value:
 * @shell: A #AnjutaShell interface
 * @name: Name of the value to get
 * @value: Value to get
 * @error: Error propagation object
 *
 * Gets a value from the shell with the given name. The value will be set
 * in the passed value pointer.
 */
void
anjuta_shell_get_value (AnjutaShell *shell,
			const char *name,
			GValue *value,
			GError **error)
{
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (name != NULL);
	g_return_if_fail (value != NULL);

	ANJUTA_SHELL_GET_IFACE (shell)->get_value (shell, name, value, error);
}

/**
 * anjuta_shell_get_valist:
 * @shell: A #AnjutaShell interface
 * @first_name: First value name
 * @first_type: First value type
 * @var_args: First value holder, Second value name, Second value type ....
 * 
 * Gets a valist of values from the shell. The valist should be in the order -
 * value1, name2, type2, value2,...
 */
void
anjuta_shell_get_valist (AnjutaShell *shell,
			 const char *first_name,
			 GType first_type,
			 va_list var_args)
{
	const char *name;
	GType type;

	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (first_name != NULL);
	
	name = first_name;
	type = first_type;

	while (name) {
		GValue value = {0, };
		GError *err = NULL;
		char *error;
		
		g_value_init (&value, type);
		
		anjuta_shell_get_value (shell, name, &value, &err);

		if (err) {
			g_warning ("Could not get value: %s", err->message);
			g_error_free (err);
			break;
		}

		G_VALUE_LCOPY (&value, var_args, 0, &error);
		
		if (error){
			g_warning ("%s: %s", G_STRLOC, error);
			g_free (error);
			break;
		}

		g_value_unset (&value);

		name = va_arg (var_args, char *);
		if (name) {
			type = va_arg (var_args, GType);
		}
	}
}

/**
 * anjuta_shell_get:
 * @shell: A #AnjutaShell interface
 * @first_name: First value name
 * @first_type: First value type
 * @...: First value holder, Second value name, Second value type .... NULL
 * 
 * Gets a list of values in the shell. The list should be NULL terminated
 * and should be in the order - name1, type1, value1, name2, type2, value2,
 * ..., NULL.
 */
void
anjuta_shell_get (AnjutaShell  *shell,
		  const char *first_name,
		  GType first_type,
		  ...)
{
	va_list var_args;

	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (first_name != NULL);

	va_start (var_args, first_type);
	anjuta_shell_get_valist (shell, first_name, first_type, var_args);
	va_end (var_args);
}

/**
 * anjuta_shell_remove_value:
 * @shell: A #AnjutaShell interface
 * @name: Name of the value to remove
 * @error: Error propagation object
 *
 * Removes a value from the shell with the given name. "value_removed" signal
 * will be emitted. Objects connecting to this signal can then update their
 * data/internal-state accordingly.
 */
void
anjuta_shell_remove_value (AnjutaShell *shell,
						   const char *name,
						   GError **error)
{
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (name != NULL);

	ANJUTA_SHELL_GET_IFACE (shell)->remove_value (shell, name, error);
}

/**
 * anjuta_shell_get_object:
 * @shell: A #AnjutaShell interface
 * @iface_name: The interface implemented by the object to be found
 * @error: Error propagation.
 *
 * Searches the currently available plugins to find the one which
 * implements the given interface as primary interface and returns it. If
 * the plugin is not yet loaded, it will be loaded and activated.
 * The returned object is garanteed to be an implementor of the
 * interface (as exported by the plugin metafile). It only searches
 * from the pool of plugin objects loaded in this shell and can only search
 * by primary interface. If there are more objects implementing this primary
 * interface, user might be prompted to select one from them (and might give
 * the option to use it as default for future queries). A typical usage of this
 * function is:
 * <programlisting>
 * GObject *docman =
 *     anjuta_plugins_get_object (shell, "IAnjutaDocumentManager", error);
 * </programlisting>
 * Notice that this function takes the interface name string as string, unlike
 * anjuta_plugins_get_interface() which takes the type directly.
 *
 * Return value: A plugin object implementing the primary interface or NULL.
 */
GObject*
anjuta_shell_get_object (AnjutaShell *shell, const gchar *iface_name,
						 GError **error)
{
	g_return_val_if_fail (shell != NULL, NULL);
	g_return_val_if_fail (iface_name != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);

	return ANJUTA_SHELL_GET_IFACE (shell)->get_object (shell, iface_name, error);
}

/**
 * anjuta_shell_get_status:
 * @shell: A #AnjutaShell interface
 * @error: Error propagation object
 *
 * Retrieves the #AnjutaStatus object associated with the shell.
 *
 * Return value: The #AnjutaStatus object.
 */
AnjutaStatus*
anjuta_shell_get_status (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (shell != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);

	return ANJUTA_SHELL_GET_IFACE (shell)->get_status (shell, error);
}

/**
 * anjuta_shell_get_ui:
 * @shell: A #AnjutaShell interface
 * @error: Error propagation object
 *
 * Retrieves the #AnjutaUI object associated with the shell.
 *
 * Return value: The #AnjutaUI object.
 */
AnjutaUI*
anjuta_shell_get_ui (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (shell != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);

	return ANJUTA_SHELL_GET_IFACE (shell)->get_ui (shell, error);
}

/**
 * anjuta_shell_get_preferences:
 * @shell: A #AnjutaShell interface
 * @error: Error propagation object
 *
 * Retrieves the #AnjutaPreferences object associated with the shell.
 *
 * Return value: The #AnjutaPreferences object.
 */
AnjutaPreferences*
anjuta_shell_get_preferences (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (shell != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);

	return ANJUTA_SHELL_GET_IFACE (shell)->get_preferences (shell, error);
}

/**
 * anjuta_shell_get_plugin_manager:
 * @shell: A #AnjutaShell interface
 * @error: Error propagation object
 *
 * Retrieves the #AnjutaPluginManager object associated with the shell.
 *
 * Return value: The #AnjutaPluginManager object.
 */
AnjutaPluginManager*
anjuta_shell_get_plugin_manager (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (shell != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);

	return ANJUTA_SHELL_GET_IFACE (shell)->get_plugin_manager (shell, error);
}

/**
 * anjuta_shell_get_profile_manager:
 * @shell: A #AnjutaShell interface
 * @error: Error propagation object
 *
 * Retrieves the #AnjutaProfileManager object associated with the shell.
 *
 * Return value: The #AnjutaProfileManager object.
 */
AnjutaProfileManager*
anjuta_shell_get_profile_manager (AnjutaShell *shell, GError **error)
{
	g_return_val_if_fail (shell != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);

	return ANJUTA_SHELL_GET_IFACE (shell)->get_profile_manager (shell, error);
}

void
anjuta_shell_session_save (AnjutaShell *shell, const gchar *session_directory,
						   GError **error)
{
	AnjutaSession *session;

	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (session_directory != NULL);

	session = anjuta_session_new (session_directory);
	anjuta_session_clear (session);
	g_signal_emit_by_name (G_OBJECT (shell), "save_session",
						   ANJUTA_SESSION_PHASE_FIRST, session);
	g_signal_emit_by_name (G_OBJECT (shell), "save_session",
						   ANJUTA_SESSION_PHASE_NORMAL, session);
	g_signal_emit_by_name (G_OBJECT (shell), "save_session",
						   ANJUTA_SESSION_PHASE_LAST, session);
	anjuta_session_sync (session);
	g_object_unref (session);
}

void
anjuta_shell_session_load (AnjutaShell *shell, const gchar *session_directory,
						   GError **error)
{
	AnjutaSession *session;

	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (session_directory != NULL);

	/* Do not allow multiple session loadings at once. Could be a trouble */
	if (g_object_get_data (G_OBJECT (shell), "__session_loading"))
	{
		/* DEBUG_PRINT ("A session load is requested in the middle of another!!"); */
		return;
	}
	g_object_set_data (G_OBJECT (shell), "__session_loading", "1");

	session = anjuta_session_new (session_directory);
	g_signal_emit_by_name (G_OBJECT (shell), "load_session",
						   ANJUTA_SESSION_PHASE_FIRST, session);
	g_signal_emit_by_name (G_OBJECT (shell), "load_session",
						   ANJUTA_SESSION_PHASE_NORMAL, session);
	g_signal_emit_by_name (G_OBJECT (shell), "load_session",
						   ANJUTA_SESSION_PHASE_LAST, session);
	g_object_unref (session);

	g_object_set_data (G_OBJECT (shell), "__session_loading", NULL);
}

void
anjuta_shell_save_prompt (AnjutaShell *shell,
						  AnjutaSavePrompt *save_prompt,
						  GError **error)
{
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (ANJUTA_IS_SAVE_PROMPT (save_prompt));
	g_signal_emit_by_name (shell, "save-prompt", save_prompt);
}

static void
anjuta_shell_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	
	if (!initialized) {
		g_signal_new ("value-added",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, value_added),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__STRING_BOXED,
			      G_TYPE_NONE, 2,
			      G_TYPE_STRING, G_TYPE_VALUE);
		
		g_signal_new ("value-removed",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, value_removed),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
		g_signal_new ("save-session",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, save_session),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__INT_OBJECT,
			      G_TYPE_NONE, 2,
				  G_TYPE_INT,
			      G_TYPE_OBJECT);
		g_signal_new ("load-session",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, load_session),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__INT_OBJECT,
			      G_TYPE_NONE, 2,
				  G_TYPE_INT,
			      G_TYPE_OBJECT);
		g_signal_new ("save-prompt",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, save_prompt),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      G_TYPE_OBJECT);
		initialized = TRUE;
	}
}

GType
anjuta_shell_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (AnjutaShellIface),
			anjuta_shell_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "AnjutaShell", 
					       &info,
					       0);
		
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	
	return type;			
}
