#ifndef _ANJUTA_SHELL_H
#define _ANJUTA_SHELL_H

#include <glib-object.h>
#include <gtk/gtkwidget.h>

#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-preferences.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_SHELL            (anjuta_shell_get_type ())
#define ANJUTA_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SHELL, AnjutaShell))
#define ANJUTA_IS_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SHELL))
#define ANJUTA_SHELL_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ANJUTA_TYPE_SHELL, AnjutaShellIface))

#define ANJUTA_SHELL_ERROR anjuta_shell_error_quark()

typedef struct _AnjutaShell      AnjutaShell;
typedef struct _AnjutaShellIface AnjutaShellIface;

typedef enum
{
	ANJUTA_SHELL_ERROR_DOESNT_EXIST,
} AnjutaShellError;

typedef enum
{
	ANJUTA_SHELL_PLACEMENT_NONE = 0,
	ANJUTA_SHELL_PLACEMENT_TOP,
	ANJUTA_SHELL_PLACEMENT_BOTTOM,
	ANJUTA_SHELL_PLACEMENT_RIGHT,
	ANJUTA_SHELL_PLACEMENT_LEFT,
	ANJUTA_SHELL_PLACEMENT_CENTER,
	ANJUTA_SHELL_PLACEMENT_FLOATING
} AnjutaShellPlacement;
	
struct _AnjutaShellIface {
	GTypeInterface g_iface;
	
	/* Signals */
	void (*value_added) (AnjutaShell *shell, char *name, GValue *value);
	void (*value_removed) (AnjutaShell *shell, char *name);

	/* Virtual Table */
	AnjutaUI* (*get_ui) (AnjutaShell  *shell, GError **err);
	AnjutaPreferences* (*get_preferences) (AnjutaShell *shell, GError **err);
	
	void (*add_widget)        (AnjutaShell  *shell,
							   GtkWidget    *widget,
							   const char   *name,
							   const char   *title,
							   AnjutaShellPlacement placement,
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

GQuark anjuta_shell_error_quark     (void);
GType  anjuta_shell_get_type        (void);

AnjutaUI* anjuta_shell_get_ui (AnjutaShell *shell, GError **err);

AnjutaPreferences* anjuta_shell_get_preferences (AnjutaShell *shell,
												 GError **err);

void   anjuta_shell_add_widget      (AnjutaShell     *shell,
									 GtkWidget       *widget,
									 const char      *name,
									 const char      *title,
									 AnjutaShellPlacement placement,
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

GObject *anjuta_shell_get_object (AnjutaShell *shell,
								  const gchar *iface_name,
								  GError **error);
/**
 * anjuta_shell_get_interface:
 * @shell: A #AnjutaShell object
 * @iface_type: The interface type implemented by the object to be found
 * @error: Error propagation object.
 *
 * Equivalent to anjuta_shell_get_object(), but additionally typecasts returned
 * object to the interface type. It also takes interface type directly. A
 * usage of this function is:
 * <programlisting>
 * IAnjutaDocumentManager *docman =
 *     anjuta_shell_get_interface (shell, IAnjutaDocumentManager, error);
 * </programlisting>
 */
#define anjuta_shell_get_interface(shell, iface_type, error) \
	((iface_type*) anjuta_shell_get_object((shell), #iface_type, (error)))

G_END_DECLS

#endif
