#ifndef TOOLS_H
#define TOOLS_H

#include <libanjuta/e-splash.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-plugin-description.h>

/* Initialization and finalization */
void anjuta_plugins_init (GList *plugin_dirs);
void anjuta_plugins_finalize (void);

/* Selection dialogs */
GtkWidget* anjuta_plugins_get_installed_dialog (AnjutaShell *shell);

/* Plugin queries based on meta-data */

const AnjutaPluginDescription*
anjuta_plugins_get_description (AnjutaShell *shell, GObject *plugin);

const AnjutaPluginDescription*
anjuta_plugins_get_description_by_id (AnjutaShell *shell,
									  const gchar *plugin_id);

/* Returns a list of plugin Descriptions */
GSList* anjuta_plugins_query (AnjutaShell *shell,
							 const gchar *section_name,
							 const gchar *attribute_name,
							 const gchar *attribute_value,
							 ...);

/* Returns the plugin description that has been selected from the list */
AnjutaPluginDescription* anjuta_plugins_select (AnjutaShell *shell, gchar *title,
												gchar *description,
												GSList *plugin_descriptions);
												
/* Returns the plugin that has been selected and activated */
GObject* anjuta_plugins_select_and_activate (AnjutaShell *shell, gchar *title,
											 gchar *description,
											 GSList *plugin_descriptions);

/* Plugin activation, deactivation and retrival */

GObject *anjuta_plugins_get_plugin (AnjutaShell *shell,
									const gchar *iface_name);

GObject *anjuta_plugins_get_plugin_by_id (AnjutaShell *shell,
										  const gchar *plugin_id);

gboolean anjuta_plugins_unload_plugin (AnjutaShell *shell, GObject *plugin);

gboolean anjuta_plugins_unload_plugin_by_id (AnjutaShell *shell,
											 const gchar *plugin_id);

void anjuta_plugins_unload_all (AnjutaShell *shell);

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
