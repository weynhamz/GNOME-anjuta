/*
** User customizable tools implementation
** Author: Biswapesh Chattopadhyay
*/

#ifndef _ANJUTA_TOOLS_H
#define _ANJUTA_TOOLS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Loads all global and local tools. Call at startup after loading the GUI */
gboolean anjuta_tools_load(void);

/* Saves all global and local tools. Call at shutdown */
gboolean anjuta_tools_save(void);

/* Starts the tool editor */
gboolean anjuta_tools_edit(void);

/* Loads all tools for the project. Call when a new project is loaded */
gboolean anjuta_project_tools_load(gchar *dir);

/* Unload all tools specific to the project. Call when a project is closed */
gboolean anjuta_project_tools_unload(void);

/* Show/hide tools based on whether project/buffer is loaded, etc. Call
whenever a project gets loaded/unloaded or the first file is loaded or
when the last file is closed */
void anjuta_tools_sensitize(void);

#ifdef __cplusplus
}
#endif

#endif /* _ANJUTA_TOOLS_H */
