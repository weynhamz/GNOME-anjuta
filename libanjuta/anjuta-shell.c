#include <config.h>
#include "anjuta-shell.h"

#include <string.h>
/*
#include <bonobo/bonobo-control-frame.h>
#include <bonobo/bonobo-widget.h>
#include <bonobo/bonobo-window.h>
*/
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
#if 0
void
anjuta_shell_add_control       (AnjutaShell   *shell,
				Bonobo_Control ctrl,
				const char    *name,
				const char    *title,
				GError       **error)
{
	GtkWidget *widget;
	BonoboControlFrame *frame;
	
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (ctrl != CORBA_OBJECT_NIL);
	g_return_if_fail (name != NULL);
	g_return_if_fail (title != NULL);	

	widget = bonobo_widget_new_control_from_objref (ctrl,
							BONOBO_OBJREF (bonobo_window_get_ui_container (BONOBO_WINDOW (shell))));
	frame = bonobo_widget_get_control_frame (BONOBO_WIDGET (widget));
	
	bonobo_control_frame_set_autoactivate (frame, FALSE);
	bonobo_control_frame_control_activate (frame);
	
	gtk_widget_show (widget);

	anjuta_shell_add_widget (shell, widget, name, title, error);
}

void
anjuta_shell_add_preferences (AnjutaShell *shell,
			      GtkWidget *page,
			      const char *name,
			      const char *title,
			      GdkPixbuf *pixbuf,
			      GError **error)
{
	g_return_if_fail (shell != NULL);
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (page != NULL);
	g_return_if_fail (GTK_IS_WIDGET (page));
	g_return_if_fail (name != NULL);
	g_return_if_fail (title != NULL);
	g_return_if_fail (pixbuf != NULL);
	g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
	g_return_if_fail (title != NULL);

	ANJUTA_SHELL_GET_IFACE (shell)->add_preferences (shell, 
							 page, 
							 name,
							 title, 
							 pixbuf,
							 error);
}

#endif

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
			      anjuta_marshal_VOID__STRING_BOXED,
			      G_TYPE_NONE, 2,
			      G_TYPE_STRING, G_TYPE_VALUE);
		
		g_signal_new ("value_removed",
			      ANJUTA_TYPE_SHELL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaShellIface, value_added),
			      NULL, NULL,
			      anjuta_marshal_VOID__STRING,
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
