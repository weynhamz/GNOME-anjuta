#include <config.h>
#include "anjuta-shell.h"

#include <string.h>
#include <gobject/gvaluecollector.h>
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
 * @title: Translated string which is displayed along side the widget when
 * required (eg. as window title or notebook tab label).
 * @error: Error propagation object.
 *
 * Adds @widget in the shell. The place where the widget will appear depends
 * on the implementor of this interface. Generally it will be a container
 * (dock, notebook, GtkContainer etc.).
 */
void
anjuta_shell_add_widget (AnjutaShell *shell,
			 GtkWidget *widget,
			 const char *name,
			 const char *title,
			 GError **error)
{
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (name != NULL);
	g_return_if_fail (title != NULL);

	ANJUTA_SHELL_GET_IFACE (shell)->add_widget (shell, widget, name,
						    title, error);
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
 * @error: Error propagation object.
 * returns: The plugin object (subclass of #AnjutaPlugin) which implements
 * the given interface. See #AnjutaPlugin for more detail on interfaces
 * implemented by plugins.
 * 
 * Searches the currently loaded plugin objects to find the one which
 * implements the given interface as primary interface and returns it.
 * The returned object is garanteed to be an implementor of the
 * interface (as exported by the plugin metafile). It only searches
 * from the pool of plugin objects loaded in this shell and can only search
 * by primary interface. If there are more objects implementing this primary
 * interface, user might be prompted to select one from them (and might give
 * the option to use it as default for future queries). A typical usage of this
 * function is:
 * <programlisting>
 * GObject *docman =
 *     anjuta_shell_get_object (shell, "IAnjutaDocumentManager", error);
 * </programlisting>
 * Notice that this function takes the interface name string as string, unlike
 * anjuta_shell_get_interface() which takes the type directly.
 *
 * @error is set to ANJUTA_SHELL_ERROR_DOESNT_EXIST, if a plugin implementing
 * the interface is not found and NULL is returned.
 *
 * Return value: A plugin object implementing the primary interface or NULL.
 */
GObject*
anjuta_shell_get_object     (AnjutaShell     *shell,
							 const char      *iface_name,
							 GError         **error)
{
	g_return_val_if_fail (shell != NULL, NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);
	g_return_val_if_fail (iface_name != NULL, NULL);

	return ANJUTA_SHELL_GET_IFACE (shell)->get_object (shell, iface_name, error);
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

		g_signal_new ("session_load",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, session_load),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0,
			      NULL);
		g_signal_new ("session_save",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, session_save),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0,
			      NULL);
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
