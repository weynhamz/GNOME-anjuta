#ifndef TOOLS_H
#define TOOLS_H

#include <libanjuta/e-splash.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-plugin-description.h>

/* Initialization and finalization */
void anjuta_plugins_init (GList *plugin_dirs);
void anjuta_plugins_finalize (void);

/* Plugin loading and unloading */
GObject* anjuta_plugins_load (AnjutaShell *shell, const gchar *name);

gboolean anjuta_plugins_unload (AnjutaShell *shell, const gchar *name);

/* Selection dialogs */
GtkWidget* anjuta_plugins_get_installed_dialog (AnjutaShell *shell);

/* Plugin queries based on meta-data */

const AnjutaPluginDescription*
anjuta_plugins_get_description (AnjutaShell *shell, GObject *plugin);

const AnjutaPluginDescription*
anjuta_plugins_get_description_by_name (AnjutaShell *shell,
										const gchar *plugin_name);

GSList* anjuta_plugins_query (AnjutaShell *shell,
							 const gchar *section_name,
							 const gchar *attribute_name,
							 const gchar *attribute_value,
							 ...);

/* Plugin activation and retrival */

GObject *
anjuta_plugins_get_plugin (AnjutaShell *shell,
						   const gchar *iface_name);

/**
 * anjuta_plugins_get_interface:
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
#define anjuta_plugins_get_interface(shell, iface_type, error) \
	(((iface_type*) anjuta_plugins_get_plugin((shell), #iface_type, (error)))

#endif
