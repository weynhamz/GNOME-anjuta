#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <syslog.h>

#include <gnome.h>
#include "anjuta.h"
#include "widget-registry.h"
#include "anjuta-plugins.h"

#define	LOCALS_PLUGIN_DIR	"/.anjuta/plugins"

static AnjutaPlugIn *plug_in_new(void)
{
	AnjutaPlugIn *self = g_new0(AnjutaPlugIn, 1);
	return self ;
}

static void plug_in_free(AnjutaPlugIn *self)
{
	g_return_if_fail(self);
	if( self->m_Handle && self->CleanUp )
		(*self->CleanUp)( self->m_Handle, self->m_UserData, app );
	if(self->m_szModName)
		g_free(self->m_szModName);
	if( self->m_Handle )
		g_module_close( self->m_Handle );
	g_free( self );
}

static void plug_in_callback(GtkWidget *menu_item, gpointer data)
{
	AnjutaPlugIn *plugin = (AnjutaPlugIn *) data;
	g_return_if_fail(plugin && plugin->Activate);

	plugin->Activate(plugin->m_Handle, plugin->m_UserData, app);
}

static AnPlugErr plug_in_init(AnjutaPlugIn *self, const gchar *szModName)
{
	AnPlugErr pieError = PIE_OK ;
	g_return_val_if_fail(self, PIE_BADPARMS );
	g_return_val_if_fail(szModName && strlen(szModName), PIE_BADPARMS);
	self->m_szModName = g_strdup(szModName) ;
	if (NULL != (self->m_Handle = g_module_open(szModName, 0)))
	{
		/* Symbol load */
		if (
		  g_module_symbol(self->m_Handle, "GetDescr", (gpointer*)&self->GetDescr)
		  && g_module_symbol(self->m_Handle, "GetVersion", (gpointer*)&self->GetVersion)
		  && g_module_symbol(self->m_Handle, "Init", (gpointer*)&self->Init)
		  && g_module_symbol(self->m_Handle, "CleanUp", (gpointer*)&self->CleanUp)
		  && g_module_symbol(self->m_Handle, "Activate", (gpointer*)&self->Activate)
		  && g_module_symbol(self->m_Handle, "GetMenuTitle", (gpointer*)&self->GetMenuTitle)
		  && g_module_symbol(self->m_Handle, "GetTooltipText", (gpointer*)&self->GetTooltipText)
		)
		{
			self->m_bStarted = (*self->Init)(self->m_Handle, &self->m_UserData, app);
			if( !self->m_bStarted )
			{
				pieError = PIE_INITFAILED;
			}
			else
			{
				GtkWidget *submenu;
				gchar *szMenu = NULL;
				g_module_symbol(self->m_Handle, "GetMenu", (gpointer*)&self->GetMenu);
				if (NULL != self->GetMenu)
					szMenu = (*self->GetMenu)();
				submenu = an_get_submenu(szMenu);
				if (szMenu)
					g_free(szMenu);
				if (submenu)
				{
					gchar *szTitle = (*self->GetMenuTitle)(self->m_Handle
					  , self->m_UserData ) ;
					self->menu_item = gtk_menu_item_new_with_label(
					  szTitle);
					gtk_widget_ref(self->menu_item);
					g_free(szTitle);
					gtk_menu_append(GTK_MENU(submenu), self->menu_item);
					gtk_widget_show(self->menu_item);
					gtk_signal_connect(GTK_OBJECT(self->menu_item), "activate"
					  , plug_in_callback, self);
				}
			}
		} else
		{
			self->GetDescr		= NULL ;
			self->GetVersion	= NULL ;
			self->Init			= NULL ;
			self->CleanUp		= NULL ;
			self->Activate		= NULL ;
			self->GetMenuTitle	= NULL ;
			self->m_bStarted	= FALSE ;
			pieError = PIE_SYMBOLSNOTFOUND ;
		}
	} else
		pieError = PIE_NOTLOADED ;
	return pieError ;
}

static gboolean
Load_plugIn(AnjutaPlugIn *self, const gchar *szModName)
{
	gboolean bOK = FALSE;
	AnPlugErr lErr = plug_in_init(self, szModName);
	const gchar	*pErr = g_module_error();
	if( NULL == pErr )
		pErr = "";
	switch( lErr )
	{
	default:
		syslog( LOG_WARNING|LOG_USER, _("Unable to load plugin %s. Error: %s"), szModName, pErr );
		break;
	case PIE_NOTLOADED:
		syslog( LOG_WARNING|LOG_USER, _("Unable to load plugin %s. Error: %s"), szModName, pErr );
		break;
	case PIE_SYMBOLSNOTFOUND:
		syslog( LOG_WARNING|LOG_USER, _("Plugin protocol %s unknown Error:%s"), szModName, pErr );
		break;
	case PIE_INITFAILED:
		syslog( LOG_WARNING|LOG_USER, _("Unable to start plugin %s"), szModName );
		break;
	case PIE_BADPARMS:
		syslog( LOG_WARNING|LOG_USER, _("Bad parameters to plugin %s. Error: %s"), szModName, pErr );
		break;
	case PIE_OK:
		bOK = TRUE ;
		break;
	}
	if( ! bOK )
		anjuta_error(_("Unable to load plugin %s.\nError: %s"), szModName, pErr);
	return bOK ;
}

static void
plugins_foreach_delete( gpointer data, gpointer user_data )
{
	plug_in_free((AnjutaPlugIn *)data);
}

static void
free_plug_ins(GList * pList)
{
	if( pList )
	{
		g_list_foreach( pList, plugins_foreach_delete, NULL );
		g_list_free (pList);
	}
}

static int
select_all_files (const struct dirent *e)
{
	return TRUE; 
}

static GList *
scan_AddIns_in_directory (AnjutaApp* pApp, const gchar *szDirName, GList *pList)
{
	GList *files;
	GList *node;

	if (!g_module_supported ())
		return NULL ;
	
	g_return_val_if_fail (pApp != NULL, pList);
	g_return_val_if_fail (((NULL != szDirName) && strlen(szDirName)), pList);

	files = NULL;

	/* Can not use p->top_proj_dir yet. */

	/* Can not use project_dbase_get_module_dir() yet */
	files = scan_files_in_dir (szDirName, select_all_files);

	node = files;
	while (node)
	{
		const char* entry_name;
		gboolean bOK = FALSE;

		entry_name = (const char*) node->data;
		node = g_list_next (node);

		if ((NULL != entry_name) && strlen (entry_name) > 3
			&& (0 == strcmp (&entry_name[strlen(entry_name)-3], ".so")))
		{
			/* FIXME: e questo ? bah library_file = g_module_build_path (module_dir, module_file); */
			gchar *pPIPath = g_strdup_printf ("%s/%s", szDirName, entry_name);
			AnjutaPlugIn *pPlugIn = plug_in_new();

			if ((NULL != pPlugIn) && (NULL != pPIPath))
			{
				if (Load_plugIn(pPlugIn, pPIPath))
				{
					pList = g_list_prepend (pList, pPlugIn);
					bOK = TRUE ;
				}
			} else
				anjuta_error (_("Out of memory scanning for plugins."));

			g_free (pPIPath);

			if (!bOK && (NULL != pPlugIn))
				plug_in_free(pPlugIn);
		}
	}

	free_string_list (files);

	return pList;
}

gboolean anjuta_plugins_load(void)
{
	gchar *plugin_dir;
	char const * const home_dir = getenv ("HOME");
	app->addIns_list = scan_AddIns_in_directory(app, PACKAGE_PLUGIN_DIR, NULL);
	/* Load the user plugins */
	if (home_dir != NULL)
	{
		plugin_dir = g_strconcat (home_dir, LOCALS_PLUGIN_DIR, NULL);
		app->addIns_list = scan_AddIns_in_directory(app, plugin_dir
		  , app->addIns_list );
		g_free (plugin_dir);
	}
	return TRUE;
}

gboolean anjuta_plugins_unload(void)
{
	free_plug_ins(app->addIns_list);
	app->addIns_list = NULL;
}
