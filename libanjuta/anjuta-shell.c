#include <config.h>
#include <string.h>
#include <gobject/gvaluecollector.h>
#include "anjuta-shell.h"
#include "anjuta-marshal.h"

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
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (name != NULL);
	g_return_if_fail (title != NULL);

	ANJUTA_SHELL_GET_IFACE (shell)->add_widget (shell, widget, name,
												title, stock_id,
												placement, error);
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
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

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
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

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

static void
anjuta_shell_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	
	if (!initialized) {
		g_signal_new ("value_added",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, value_added),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__STRING_BOXED,
			      G_TYPE_NONE, 2,
			      G_TYPE_STRING, G_TYPE_VALUE);
		
		g_signal_new ("value_removed",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, value_removed),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
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
