/*
** menu-registry.h: Helper functions for registering and getting submenus.
** Author: Biswapesh Chattopadhyay
*/

#ifndef _MENU_REGISTRY_H
#define _MENU_REGISTRY_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
** Associate a submenu with a name. Tools can query qith a name to get
** the corresponding submenu. Once a submenu widget is obtained, tools
** can add their own menu items to the submenu. Ideally, we should generalize
** this so that all widgets are registered, but this will do for now.
*/
void an_register_submenu(const gchar *name, GtkWidget *submenu);

/* Given a name, returns the corresponding submenu. If a submenu corresponding
** to the given name does not exist, then a search is make for the default
** submenu (registered with an_register_submenu() with name=NULL). If
** this is unsuccessful as well, NULL is returned. If NULL is passed as
** parameter, the default submenu (if any) is returned.
*/
GtkWidget *an_get_submenu(const gchar *name);

#ifdef __cplusplus
}
#endif

#endif /* _MENU_REGISTRY_H */
