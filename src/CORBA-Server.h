/*
    CORBA-Server.h
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


#ifndef	ANJ_CORBA_SERVER_H
#define	ANJ_CORBA_SERVER_H

#ifdef USE_GLADEN

#include <libgnorba/gnorba.h>
#include "Gladen.h"
#include "Prj.h"

/*----------------------------------------------------------------------*/
// Utilities
#define	EV()	&self->m_ev
#define	EVV()	self->m_ev
#define	CHK_EV(ev)	if( ev._major != CORBA_NO_EXCEPTION) { /*MessageBoxs apsoj d'akl; */return FALSE ; }
#define	CHKEV()	CHK_EV(EVV())


void CInitEx( CORBA_Environment *pev );

/*----------------------------------------------------------------------*/

typedef	void (*C_S_Del_F)( void*, void*) ;	/* Delete function to be called automatically */

/* Corba server object for retrieval */
typedef struct {
	CORBA_ORB				*m_pORB;		/* Global ORB */
	CORBA_Object			m_SrvObj;		/* Corba object */
	void					*m_pSrv; 		/* real ptr */
	CORBA_char				*m_SrvStr;		/* String rappresentation of the object */
	PortableServer_ObjectId *m_pobjid;
	gchar					*m_pName ;		/* User defined name */
	//void (*m_pDelete)( void*, void*) ;	/* Delete function to be called automatically */
	C_S_Del_F				m_pDelete;
} C_ServerObj ;

C_ServerObj *C_ServerObjNew(void);
void C_ServerObjInit(C_ServerObj *self);
void C_ServerObjSetORB(C_ServerObj *self, CORBA_ORB *pORB );
void C_ServerObjDelete(C_ServerObj *self, const gboolean bFree );
void C_ServerObjSet( C_ServerObj *self, void *pSrv, C_S_Del_F pDelete, 
					PortableServer_ObjectId *poid, CORBA_Object pSrvObj, gchar *pName );

/*----------------------------------------------------------------------*/

typedef struct t_LocalServer {
	/* Global CORBA Stuff */
	CORBA_ORB					m_orb;
	CORBA_Object				m_root_poa;
	PortableServer_POAManager	root_poa_manager;
	CORBA_Environment			m_ev;
	CORBA_Object				m_gladen;
	/*-------------------------------*/	
	C_ServerObj 				m_serv;
	guint						m_FnSetup;		/* One time initialization		*/
} LocalServer ;

LocalServer *GetCorbaManager(void);
gboolean IsGladen(void);
gboolean LocalServerInit(LocalServer *self, int *pargc, char **argv, CORBA_ORB corb );
void LocalServerEnd( LocalServer *self );


/*----------------------------------------------------------------------*/

typedef struct
{
   POA_ProjectManager_PrjMan	servant;
   PortableServer_POA			poa;
	
   CORBA_char *attr_srcLanguage;

   CORBA_char *attr_projectName;

}
impl_POA_ProjectManager_PrjMan;

ProjectManager_PrjMan
impl_ProjectManager_PrjMan__create(PortableServer_POA poa,
				   CORBA_Environment * ev, C_ServerObj *pS );


void
impl_ProjectManager_PrjMan__destroy(impl_POA_ProjectManager_PrjMan
						* servant,
						CORBA_Environment * ev);
CORBA_char
   *impl_ProjectManager_PrjMan__get_srcLanguage(impl_POA_ProjectManager_PrjMan
						* servant,
						CORBA_Environment * ev);


CORBA_char
   *impl_ProjectManager_PrjMan__get_projectName(impl_POA_ProjectManager_PrjMan
						* servant,
						CORBA_Environment * ev);

CORBA_boolean
impl_ProjectManager_PrjMan_AddFileToProject(impl_POA_ProjectManager_PrjMan *
					    servant, CORBA_char * szCategory,
					    CORBA_char * szFileName,
					    CORBA_Environment * ev);

CORBA_boolean
impl_ProjectManager_PrjMan_RemFileFromProject(impl_POA_ProjectManager_PrjMan *
					      servant,
					      CORBA_char * szCategory,
					      CORBA_char * szFileName,
					      CORBA_Environment * ev);

CORBA_boolean
impl_ProjectManager_PrjMan_SetClassInfo(impl_POA_ProjectManager_PrjMan *
					servant, CORBA_char * szClass,
					CORBA_char * szFileDecl,
					CORBA_char * szFileImpl,
					CORBA_Environment * ev);
void
impl_ProjectManager_PrjMan_SetGladeRef(impl_POA_ProjectManager_PrjMan *
				       servant, Gladen_GladeRef pgf,
				       CORBA_Environment * ev);


int
MainLoop(int *pargc, char *argv[], CORBA_ORB corb );

LocalServer *GetCorbaManager();

void MessageBox( const char* szMsg );

#endif /* USE_GLADEN */
#endif	/*ANJ_CORBA_SERVER_H*/
