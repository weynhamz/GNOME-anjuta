/*
    CORBA-Server.c
	Corba Server Implementation for Anjuta
    Copyright (C) 2001  LB

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "support.h"
#include "anjuta.h"
#include "CORBA-Server.h"

#include <orb/orbit.h>
#include <libgnorba/gnorba.h>
//typedef CORBA_Object Gladen_GladeRef;

#define	SKEL_IMPL
#include "Prj-skelimpl.c"
#include "Prj-skels.c"
#include "Prj-stubs.c"

#include "glades.h"

static gboolean
ImportFileInProject ( const gchar * p_szModule, const gchar *p_szFileName );
static guint
final_setup_from_main_loop (gpointer data);


/*---------------------------------------------------------------------*/

void CInitEx( CORBA_Environment *pev )
{
	CORBA_exception_init( pev );
}

/*---------------------------------------------------------------------*/

C_ServerObj *C_ServerObjNew()
{
	C_ServerObj *self = g_new0( C_ServerObj, 1 );
	if( NULL != self )
	{
		C_ServerObjInit(self);
	}
	return self;
}

void C_ServerObjInit(C_ServerObj *self)
{
	self->m_pORB	= NULL ;
	self->m_SrvObj	= NULL ;
	self->m_pSrv	= NULL ;
	self->m_SrvStr	= NULL ;
	self->m_pobjid	= NULL ;
	self->m_pName	= NULL ;
	self->m_pDelete	= NULL ;
}

void C_ServerObjSetORB(C_ServerObj *self, CORBA_ORB *pORB )
{
	self->m_pORB	= pORB ;
}

void C_ServerObjSet( C_ServerObj *self, void *pSrv, C_S_Del_F pDelete, 
					PortableServer_ObjectId *poid, CORBA_Object pSrvObj, gchar *pName )
{
	self->m_SrvObj	= pSrvObj ;
	self->m_pSrv	= pSrv ;
	if( NULL != pSrv )
	{
		CORBA_Environment	ev ;
		self->m_SrvStr	= CORBA_ORB_object_to_string( *self->m_pORB, pSrvObj, &ev );
	}

	self->m_pobjid	= poid ;
	self->m_pName	= g_strdup( pName );
	self->m_pDelete	= pDelete ;
}

void C_ServerObjDelete(C_ServerObj *self, const gboolean bFree )
{
	CORBA_Environment ev;
	CInitEx( &ev );
	if( NULL != self->m_pDelete )
		(*self->m_pDelete)( self->m_pSrv, &ev );
	self->m_pSrv	= NULL ;
	
	self->m_SrvObj	= NULL ;
	CORBA_free(self->m_SrvStr) ;
	CORBA_free(self->m_pobjid);
	g_free(self->m_pName);
	if( bFree )
	{
		g_free( self );
	}
}

/*---------------------------------------------------------------------*/

/*
se devo creare piu' oggetti devo utilizzare una sola corba_bind e piu' create
*/


gboolean LocalServerInit(LocalServer *self, int *pargc, char **argv, CORBA_ORB corb )
{
	C_ServerObjInit( &self->m_serv );
	self->m_FnSetup	= 0 ;
	
	self->m_gladen = NULL ;
	CInitEx( EV() );
	self->m_orb	= corb ;
	/*gnome_CORBA_init( "Anjuta Server", VERSION, pargc, argv, 
								GNORBA_INIT_SERVER_FUNC, EV() );*/
	CHKEV();
	self->m_root_poa = CORBA_ORB_resolve_initial_references( self->m_orb, "RootPOA", EV() );
	CHKEV();
	self->root_poa_manager = 
			PortableServer_POA__get_the_POAManager( 
							(PortableServer_POA)self->m_root_poa, EV() );
	CHKEV();
	PortableServer_POAManager_activate( self->root_poa_manager, EV()) ;
	CHKEV();
	C_ServerObjSetORB( &self->m_serv, &self->m_orb );
	/* Start !!!! */
	impl_ProjectManager_PrjMan__create( (PortableServer_POA)self->m_root_poa, 
							EV(), &self->m_serv );
	CHKEV();
	/*self->name_server	=	gnome_name_service_get();	
	goad_server_register( name_server, self->m_Server, 
							goad_server_activation_id(), "AnjutaServer", EV());*/
	return TRUE;
}

void LocalServerEnd( LocalServer *self )
{
	CInitEx( EV() );
	if( self->m_gladen )
	{
		Gladen_GladeRef_Exit(self->m_gladen, EV() );
		CORBA_Object_release( self->m_gladen, EV() );
	}
	self->m_gladen = NULL ;
	C_ServerObjDelete(&self->m_serv, FALSE );
	CORBA_ORB_shutdown(self->m_orb, CORBA_FALSE, &self->m_ev);
}


#define	GLOBAL_SERVER
#ifdef	GLOBAL_SERVER
/* For debug .... */
static LocalServer	g_Server ;
LocalServer *GetCorbaManager(void)
{
	return &g_Server;
}

#else
#define	GetCorbaManager()	&l_Server
#endif

int
MainLoop(int *pargc, char *argv[], CORBA_ORB corb )
{
#ifndef	GLOBAL_SERVER
	LocalServer	l_Server ;
#endif
	if( LocalServerInit( GetCorbaManager(), pargc, argv, corb ) )
	{
		GetCorbaManager()->m_FnSetup = gtk_idle_add ((GtkFunction) final_setup_from_main_loop, NULL);
		gtk_main ();
		LocalServerEnd( GetCorbaManager() );
		return 0;
	} else
		MessageBox( _("CORBA Module not Initialized") );
	return -1;

}

static guint
final_setup_from_main_loop (gpointer data)
{
	gladen_start();
	gtk_idle_remove( GetCorbaManager()->m_FnSetup );
	return FALSE;
}


#if 0
PortableServer_ObjectId *C_GetObjectID( servant, poa )
{
	PortableServer_ObjectId *objid;
	CORBA_Environment		l_ev;	
	
	CInitEx( &l_ev );
	objid = PortableServer_POA_servant_to_id(servant->poa, servant, &l_ev);
	if( CHECK_CORBA_E(&l_ev) )
		return objid ;
	else
		return NULL ;
}

C_DeleteObjID( PortableServer_ObjectId *objid )
{
	if( NULL != objid )
		CORBA_free(objid);
}
#endif

void MessageBox( const char* szMsg )
{
	GtkWidget * msgBox = 

   /* Create the dialog using a couple of stock buttons */
   gnome_message_box_new ( szMsg, 
             GNOME_MESSAGE_BOX_QUESTION,
             GNOME_STOCK_BUTTON_OK,
             NULL );
   gnome_dialog_run_and_close (GNOME_DIALOG (msgBox ));
}


ProjectManager_PrjMan
impl_ProjectManager_PrjMan__create(PortableServer_POA poa,
				   CORBA_Environment * ev, C_ServerObj *pS )
{
   ProjectManager_PrjMan retval;
   impl_POA_ProjectManager_PrjMan *newservant;
   PortableServer_ObjectId *objid;

   newservant = g_new0(impl_POA_ProjectManager_PrjMan, 1);
   newservant->servant.vepv = &impl_ProjectManager_PrjMan_vepv;
   newservant->poa = poa;
   POA_ProjectManager_PrjMan__init((PortableServer_Servant) newservant, ev);
   objid = PortableServer_POA_activate_object(poa, newservant, ev);
   //CORBA_free(objid);
   retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
	C_ServerObjSet( pS, newservant, (C_S_Del_F)&impl_ProjectManager_PrjMan__destroy, 
							objid, retval, "AnjutaServer" );
   return retval;
}

static void
impl_ProjectManager_PrjMan__destroy(impl_POA_ProjectManager_PrjMan * servant,
				    CORBA_Environment * ev)
{
   PortableServer_ObjectId *objid;

   objid = PortableServer_POA_servant_to_id(servant->poa, servant, ev);
   PortableServer_POA_deactivate_object(servant->poa, objid, ev);
   CORBA_free(objid);

   POA_ProjectManager_PrjMan__fini((PortableServer_Servant) servant, ev);
   g_free(servant);
}

CORBA_char *
impl_ProjectManager_PrjMan__get_srcLanguage(impl_POA_ProjectManager_PrjMan *
					    servant, CORBA_Environment * ev)
{
   CORBA_char *retval;
	retval = CORBA_string_dup( "C" );
	MessageBox("get_srcLanguage");	
   return retval;
}

CORBA_char *
impl_ProjectManager_PrjMan__get_projectName(impl_POA_ProjectManager_PrjMan *
					    servant, CORBA_Environment * ev)
{
   CORBA_char *retval;
	retval = CORBA_string_dup( "TBD ugo" );
	MessageBox("get_projectName");
   return retval;
}

CORBA_boolean
impl_ProjectManager_PrjMan_AddFileToProject(impl_POA_ProjectManager_PrjMan *
					    servant, CORBA_char * szCategory,
					    CORBA_char * szFileName,
					    CORBA_Environment * ev)
{
   CORBA_boolean retval;
	/* debug ... 
	MessageBox("AddFileToProject");
	MessageBox(szFileName);	*/
	if( ImportFileInProject ( szCategory, szFileName ) )
		retval = CORBA_TRUE ;
	else
		retval = CORBA_FALSE ;
   return retval;
}

CORBA_boolean
impl_ProjectManager_PrjMan_RemFileFromProject(impl_POA_ProjectManager_PrjMan *
					      servant,
					      CORBA_char * szCategory,
					      CORBA_char * szFileName,
					      CORBA_Environment * ev)
{
   CORBA_boolean retval;
	MessageBox("RemFileFromProject");
	retval = CORBA_FALSE;
   return retval;
}

CORBA_boolean
impl_ProjectManager_PrjMan_SetClassInfo(impl_POA_ProjectManager_PrjMan *
					servant, CORBA_char * szClass,
					CORBA_char * szFileDecl,
					CORBA_char * szFileImpl,
					CORBA_Environment * ev)
{
   CORBA_boolean retval;
	MessageBox("SetClassInfo");
	retval = CORBA_FALSE;
   return retval;
}

/* The file name is .. ?? absolute ???? 
static gboolean
ImportFileInProject ( const gchar * p_szModule, const gchar *p_szFileName )
{
	gchar	*dir, *comp_dir ;
	ProjectDBase * pProject = app-> project_dbase ;

	dir = g_dirname (p_szFileName);
	comp_dir = project_dbase_get_module_dir ( p, p->sel_module );
	lo saltiamio !!!!

	if (strcmp ( dir, comp_dir) == 0 )
		on_prj_import_confirm_yes (NULL, user_data);

	g_free (dir);
	g_free (comp_dir);
}
*/
// By default source !

static PrjModule
project_get_module( ProjectDBase * p, const gchar *szModule )
{
	gint iModule;

	if (NULL!=p)
	{
		for ( iModule = 0 ; iModule < MODULE_END_MARK; iModule++)
		{
			if( !g_strcasecmp( szModule, module_map[iModule] ) )
			{
				return (PrjModule) iModule ;
			}
		}
	}
	return MODULE_SOURCE ;
}

/*
gchar *
project_dbase_get_source_target (ProjectDBase * p)
{
	gchar *str, *src_dir, *target;

	if (!p)
		return NULL;
	if (p->project_is_open == FALSE)
		return NULL;
	target = prop_get (p->props, "project.source.target");
	src_dir = project_dbase_get_module_dir (p, MODULE_SOURCE);
	str = g_strconcat (src_dir, "/", target, NULL);
	g_free (target);
	g_free (src_dir);
	return str;
}
*/


// absolute FilePath 

static gboolean
ImportFileInProject ( const gchar * p_szModule, const gchar *p_szFileName )
{
	gchar		*dir, *comp_dir, *fn, *filename ;
	GList		*list, *mod_files;
	ProjectDBase *pPrj = app-> project_dbase ;
	PrjModule	selMod ;
	
	filename = g_strdup( p_szFileName );
	selMod = project_get_module( pPrj, p_szModule );

	dir = g_dirname (filename);

	comp_dir	= project_dbase_get_module_dir ( pPrj, selMod );
	mod_files	= project_dbase_get_module_files ( pPrj, selMod );
	list		= mod_files;
	while (list)
	{
		if (strcmp (extract_filename (filename), list->data) == 0)
		{
			/*
			 * file has already been added. So skip 
			 */
			g_free (dir);
			g_free (filename);
			glist_strings_free (list);
			return TRUE ;
		}
		list = g_list_next (list);
	}
	glist_strings_free (list);
	/*
	 * File has not been added. So add it 
	 */
	if (strcmp ( dir, comp_dir) != 0)
	{
		/*
		 * File does not exist in the corrospondig dir. So, import it. 
		 */
		fn = g_strconcat (comp_dir, "/",
				     extract_filename (filename), NULL);
		force_create_dir (comp_dir);
		if (!copy_file (filename, fn, TRUE))
		{
			g_free (dir);
			g_free (fn);
			messagebox (GNOME_MESSAGE_BOX_INFO,
				    _("Error while copying the file inside the module."));
			return FALSE ;
		}
		filename = fn;
	}
	else
		fn = g_strdup (filename);	/* Just to make the control flow easy */

	project_dbase_add_file_to_module ( pPrj , selMod, filename );
	g_free (filename);
	g_free (fn);
	g_free (dir);
	g_free (comp_dir);
	return TRUE ;
}

void
impl_ProjectManager_PrjMan_SetGladeRef(impl_POA_ProjectManager_PrjMan *
				       servant, Gladen_GladeRef pgf,
				       CORBA_Environment * ev)
{
	CORBA_Environment		l_ev;	
	
	CInitEx( &l_ev );
	GetCorbaManager()->m_gladen =  pgf ;
	/* Sara' necessario ? mah.... */
	if( NULL != pgf )
		CORBA_Object_duplicate( GetCorbaManager()->m_gladen, &l_ev );
	//Gladen_GladeRef_AddComponent((Gladen_GladeRef) pgf, "GnomeApp", "WndMain", &l_ev );
}


gboolean IsGladen(void)
{
	return ( GetCorbaManager()->m_gladen != NULL ) ? TRUE : FALSE ;
}

gboolean
CheckString( const gchar *szString )
{
	if( NULL == szString )
		return FALSE ;
	if( strlen( szString ) )
		return TRUE;
	else
		return FALSE;
}

void
impl_ProjectManager_PrjMan_SaveFileIfModified(impl_POA_ProjectManager_PrjMan *
					      servant,
					      CORBA_char * szFullPath,
					      CORBA_Environment * ev)
{
	if( CheckString( szFullPath ) )
		anjuta_save_file_if_modified( szFullPath );
}

void
impl_ProjectManager_PrjMan_ReloadFile(impl_POA_ProjectManager_PrjMan *
				      servant, CORBA_char * szFullPath,
				      CORBA_Environment * ev)
{
	if( CheckString( szFullPath ) )
		anjuta_reload_file( szFullPath );

}
