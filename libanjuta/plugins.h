#ifndef TOOLS_H
#define TOOLS_H

#include <libanjuta/e-splash.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-preferences.h>

void anjuta_plugins_init (GList *plugin_dirs);
void anjuta_plugins_finalize (void);
void anjuta_plugins_load (AnjutaShell *shell, 
						  AnjutaUI *ui, AnjutaPreferences *prefs,
						  ESplash *splash, const char *name);
gboolean anjuta_plugins_unload (AnjutaShell *shell);

GObject *anjuta_plugins_get_object (AnjutaShell *shell,
									const gchar *interface_name);

GtkWidget* anjuta_plugins_get_installed_dialog (AnjutaShell *shell,
												AnjutaUI *ui,
												AnjutaPreferences *prefs);

#endif
