#include "Prj.h"


/*** Implementation stub prototypes ***/
void impl_ProjectManager_PrjMan__destroy(impl_POA_ProjectManager_PrjMan
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
				       servant, CORBA_Object pgf,
				       CORBA_Environment * ev);

void
impl_ProjectManager_PrjMan_SaveFileIfModified(impl_POA_ProjectManager_PrjMan *
					      servant,
					      CORBA_char * szFullPath,
					      CORBA_Environment * ev);

void
impl_ProjectManager_PrjMan_ReloadFile(impl_POA_ProjectManager_PrjMan *
				      servant, CORBA_char * szFullPath,
				      CORBA_Environment * ev);

/*** epv structures ***/
static PortableServer_ServantBase__epv impl_ProjectManager_PrjMan_base_epv = {
   NULL,			/* _private data */
   NULL,			/* finalize routine */
   NULL,			/* default_POA routine */
};
static POA_ProjectManager_PrjMan__epv impl_ProjectManager_PrjMan_epv = {
   NULL,			/* _private */
   (gpointer) & impl_ProjectManager_PrjMan__get_srcLanguage,

   (gpointer) & impl_ProjectManager_PrjMan__get_projectName,

   (gpointer) & impl_ProjectManager_PrjMan_AddFileToProject,

   (gpointer) & impl_ProjectManager_PrjMan_RemFileFromProject,

   (gpointer) & impl_ProjectManager_PrjMan_SetClassInfo,

   (gpointer) & impl_ProjectManager_PrjMan_SetGladeRef,

   (gpointer) & impl_ProjectManager_PrjMan_SaveFileIfModified,

   (gpointer) & impl_ProjectManager_PrjMan_ReloadFile,

};

/*** vepv structures ***/
static POA_ProjectManager_PrjMan__vepv impl_ProjectManager_PrjMan_vepv = {
   &impl_ProjectManager_PrjMan_base_epv,
   &impl_ProjectManager_PrjMan_epv,
};

/*** Stub implementations ***/
