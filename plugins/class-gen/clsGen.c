/*
 * Class creator plugin for anjuta
 *
 * Authors:
 *   lb
 */

#include "../../src/anjuta.h"
#include "../../src/project_dbase.h"
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <gnome.h>

#define	INIT(x)	CG_Creator	*self = (CG_Creator*)x

gchar   *GetDescr       (void);
glong    GetVersion     (void);
gboolean Init           (GModule *self, void **pUserData, AnjutaApp* p);
void     CleanUp        (GModule *self, void *pUserData, AnjutaApp* p);
void     Activate       (GModule *self, void *pUserData, AnjutaApp* p);
gchar   *GetMenuTitle   (GModule *self, void *pUserData);
gchar   *GetTooltipText (GModule *self, void *pUserData);


gboolean
ImportFileInProject ( const gchar * p_szModule, const gchar *p_szFileName );

void
on_dlgClass                            (GnomeDialog     *gnomedialog,
                                        gint             arg1,
                                        gpointer         user_data);

void
on_dlgClass                            (GnomeDialog     *gnomedialog,
                                        gint             arg1,
                                        gpointer         user_data);

void
BroseDecl                              (GtkButton       *button,
                                        gpointer         user_data);

void
BrowseImpl                             (GtkButton       *button,
                                        gpointer         user_data);

void
on_m_clsname_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
BrowseDecl                             (GtkButton       *button,
                                        gpointer         user_data);

typedef struct ClsGen_CreateClass {
	gboolean m_bOK;
	gboolean m_bUserEdited;
	gboolean m_cpp;
	gchar	*m_szClassName;
	gchar	*m_szDeclFile;
	gchar	*m_szImplFile;
	gchar	*m_szPrevClassname;
	
  GtkWidget *dlgClass;
  GtkWidget *dialog_vbox1;
  GtkWidget *frame1;
  GtkWidget *table1;
  GtkWidget *m_brH;
  GtkWidget *m_brC;
  GtkWidget *m_clsname;
  GtkWidget *m_declFile;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *m_ImplFile;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;
  GtkWidget *CheckC;
	
} CG_Creator;

// Operations:
void CG_Init(CG_Creator *self);
void CG_Del(CG_Creator *self);
// Operations:
void CG_Init(CG_Creator *self);
void CG_Del(CG_Creator *self);
void CG_Show(CG_Creator *self, ProjectDBase *p );

GtkWidget*
create_dlgClass ( CG_Creator *self );

void CreateCodeClass( ProjectDBase *self );

/* << private >> */
static void MessageBox( const char* szMsg );
static struct tm *GetNowTime( void );
static void GeneraH( CG_Creator *self, FILE* fpOut );
static void GeneraC( CG_Creator *self, FILE* fpOut );

gchar	*GetDescr(void);
glong	GetVersion(void);
gboolean Init( GModule *self, void **pUserData, AnjutaApp* p );
void CleanUp( GModule *self, void *pUserData, AnjutaApp* p );
void Activate( GModule *self, void *pUserData, AnjutaApp* p);
gchar *GetMenuTitle( GModule *self, void *pUserData );
gchar *GetTooltipText( GModule *self, void *pUserData ) ;


/* Get module description */
gchar	*GetDescr()
{
	return g_strdup(_("Class Creator"));
}
	/* GetModule Version hi/low word 1.02 0x10002 */
glong	GetVersion()
{
	return 0x10000L ;
}

gboolean Init( GModule *self, void **pUserData, AnjutaApp* p )
{
	return TRUE ;
}

void CleanUp( GModule *self, void *pUserData, AnjutaApp* p )
{
}

void Activate( GModule *self, void *pUserData, AnjutaApp* p)
{
	if( p && p->project_dbase && p->project_dbase->project_is_open )
		CreateCodeClass( p->project_dbase );
	else
		MessageBox(_("No Project open, unable to add class!"));
}

gchar *GetMenuTitle( GModule *self, void *pUserData )
{
	return g_strdup(_("Class Creator C/C++ 1.0"));
}

gchar *GetTooltipText( GModule *self, void *pUserData ) 
{
   return g_strdup(_("Class Creator"));
}

static gboolean IsLegalClassName( const char *szClassName ); 
static gboolean IsLegalFileName( const char *szFileName );

static void MessageBox( const char* szMsg )
{
	GtkWidget * msgBox = 

   /* Create the dialog using a couple of stock buttons */
   gnome_message_box_new ( szMsg, 
             GNOME_MESSAGE_BOX_QUESTION,
             GNOME_STOCK_BUTTON_OK,
             NULL );
   gnome_dialog_run_and_close (GNOME_DIALOG (msgBox ));
}

static gboolean IsLegalFileName( const char *szFileName )
{
	int nLen ;
	if( NULL == szFileName )
		return FALSE ;
	nLen = strlen( szFileName ) ;
	if( !nLen )
		return FALSE ;
	return TRUE ;
}

static gboolean IsLegalClassName( const char *szClassName )
{
	int nLen, i ;
	if( NULL == szClassName )
		return FALSE ;
	nLen = strlen( szClassName ) ;
	if( !nLen )
		return FALSE ;
	if( !isalpha( szClassName[0] ) )
		return FALSE ;
	for( i = 1 ; i < nLen ; i ++ )
	{
		if( !isalnum( szClassName[i] ) )
			return FALSE ;
	}
	return TRUE ;
}
   
static void CG_GetStrings( CG_Creator	*self )
{
	g_free( self->m_szClassName	 );
	g_free( self->m_szDeclFile );
	g_free( self->m_szImplFile	 );
	self->m_szClassName	= gtk_editable_get_chars(GTK_EDITABLE(self->m_clsname),0, -1 );
	self->m_szDeclFile	= gtk_editable_get_chars(GTK_EDITABLE(self->m_declFile),0, -1 );
	self->m_szImplFile	= gtk_editable_get_chars(GTK_EDITABLE(self->m_ImplFile),0, -1 );
	self->m_cpp	= gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(self->CheckC) );
}

void
on_dlgClass                            (GnomeDialog     *gnomedialog,
                                        gint             arg1,
                                        gpointer         user_data)
{
	INIT(user_data);
	
	if( arg1 == GNOME_OK )
	{
		// Reads the user values in the entry
		CG_GetStrings( self );
		if( ! IsLegalClassName(self->m_szClassName ) )
		{
			MessageBox( _("Class name not valid") );
			return ;
		}
		if( ! IsLegalFileName(self->m_szDeclFile ) )
		{
			MessageBox( _("Declaration filename not valid") );			
			return ;
		}
		if( ! IsLegalFileName(self->m_szImplFile ) )
		{
			MessageBox( _("Implementation filename not valid") );
			return ;
		}
		self->m_bOK = TRUE ;
		gnome_dialog_close(gnomedialog);
	} else if ( arg1 == GNOME_CANCEL )
	{
		gnome_dialog_close(gnomedialog);	}
}


void
BrowseImpl                             (GtkButton       *button,
                                        gpointer         user_data)
{
//	INIT(user_data);
}

gboolean IsFileNameGenerated(gchar* classname, gchar* filename, gchar* extension);

gboolean IsFileNameGenerated(gchar* classname, gchar* filename, gchar* extension)
{
	gboolean result;
	gchar* generatedname = g_strdup_printf("%s.%s", classname, extension);

	if(!strcmp(generatedname, filename)) {
		result = TRUE;
	} else {
		result = FALSE;
	}
	
	g_free(generatedname);
	
	return result;
}

static void CG_DataChanged(CG_Creator	*self )
{
	gchar* generatedname;
	gboolean	bHow = FALSE ;
	
	CG_GetStrings( self );

	if(IsFileNameGenerated(self->m_szPrevClassname, self->m_szDeclFile, "h")) {
		generatedname =  g_strdup_printf("%s.h", self->m_szClassName);
		gtk_signal_handler_block_by_func(GTK_OBJECT(self->m_declFile), GTK_SIGNAL_FUNC(on_m_clsname_changed), self);
		gtk_entry_set_text(GTK_ENTRY(self->m_declFile), generatedname);
		gtk_signal_handler_unblock_by_func(GTK_OBJECT(self->m_declFile), GTK_SIGNAL_FUNC(on_m_clsname_changed), self);
		g_free(generatedname);
	}
	
	if(IsFileNameGenerated(self->m_szPrevClassname, self->m_szImplFile, "cpp")) {
		generatedname =  g_strdup_printf("%s.cpp", self->m_szClassName);
		gtk_signal_handler_block_by_func(GTK_OBJECT(self->m_ImplFile), GTK_SIGNAL_FUNC(on_m_clsname_changed), self);
		gtk_entry_set_text(GTK_ENTRY(self->m_ImplFile), generatedname);
		gtk_signal_handler_unblock_by_func(GTK_OBJECT(self->m_ImplFile), GTK_SIGNAL_FUNC(on_m_clsname_changed), self);
		g_free(generatedname);
	}
	
	if(self->m_szPrevClassname)
		g_free(self->m_szPrevClassname);
	self->m_szPrevClassname = g_strdup(self->m_szClassName);
	
	// Se lutente non ha editato genero automaticamente
	// i nomi di file.
	if( 	IsLegalClassName(self->m_szClassName ) 
		&&	IsLegalFileName(self->m_szDeclFile ) 
		&&	IsLegalFileName(self->m_szImplFile ) )
	{
		bHow = TRUE ;
	}
	gtk_widget_set_sensitive(self->button1, bHow );
	
	if( !self->m_bUserEdited )
	{
	}
}


void
on_m_clsname_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	INIT(user_data);
	CG_DataChanged(self );
}


void
BrowseDecl                             (GtkButton       *button,
                                        gpointer         user_data)
{
//	INIT(user_data);
}

void CreateCodeClass( ProjectDBase *self )
{
	CG_Creator l_server;
	
	CG_Init(&l_server);
	CG_Show(&l_server, self ) ;
	CG_Del(&l_server);
}

static struct tm *GetNowTime( void )
{
	time_t l_time;

	l_time = time (NULL);
	return localtime(&l_time);
}


static void GeneraH( CG_Creator *self, FILE* fpOut )
{
	fprintf( fpOut, "/*\n FILE:%s\n", self->m_szDeclFile );
	{
		gchar* username;
		username = getenv ("USERNAME");
		if (!username)
			username = getenv ("USER");
		fprintf( fpOut, "%s", username );
	}
	{
		struct tm *t = GetNowTime();
		fprintf( fpOut, " \ton:%s\n*/\n\n", asctime(t) );
	}
	
	fprintf( fpOut, 
		"#ifndef	H_include_%s\n"
		"#define	H_include_%s\n"
		"#ifdef HAVE_CONFIG_H\n"
		"#  include <config.h>\n"
		"#endif\n"
		"\n"
		"#include <sys/types.h>\n"
		"#include <sys/stat.h>\n"
		"#include <unistd.h>\n"
		"#include <string.h>\n"
		"\n"
		"#include <gnome.h>\n"
		"\n"
		"typedef struct td_%s {\n"
		"  /* TODO: Put your data here */\n"
		" } %s, *%sPtr ;\n\n"
		"%s *%s_new(void);\n"
		"void %s_delete( %s *self );\n"
		"void %s_end( %s *self );\n"
		"gboolean %s_init( %s *self );\n"
		"#endif	/*H_include_%s*/\n"
		"\n\n",
		self->m_szClassName, self->m_szClassName, 
		self->m_szClassName, self->m_szClassName, 
		self->m_szClassName, self->m_szClassName, 
		self->m_szClassName, self->m_szClassName, 
		self->m_szClassName, self->m_szClassName, self->m_szClassName, 
		self->m_szClassName, self->m_szClassName, self->m_szClassName );
	
}

static void GeneraHH( CG_Creator *self, FILE* fpOut )
{
	fprintf( fpOut, "/*\n FILE:%s\n", self->m_szDeclFile );
	{
		gchar* username;
		username = getenv ("USERNAME");
		if (!username)
			username = getenv ("USER");
		fprintf( fpOut, "%s", username );
	}
	{
		struct tm *t = GetNowTime();
		fprintf( fpOut, " \ton:%s\n*/\n\n", asctime(t) );
	}
	
	fprintf( fpOut, 
		"#ifndef	H_include_%s\n"
		"#define	H_include_%s\n", self->m_szClassName, self->m_szClassName );
	fprintf( fpOut, 	
		"#ifdef HAVE_CONFIG_H\n"
		"#  include <config.h>\n"
		"#endif\n"
		"\n"
		"#include <sys/types.h>\n"
		"#include <sys/stat.h>\n"
		"#include <unistd.h>\n"
		"#include <string.h>\n"
		"\n"
		"#include <gnome.h>\n"
		"\n"
		"class %s {\n", self->m_szClassName );
	fprintf( fpOut, 
		"  /* TODO: Put your data here */\n"
		"  /* Constructor */\n"
		" public:\n"
		" %s();\n"
		" ~%s();\n"
		" }  ;\n\n"
		"typedef class %s *%sPtr;\n"
		"#endif	/*H_include_%s*/\n"
		"\n\n",
		self->m_szClassName, self->m_szClassName, 
		self->m_szClassName, self->m_szClassName, 
		self->m_szClassName );

}


static void GeneraC( CG_Creator *self, FILE* fpOut )
{
	fprintf( fpOut, "/*\n FILE:%s\n", self->m_szDeclFile );
	{
		gchar* username;
		username = getenv ("USERNAME");
		if (!username)
			username = getenv ("USER");
		fprintf( fpOut, "%s", username );
	}
	{
		struct tm *t = GetNowTime();
		fprintf( fpOut, " \ton:%s\n*/\n\n", asctime(t) );
	}
	
	fprintf( fpOut, 
		"#include	\"%s\"\n"
		"\n"
		"\n"
		"gboolean %s_init( %s *self )\n"
		"{\n"
		" /* TODO: put init code here */\n"
		"\treturn TRUE ;\n"
		"}\n"
		"\n", self->m_szDeclFile, self->m_szClassName, self->m_szClassName );

	fprintf( fpOut, 
		"void %s_end( %s *self )\n"
		"{\n"
		" /*TO-DO: put here end code*/\n"
		"}\n\n",
		self->m_szClassName, self->m_szClassName );
		
	fprintf( fpOut,
		"void %s_delete( %s *self )\n"
		"{\n"
		" /* TODO: put end code here */\n"
		"\tg_return_if_fail(NULL!=self);\n\n"
		"\t%s_end(self);\n"
		"\tg_free(self);\n"
		"}\n\n",
		self->m_szClassName, self->m_szClassName, self->m_szClassName );
		
	fprintf( fpOut,
		"%s *%s_new(void)\n"
		"{\n\t%s *self;\n"
		" /* TODO: put init code here */\n"
		"\tself=g_new( %s, 1 );\n"
		"\tif(NULL!=self)\n"
		"\t{\n"
		"\t\tif(!%s_init(self))\n"
		"\t\t{\n"
		"\t\t\tg_free(self);\n"
		"\t\t\tself = NULL ;\n"
		"\t\t}\n"
		"\t}\n"
		"\treturn self;\n"
		"}\n\n"

		"\n\n",		self->m_szClassName, 
		self->m_szClassName, self->m_szClassName, 
		self->m_szClassName, self->m_szClassName);
}

static void GeneraCC( CG_Creator *self, FILE* fpOut )
{
	fprintf( fpOut, "/*\n FILE:%s\n", self->m_szDeclFile );
	{
		gchar* username;
		username = getenv ("USERNAME");
		if (!username)
			username = getenv ("USER");
		fprintf( fpOut, "%s", username );
	}
	{
		struct tm *t = GetNowTime();
		fprintf( fpOut, " \ton:%s\n*/\n\n", asctime(t) );
	}
	
	fprintf( fpOut, 
		"#include	\"%s\"\n"
		"\n"
		"\n"
		"%s::%s()\n"
		"{\n"
		" /* TODO: put init code here */\n"
		"}\n"
		"\n", self->m_szDeclFile, self->m_szClassName, self->m_szClassName );

	fprintf( fpOut, 
		"%s::~%s()\n"
		"{\n"
		" /*TO-DO: put here end code*/\n"
		"}\n\n",
		self->m_szClassName, self->m_szClassName );
		
}

	
void CG_Show(CG_Creator *self, ProjectDBase *p )
{
	if( NULL != create_dlgClass (self) )
	{
		gtk_widget_show (self->dlgClass);
		//gtk_window_activate_focus (self->m_clsname);
		gtk_widget_draw_focus(self->m_clsname);
		gtk_widget_set_sensitive(self->m_brH, FALSE );
		gtk_widget_set_sensitive(self->m_brC, FALSE );
		gtk_widget_set_sensitive(self->button1, FALSE );
		gnome_dialog_run_and_close (GNOME_DIALOG (self->dlgClass));
		if( self->m_bOK )
		{
			gboolean	bOK = FALSE ;
			gchar	*szDir = project_dbase_get_module_dir (p, MODULE_SOURCE );
			gchar	*szfNameC = g_strdup_printf( "%s/%s", szDir, self->m_szImplFile );
			gchar	*szfNameH = g_strdup_printf( "%s/%s", szDir, self->m_szDeclFile );
			FILE* fpH = fopen( szfNameH, "at" );
			FILE* fpC;
			if( NULL != fpH )
			{
				if(self->m_cpp)
					GeneraHH( self, fpH );
				else
					GeneraH( self, fpH );
				fflush( fpH );
				bOK = !ferror( fpH );
				fclose( fpH );
				fpH = NULL ;
			}
			fpC = fopen( szfNameC, "at" );
			if( NULL != fpC )
			{
				if(self->m_cpp)
					GeneraCC( self, fpC );
				else
					GeneraC( self, fpC );
				fflush( fpC );
				if( bOK )
					bOK = !ferror( fpC );
				fclose( fpC );
				fpC = NULL ;
			}
			//---------------
			if( bOK )
			{
				if( ! ImportFileInProject ( "SOURCE", szfNameC ) )
				{
					MessageBox( _("Error importing source file"));
				}
				if( ! ImportFileInProject ( "SOURCE", szfNameH ) )
				{
					MessageBox( _("Error importing include file"));
				}
			} else
				MessageBox( _("Error importing files"));
			g_free( szfNameC );
			g_free( szfNameH );
		}
	}
}

// Operations:
void CG_Init(CG_Creator *self)
{
	self->m_bOK = FALSE ;
	self->m_bUserEdited = FALSE ;
	self->m_cpp	= FALSE ;
	self->m_szClassName = NULL ;
	self->m_szDeclFile= NULL ;
	self->m_szImplFile= NULL ;
	self->m_szPrevClassname = g_strdup("");
}

void CG_Del(CG_Creator *self)
{
	g_free(self->m_szClassName);
	g_free(self->m_szDeclFile) ;
	g_free(self->m_szImplFile);
	g_free(self->m_szPrevClassname);
}



GtkWidget*
create_dlgClass ( CG_Creator *self )
{
  self->dlgClass = gnome_dialog_new (_("Class Creator"), NULL);
  gtk_object_set_data (GTK_OBJECT (self->dlgClass), "dlgClass", self->dlgClass);

  self->dialog_vbox1 = GNOME_DIALOG (self->dlgClass)->vbox;
  gtk_object_set_data (GTK_OBJECT (self->dlgClass), "dialog_vbox1", self->dialog_vbox1);
  gtk_widget_show (self->dialog_vbox1);

  self->frame1 = gtk_frame_new (_("New Class: "));
  gtk_widget_ref (self->frame1);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "frame1", self->frame1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->frame1);
  gtk_box_pack_start (GTK_BOX (self->dialog_vbox1), self->frame1, TRUE, TRUE, 0);

  self->table1 = gtk_table_new ( 4, 3, FALSE);
  gtk_widget_ref (self->table1);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "table1", self->table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->table1);
  gtk_container_add (GTK_CONTAINER (self->frame1), self->table1);

  self->m_brH = gtk_button_new_with_label (_("Browse..."));
  gtk_widget_ref (self->m_brH);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "m_brH", self->m_brH,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->m_brH);
  gtk_table_attach (GTK_TABLE (self->table1), self->m_brH, 2, 3, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  self->m_brC = gtk_button_new_with_label (_("Browse..."));
  gtk_widget_ref (self->m_brC);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "m_brC", self->m_brC,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->m_brC);
  gtk_table_attach (GTK_TABLE (self->table1), self->m_brC, 2, 3, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  self->m_clsname = gtk_entry_new_with_max_length (32);
  gtk_widget_ref (self->m_clsname);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "m_clsname", self->m_clsname,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->m_clsname);
  gtk_table_attach (GTK_TABLE (self->table1), self->m_clsname, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  self->m_declFile = gtk_entry_new_with_max_length (255);
  gtk_entry_set_text(GTK_ENTRY(self->m_declFile), ".h");
  gtk_widget_ref (self->m_declFile);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "m_declFile", self->m_declFile,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->m_declFile);
  gtk_table_attach (GTK_TABLE (self->table1), self->m_declFile, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  self->label1 = gtk_label_new (_("Class Name:"));
  gtk_widget_ref (self->label1);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label1", self->label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label1);
  gtk_table_attach (GTK_TABLE (self->table1), self->label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  self->label2 = gtk_label_new (_("Declaration File:"));
  gtk_widget_ref (self->label2);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label2", self->label2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label2);
  gtk_table_attach (GTK_TABLE (self->table1), self->label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  self->label3 = gtk_label_new (_("Implementation File:"));
  gtk_widget_ref (self->label3);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label3", self->label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label3);
  gtk_table_attach (GTK_TABLE (self->table1), self->label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  self->m_ImplFile = gtk_entry_new_with_max_length (255);
  gtk_entry_set_text(GTK_ENTRY(self->m_ImplFile), ".cpp");
  gtk_widget_ref (self->m_ImplFile);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "m_ImplFile", self->m_ImplFile,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->m_ImplFile);
  gtk_table_attach (GTK_TABLE (self->table1), self->m_ImplFile, 1, 2, 2, 3,
                    (GtkAttachOptions) 0,/*(GTK_EXPAND | GTK_FILL),*/
                    (GtkAttachOptions) (0), 0, 0);

	self->CheckC = gtk_check_button_new_with_label( "C++" );
	gtk_widget_ref (self->CheckC);	
	gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "CheckC", self->CheckC,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (self->CheckC);
	gtk_table_attach (GTK_TABLE (self->table1), self->CheckC, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  self->dialog_action_area1 = GNOME_DIALOG (self->dlgClass)->action_area;
  gtk_object_set_data (GTK_OBJECT (self->dlgClass), "dialog_action_area1", self->dialog_action_area1);
  gtk_widget_show (self->dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (self->dialog_action_area1), GTK_BUTTONBOX_EDGE);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (self->dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (self->dlgClass), GNOME_STOCK_BUTTON_OK);
  self->button1 = GTK_WIDGET (g_list_last (GNOME_DIALOG (self->dlgClass)->buttons)->data);
  gtk_widget_ref (self->button1);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button1", self->button1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button1);
  GTK_WIDGET_SET_FLAGS (self->button1, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (self->dlgClass), GNOME_STOCK_BUTTON_HELP);
  self->button2 = GTK_WIDGET (g_list_last (GNOME_DIALOG (self->dlgClass)->buttons)->data);
  gtk_widget_ref (self->button2);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button2", self->button2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button2);
  GTK_WIDGET_SET_FLAGS (self->button2, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (self->dlgClass), GNOME_STOCK_BUTTON_CANCEL);
  self->button3 = GTK_WIDGET (g_list_last (GNOME_DIALOG (self->dlgClass)->buttons)->data);
  gtk_widget_ref (self->button3);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button3", self->button3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button3);
  GTK_WIDGET_SET_FLAGS (self->button3, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (self->dlgClass), "clicked",
                      GTK_SIGNAL_FUNC (on_dlgClass),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->m_brH), "clicked",
                      GTK_SIGNAL_FUNC (BrowseDecl),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->m_brC), "clicked",
                      GTK_SIGNAL_FUNC (BrowseImpl),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->m_clsname), "changed",
                      GTK_SIGNAL_FUNC (on_m_clsname_changed),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->m_declFile), "changed",
                      GTK_SIGNAL_FUNC (on_m_clsname_changed),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->m_ImplFile), "changed",
                      GTK_SIGNAL_FUNC (on_m_clsname_changed),
                      self);
	gtk_widget_grab_focus(self->m_clsname);
  return self->dlgClass;
}
