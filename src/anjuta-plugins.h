#ifndef _ANJUTA_PLUGINS_H
#define _ANJUTA_PLUGINS_H

#include <gmodule.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef	enum _AnPlugErr
{
	PIE_OK,
	PIE_NOTLOADED,
	PIE_SYMBOLSNOTFOUND,
	PIE_INITFAILED,
	PIE_BADPARMS,
} AnPlugErr;

typedef struct _AnjutaPlugIn
{
	GModule *m_Handle;
	gchar *m_szModName;
	gboolean m_bStarted;	/* Flag successfuly initialized */
	void *m_UserData;	/* user data */
	/* Get module description */
	gchar	*(*GetDescr)();
	/* Get the menu under whcih the plugin should go */
	gchar * (*GetMenu)();
	/* GetModule Version hi/low word 1.02 0x10002 */
	glong	(*GetVersion)();
	/* Init Module */
	gboolean (*Init)( GModule *self, void **pUserData, AnjutaApp* p );
	/* Clean-up */
	void (*CleanUp)( GModule *self, void *pUserData, AnjutaApp* p );
	/* Activation */
	void (*Activate)( GModule *self, void *pUserData, AnjutaApp* p);
	/* Menu title */
	gchar *(*GetMenuTitle)( GModule *self, void *pUserData );
	/* Tooltip Text */
	gchar *(*GetTooltipText)( GModule *self, void *pUserData );
	/* Menu item - populated by the plugin loader */
	GtkWidget *menu_item;
} AnjutaPlugIn;

gboolean anjuta_plugins_load(void);
gboolean anjuta_plugins_unload(void);

#ifdef __cplusplus
}
#endif

#endif /* _ANJUTA_PLUGINS_H */
