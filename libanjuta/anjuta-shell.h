#ifndef _ANJUTA_SHELL_H
#define _ANJUTA_SHELL_H

#include <glib-object.h>
#include <gtk/gtkwidget.h>
//#include <bonobo/bonobo-ui-container.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_SHELL            (anjuta_shell_get_type ())
#define ANJUTA_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SHELL, AnjutaShell))
#define ANJUTA_IS_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SHELL))
#define ANJUTA_SHELL_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ANJUTA_TYPE_SHELL, AnjutaShellIface))

#define ANJUTA_SHELL_ERROR anjuta_shell_error_quark()

typedef struct _AnjutaShell      AnjutaShell;
typedef struct _AnjutaShellIface AnjutaShellIface;
typedef enum   _AnjutaShellError AnjutaShellError;

struct _AnjutaShellIface {
	GTypeInterface g_iface;
	
	/* Signals */
	void (*value_added) (AnjutaShell *shell, char *name, GValue *value);
	void (*value_removed) (AnjutaShell *shell, char *name);
	void (*session_load) (AnjutaShell *shell);
	void (*session_save) (AnjutaShell *shell);

	/* Virtual Table */
	void (*add_widget)        (AnjutaShell  *shell,
							   GtkWidget    *widget,
							   const char   *name,
							   const char   *title,
							   GError      **error);
	void (*remove_widget)     (AnjutaShell  *shell,
							   GtkWidget    *widget,
							   GError      **error);
	void (*add_value)         (AnjutaShell  *shell,
							   const char   *name,
							   const GValue *value,
							   GError       **error);
	void (*get_value)         (AnjutaShell  *shell,
							   const char   *name,
							   GValue       *value,
							   GError       **error);
	void (*remove_value)      (AnjutaShell  *shell,
							   const char   *name,
							   GError       **error);
	GObject* (*get_object)    (AnjutaShell  *shell,
							   const char   *iface_name,
							   GError       **error);
};

enum AnjutaShellError {
	ANJUTA_SHELL_ERROR_DOESNT_EXIST,
};


GQuark anjuta_shell_error_quark     (void);
GType  anjuta_shell_get_type        (void);
void   anjuta_shell_add_widget      (AnjutaShell     *shell,
									 GtkWidget       *widget,
									 const char      *name,
									 const char      *title,
									 GError         **error);
void   anjuta_shell_remove_widget   (AnjutaShell     *shell,
									 GtkWidget       *widget,
									 GError         **error);
void   anjuta_shell_add_value       (AnjutaShell     *shell,
									 const char      *name,
									 const GValue    *value,
									 GError         **error);
void   anjuta_shell_add_valist      (AnjutaShell     *shell,
									 const char      *first_name,
									 GType            first_type,
									 va_list          var_args);
void   anjuta_shell_add             (AnjutaShell     *shell,
									 const char      *first_name,
									 GType            first_type,
									 ...);
void   anjuta_shell_get_value       (AnjutaShell     *shell,
									 const char      *name,
									 GValue          *value,
									 GError         **error);
void   anjuta_shell_get_valist      (AnjutaShell     *shell,
									 const char      *first_name,
									 GType            first_type,
									 va_list          var_args);
void   anjuta_shell_get             (AnjutaShell     *shell,
									 const char      *first_name,
									 GType            first_type,
									 ...);
void   anjuta_shell_remove_value    (AnjutaShell     *shell,
									 const char      *name,
									 GError         **error);

GObject*   anjuta_shell_get_object  (AnjutaShell     *shell,
									 const char      *iface_name,
									 GError         **error);

G_END_DECLS

#endif
