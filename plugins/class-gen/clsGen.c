/*
 * clsGen.h Copyright (C) 2002  Dave Huseby
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */


/*
 * V 0.0.2 -- Generic Class Generator Plugins
 *	Author: Dave Huseby (huseby@linuxprogrammer.org)
 *	Notes: I was asked to generalize my C++ class generator into Anjuta's
 *	general class generator plugin.  This is the result.
 *
 * V 0.0.1 -- Generic C++ Class Generator Plugin
 * 	Author: Dave Huseby (huseby@linuxprogrammer.org)
 *
 * Based on the Gtk clsGen plugin by:
 *	Tom Dyas (tdyas@romulus.rutgers.edu)
 *	JP Rosevear (jpr@arcavia.com)
 *
 */

#include "../../src/anjuta.h"
#include "../../src/project_dbase.h"
#include "class_logo.xpm"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <gnome.h>

#define	INIT(x)	CG_Creator* self = (CG_Creator*)x
#define SAFE_FREE(x) { if(x != NULL) g_free(x); }


/*
 * Dialog Class Struct
 */
typedef struct ClsGen_CreateClass {
	// Member variables
	gboolean		m_bOK;
	gboolean		m_bUserEdited;
	gboolean		m_bUserSelectedHeader;
	gboolean		m_bUserSelectedSource;
	gboolean		m_bVirtualDestructor;
	gboolean		m_bInline;
	gchar*			m_szClassName;
	gchar*			m_szDeclFile;
	gchar*			m_szImplFile;
	gchar*			m_szBaseClassName;
	gchar*			m_szAccess;
	gchar*			m_szClassType;
	ProjectDBase*	m_pDB;
	
	// GTK+ interface
	GtkWidget*		dlgClass;
	GtkWidget*		fixed;
	
	// logo
	GtkWidget*		pixmap_logo;
	GdkColormap*	colormap;
	GdkPixmap*		gdkpixmap;
	GdkBitmap*		mask;
	
	// buttons
	GtkWidget*		button_help;
	GtkWidget*		button_cancel;
	GtkWidget*		button_finish;
	GtkWidget*		button_browse_header_file;
	GtkWidget*		button_browse_source_file;
	
	// entries
	GtkWidget*		entry_class_name;
	GtkWidget*		entry_header_file;
	GtkWidget*		entry_source_file;
	GtkWidget*		entry_base_class;
	
	// labels
	GtkWidget*		label_welcome;
	GtkWidget*		label_description;
	GtkWidget*		label_class_type;
	GtkWidget*		label_class_name;
	GtkWidget*		label_header_file;
	GtkWidget*		label_source_file;
	GtkWidget*		label_base_class;
	GtkWidget*		label_access;
	GtkWidget*		label_author;
	GtkWidget*		label_version;
	GtkWidget*		label_todo;
	
	// separators
	GtkWidget*		hseparator1;
	GtkWidget*		hseparator3;
	GtkWidget*		hseparator2;
	GtkWidget*		hseparator4;
	
	// checkbuttons
	GtkWidget*		checkbutton_virtual_destructor;
	GtkWidget*		checkbutton_inline;
	
	// combo boxes
	GtkWidget*		combo_access;
	GList*			combo_access_items;
	GtkWidget*		combo_access_entry;
	GtkWidget*		combo_class_type;
	GList*			combo_class_type_items;
	GtkWidget*		combo_class_type_entry;
	
	// file selections
	GtkWidget*		header_file_selection;
	GtkWidget*		source_file_selection;
	
	// tooltips
	GtkTooltips*	tooltips;
	
} CG_Creator;


/*
 *------------------------------------------------------------------------------
 * Foward Declarations
 *------------------------------------------------------------------------------
 */

/*
 * Plugin Interface
 */
gchar* GetDescr(void);
glong GetVersion(void);
gboolean Init(GModule* self, void** pUserData, AnjutaApp* p);
void CleanUp(GModule* self, void* pUserData, AnjutaApp* p);
void Activate(GModule* self, void* pUserData, AnjutaApp* p);
gchar* GetMenuTitle(GModule* self, void* pUserData);
gchar* GetTooltipText(GModule* self, void* pUserData);


/*
 * External Helper Function
 */
gboolean ImportFileInProject(const gchar* p_szModule, const gchar* p_szFileName);


/*
 * Event Callbacks
 */
gint on_delete_event(GtkWidget* widget, GdkEvent* event, gpointer data);
void on_header_browse_clicked(GtkButton* button, gpointer user_data);
void on_source_browse_clicked(GtkButton* button, gpointer user_data);
void on_class_name_changed(GtkEditable* editable, gpointer user_data);
void on_class_type_changed(GtkEditable* editable, gpointer user_data);
void on_finish_clicked(GtkButton* button, gpointer user_data);
void on_cancel_clicked(GtkButton* button, gpointer user_data);
void on_help_clicked(GtkButton* button, gpointer user_data);
void on_inline_toggled(GtkToggleButton* button, gpointer user_data);
void on_header_file_selection_cancel(GtkFileSelection* selection, gpointer user_data);
void on_source_file_selection_cancel(GtkFileSelection* selection, gpointer user_data);
void on_header_file_selection(GtkFileSelection* selection, gpointer user_data); 
void on_source_file_selection(GtkFileSelection* selection, gpointer user_data); 


/*
 * << private >>
 */
static void MessageBox(const char* szMsg);
static struct tm* GetNowTime(void);
static gboolean IsLegalClassName(const char* szClassName); 
static gboolean IsLegalFileName(const char* szFileName);
static void Generate(CG_Creator *self);
static void GenerateHeader(CG_Creator* self, FILE* fpOut);
static void GenerateSource(CG_Creator* self, FILE* fpOut);
static void CG_GetStrings(CG_Creator* self);


/*
 * Operations
 */
void CG_Init(CG_Creator* self, ProjectDBase* pDB);
void CG_Del(CG_Creator* self);
void CG_Show(CG_Creator* self);
void CreateCodeClass(CG_Creator* self, ProjectDBase* pDB);
GtkWidget* CreateDialogClass(CG_Creator* self);


/*
 *------------------------------------------------------------------------------
 * Plugin Interface
 *------------------------------------------------------------------------------
 */

gchar*
GetDescr()
{
	return g_strdup("Class Builder");
}


glong
GetVersion()
{
	return 0x10000L; 
}


gboolean
Init(GModule *self, void **pUserData, AnjutaApp* p)
{
	// Malloc a CG_Creator struct
	*pUserData = malloc(sizeof(CG_Creator));
	return TRUE; 
}


void
CleanUp(GModule *self, void *pUserData, AnjutaApp* p) 
{
	// Release the CG_Creator struct
	SAFE_FREE(pUserData);
}


void
Activate(GModule *self, void *pUserData, AnjutaApp* p) 
{
	if(p && p->project_dbase && p->project_dbase->project_is_open)
		CreateCodeClass((CG_Creator*)pUserData, p->project_dbase);
}


gchar*
GetMenuTitle( GModule *self, void *pUserData )
{
	return g_strdup("Class Builder");
}


gchar*
GetTooltipText( GModule *self, void *pUserData ) 
{
   return g_strdup("Class Builder");
}


/*
 *------------------------------------------------------------------------------
 * Event Callbacks
 *------------------------------------------------------------------------------
 */


gint
on_delete_event(GtkWidget* widget, GdkEvent* event, gpointer user_data)
{	
	INIT(user_data);
	
	// free up the strings and destroy the window
	CG_Del(self);
	//gtk_widget_destroy(GTK_WIDGET(self->dlgClass));
	
	return FALSE;
}

void
on_header_browse_clicked(GtkButton* button, gpointer user_data)
{
	INIT(user_data);
	
	self->header_file_selection = gtk_file_selection_new("Select header file.");
	gtk_window_set_modal(GTK_WINDOW(self->header_file_selection), FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(self->header_file_selection)->ok_button),
						"clicked", GTK_SIGNAL_FUNC(on_header_file_selection), self);
	
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(self->header_file_selection)->cancel_button),
						"clicked", GTK_SIGNAL_FUNC(on_header_file_selection_cancel), self);

	gtk_file_selection_complete(GTK_FILE_SELECTION(self->header_file_selection), "*.h");
	gtk_widget_show(self->header_file_selection);
}


void
on_source_browse_clicked(GtkButton* button, gpointer user_data)
{
	INIT(user_data);

	self->source_file_selection = gtk_file_selection_new("Select source file.");
	gtk_window_set_modal(GTK_WINDOW(self->source_file_selection), FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(self->source_file_selection)->ok_button),
						"clicked", GTK_SIGNAL_FUNC(on_source_file_selection), self);
	
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(self->source_file_selection)->cancel_button),
						"clicked", GTK_SIGNAL_FUNC(on_source_file_selection_cancel), self);
	
	SAFE_FREE(self->m_szClassType);
	self->m_szClassType = gtk_editable_get_chars(GTK_EDITABLE(self->combo_class_type_entry),0, -1);
	
	if(strcmp(self->m_szClassType, "Generic C++ Class") == 0)
	{
		gtk_file_selection_complete(GTK_FILE_SELECTION(self->source_file_selection), "*.cc");
	}
	else if(strcmp(self->m_szClassType, "GTK+ Class") == 0)
	{
		gtk_file_selection_complete(GTK_FILE_SELECTION(self->source_file_selection), "*.c");
	}

	gtk_widget_show(self->source_file_selection);
}

void
on_class_name_changed(GtkEditable* editable, gpointer user_data)
{	
	gchar buf[1024];
	INIT(user_data);
	
	SAFE_FREE(self->m_szClassName);
	SAFE_FREE(self->m_szDeclFile);
	SAFE_FREE(self->m_szImplFile);
	
	// get the new class name
	self->m_szClassName = gtk_editable_get_chars(GTK_EDITABLE(self->entry_class_name),0, -1);
	
	if(strlen(self->m_szClassName) > 0)
	{
		if(!self->m_bUserSelectedHeader)
		{
			// set the header file name
			memset(buf, 0, 1024 * sizeof(gchar));
			sprintf(buf, "%s.h", self->m_szClassName);
			gtk_entry_set_text(GTK_ENTRY(self->entry_header_file), buf);
		}
		
		if(!self->m_bUserSelectedSource)
		{
			SAFE_FREE(self->m_szClassType);
			self->m_szClassType = gtk_editable_get_chars(GTK_EDITABLE(self->combo_class_type_entry),0, -1);
			
			if(strcmp(self->m_szClassType, "Generic C++ Class") == 0)
			{
				// set the cc file name
				memset(buf, 0, 1024 * sizeof(gchar));
				sprintf(buf, "%s.cc", self->m_szClassName);
				gtk_entry_set_text(GTK_ENTRY(self->entry_source_file), buf);
			}
			else if(strcmp(self->m_szClassType, "GTK+ Class") == 0)
			{
				// set the cc file name
				memset(buf, 0, 1024 * sizeof(gchar));
				sprintf(buf, "%s.c", self->m_szClassName);
				gtk_entry_set_text(GTK_ENTRY(self->entry_source_file), buf);
			}
		}
		
		// set the browse buttons to sensitive
		gtk_widget_set_sensitive(self->button_browse_header_file, TRUE);
		if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->checkbutton_inline)))
			gtk_widget_set_sensitive(self->button_browse_source_file, TRUE);
		
		self->m_bUserEdited = TRUE;
		gtk_widget_set_sensitive(GTK_WIDGET(self->button_finish), TRUE);
	}
	else
	{
		// set the header and cc file names to null and set the buttons to insensitive
		if(!self->m_bUserSelectedHeader)
		{
			gtk_entry_set_text(GTK_ENTRY(self->entry_header_file), "");
			gtk_widget_set_sensitive(self->button_browse_header_file, FALSE);
		}
		
		if(!self->m_bUserSelectedSource)
		{
			gtk_entry_set_text(GTK_ENTRY(self->entry_source_file), "");
			gtk_widget_set_sensitive(self->button_browse_source_file, FALSE);
		}
		
		self->m_bUserEdited = FALSE;
		gtk_widget_set_sensitive(GTK_WIDGET(self->button_finish), FALSE);
	}
		
	// get the chars from the entries
	self->m_szDeclFile = gtk_editable_get_chars(GTK_EDITABLE(self->entry_header_file), 0, -1);
	self->m_szImplFile = gtk_editable_get_chars(GTK_EDITABLE(self->entry_source_file), 0, -1);
}


void
on_class_type_changed(GtkEditable* editable, gpointer user_data)
{	
	gchar buf = '\0';
	INIT(user_data);
	
	SAFE_FREE(self->m_szClassType);
	
	// get the new class name
	self->m_szClassType = gtk_editable_get_chars(GTK_EDITABLE(self->combo_class_type_entry),0, -1);
	
	if(strlen(self->m_szClassType) > 0)
	{
		if(strcmp(self->m_szClassType, "Generic C++ Class") == 0)
		{
			// tailor the interface to c++
			gtk_widget_set_sensitive(self->combo_access, TRUE);
			gtk_widget_set_sensitive(self->checkbutton_virtual_destructor, TRUE);
			gtk_widget_set_sensitive(self->entry_base_class, TRUE);
			gtk_widget_set_sensitive(self->label_base_class, TRUE);
			gtk_widget_set_sensitive(self->label_access, TRUE);
		}
		else if(strcmp(self->m_szClassType, "GTK+ Class") == 0)
		{
			// tailor the interface to GTK+ c
			gtk_widget_set_sensitive(self->combo_access, FALSE);
			gtk_widget_set_sensitive(self->checkbutton_virtual_destructor, FALSE);
			gtk_widget_set_sensitive(self->entry_base_class, FALSE);
			gtk_widget_set_sensitive(self->label_base_class, FALSE);
			gtk_widget_set_sensitive(self->label_access, FALSE);
			gtk_entry_set_text(GTK_ENTRY(self->entry_base_class), &buf);
			
			SAFE_FREE(self->m_szBaseClassName);
			self->m_szBaseClassName = gtk_editable_get_chars(GTK_EDITABLE(self->entry_base_class),0, -1);
		}
	}
}


void
on_finish_clicked(GtkButton* button, gpointer user_data)
{

	INIT(user_data);
	
	CG_GetStrings(self);
	if(!IsLegalClassName(self->m_szClassName))
	{
		MessageBox(_("Class name not valid"));
		return;
	}
	if(strlen(self->m_szBaseClassName) > 0)
	{
		if(!IsLegalClassName(self->m_szBaseClassName))
		{
			MessageBox(_("Base class name not valid"));
			return;
		}
	}
	if(!IsLegalFileName(self->m_szDeclFile))
	{
		MessageBox(_("Declaration file name not valid"));
		return;
	}
	if(!IsLegalFileName(self->m_szImplFile))
	{
		MessageBox(_("Implementation file name not valid"));
		return;
	}
	Generate(self);
	CG_Del(self);
	gtk_widget_destroy(GTK_WIDGET(self->dlgClass));
}


void
on_cancel_clicked(GtkButton* button, gpointer user_data)
{
	INIT(user_data);
	
	// free up the strings and destroy the window
	CG_Del(self);
	gtk_widget_destroy(GTK_WIDGET(self->dlgClass));
}


void
on_help_clicked(GtkButton* button, gpointer user_data)
{
}


void
on_inline_toggled(GtkToggleButton* button, gpointer user_data)
{
	INIT(user_data);
	
	self->m_bInline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->checkbutton_inline));
	
	if(!self->m_bInline)
	{
		// set the source file entry and browse buttons sensitive
		gtk_widget_set_sensitive(self->entry_source_file, TRUE);
		if(self->m_bUserEdited)
			gtk_widget_set_sensitive(self->button_browse_source_file, TRUE);
	}
	else
	{
		// set the source file entry and browse buttons insensitive
		gtk_widget_set_sensitive(self->entry_source_file, FALSE);
		gtk_widget_set_sensitive(self->button_browse_source_file, FALSE);
	}
}


void
on_header_file_selection_cancel(GtkFileSelection* selection, gpointer user_data)
{
	INIT(user_data);
	
	printf("header cancel\n");
	gtk_widget_destroy(self->header_file_selection);
	self->header_file_selection = NULL;
}


void
on_source_file_selection_cancel(GtkFileSelection* selection, gpointer user_data)
{
	INIT(user_data);
	
	printf("source cancel\n");
	gtk_widget_destroy(self->source_file_selection);
	self->source_file_selection = NULL;
}


void 
on_header_file_selection(GtkFileSelection* selection, gpointer user_data)
{
	INIT(user_data);
	
	SAFE_FREE(self->m_szDeclFile);
	self->m_szDeclFile = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(self->header_file_selection)));
	gtk_entry_set_text(GTK_ENTRY(self->entry_header_file), self->m_szDeclFile);
	
	if(strlen(self->m_szDeclFile) > 0)
		self->m_bUserSelectedHeader = TRUE;
	else
		self->m_bUserSelectedHeader = FALSE;
	
	gtk_widget_destroy(self->header_file_selection);
	self->header_file_selection = NULL;
}


void
on_source_file_selection(GtkFileSelection* selection, gpointer user_data)
{
	INIT(user_data);
	
	SAFE_FREE(self->m_szImplFile);
	self->m_szImplFile = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(self->source_file_selection)));
	gtk_entry_set_text(GTK_ENTRY(self->entry_source_file), self->m_szImplFile);

	if(strlen(self->m_szImplFile) > 0)
		self->m_bUserSelectedSource = TRUE;
	else
		self->m_bUserSelectedSource = FALSE;
	
	gtk_widget_destroy(self->source_file_selection);
	self->source_file_selection = NULL;
}


/*
 *------------------------------------------------------------------------------
 * Private
 *------------------------------------------------------------------------------
 */

static void
MessageBox(const char* szMsg)
{
	/* Create the dialog using a couple of stock buttons */
	GtkWidget * msgBox = gnome_message_box_new
	(
		szMsg, 
		GNOME_MESSAGE_BOX_QUESTION,
		GNOME_STOCK_BUTTON_OK,
		NULL
	);
	
	gnome_dialog_run_and_close(GNOME_DIALOG (msgBox));
}


static struct tm*
GetNowTime(void)
{
	time_t l_time;

	l_time = time(NULL);
	return localtime(&l_time);
}


static gboolean
IsLegalClassName(const char *szClassName)
{
	int nLen, i;
	
	if(NULL == szClassName)
		return FALSE;
	
	nLen = strlen(szClassName);
	if(!nLen)
		return FALSE;
	
	if(!isalpha(szClassName[0]))
		return FALSE;
	
	for(i = 1; i < nLen; i ++)
	{
		if(!isalnum(szClassName[i]))
			return FALSE;
	}
	
	return TRUE;
}


static gboolean
IsLegalFileName(const char *szFileName)
{
	int nLen;
	
	if(NULL == szFileName)
		return FALSE;
	
	nLen = strlen(szFileName);
	if(!nLen)
		return FALSE;
	
	return TRUE;
}


static void
Generate(CG_Creator *self)
{
	gboolean bOK = FALSE;
	gchar* szDir = project_dbase_get_module_dir(self->m_pDB, MODULE_SOURCE);
	gchar* szfNameHeader;
	gchar* szfNameSource;
	FILE* fpHeader;
	FILE* fpSource;
	
	if(!self->m_bUserSelectedHeader)
		szfNameHeader = g_strdup_printf("%s/%s", szDir, self->m_szDeclFile);
	else
		szfNameHeader = g_strdup(self->m_szImplFile);
	
	if(!self->m_bUserSelectedSource)
		szfNameSource = g_strdup_printf("%s/%s", szDir, self->m_szImplFile);
	else
		szfNameSource = g_strdup(self->m_szImplFile);
	
	if(!self->m_bInline)
	{
		fpHeader = fopen(szfNameHeader, "at");
		if(fpHeader != NULL)
		{
			GenerateHeader(self, fpHeader);
			fflush(fpHeader);
			bOK = !ferror(fpHeader);
			fclose(fpHeader);
			fpHeader = NULL;
		}
		
		fpSource = fopen(szfNameSource, "at");
		if(fpSource != NULL)
		{
			GenerateSource(self, fpSource);
			fflush(fpSource);
			bOK = !ferror(fpSource);
			fclose(fpSource);
			fpSource = NULL;
		}
	}
	else
	{
		fpHeader = fopen(szfNameHeader, "at");
		if(fpHeader != NULL)
		{
			GenerateHeader(self, fpHeader);
			GenerateSource(self, fpHeader);
			fflush(fpHeader);
			bOK = !ferror(fpHeader);
			fclose(fpHeader);
			fpHeader = NULL;
		}
	}
	
	if(bOK)
	{
		if(!self->m_bInline)
			if(!ImportFileInProject("SOURCE", szfNameSource))
				MessageBox(_("Error in importing source file"));
		
		if(!ImportFileInProject("SOURCE", szfNameHeader))
			MessageBox(_("Error in importing include file"));
	}
	else
		MessageBox(_("Error in importing files"));
	
	g_free(szfNameHeader);
	g_free(szfNameSource);
}

static void
GenerateHeader(CG_Creator *self, FILE* fpOut)
{
	int i;
	gchar* all_uppers = (gchar*)malloc((strlen(self->m_szClassName) + 1) * sizeof(gchar));
	strcpy(all_uppers, self->m_szClassName);
	for(i = 0; i < strlen(all_uppers); i++)
	{
		all_uppers[i] = toupper(all_uppers[i]);
	}

	if(strcmp(self->m_szClassType, "Generic C++ Class") == 0)
	{
		// output a C++ header
		fprintf(fpOut, "//\n// File: %s\n", self->m_szDeclFile);
		
		{
			gchar* username;
			gchar* email;
			
			username = getenv ("USERNAME");
			if (!username)
				username = getenv ("USER");
			
			email = getenv("EMAIL");
			fprintf( fpOut, "// Created by: %s <%s>\n", username, email );
		}
		
		{
			struct tm *t = GetNowTime();
			fprintf( fpOut, "// Created on: %s//\n\n", asctime(t) );
		}
		
		
		fprintf
		(
			fpOut,
			"#ifndef _%s_H_\n"
			"#define _%s_H_\n"
			"\n"
			"\n",
			all_uppers, 
			all_uppers
		);
		
		if(self->m_bInline)
		{
			// output some nice deliniation comments
			fprintf
			(
				fpOut,
				"//------------------------------------------------------------------------------\n"
				"// %s Declaration\n"
				"//------------------------------------------------------------------------------\n"
				"\n"
				"\n",
				self->m_szClassName
			);
		}
		
		if(strlen(self->m_szBaseClassName) > 0)
		{
			// output class with inheritence
			fprintf
			(
				fpOut, 
				"class %s : %s %s\n"
				"{\n"
				"\tpublic:\n"
				"\t\t%s();\n",
				self->m_szClassName, 
				self->m_szAccess,
				self->m_szBaseClassName,
				self->m_szClassName
			);
		}
		else
		{
			// output class without inheritence
			fprintf
			(
				fpOut, 
				"class %s\n"
				"{\n"
				"\tpublic:\n"
				"\t\t%s();\n",
				self->m_szClassName, 
				self->m_szClassName
			);
		}
		
		if(self->m_bVirtualDestructor)
		{
			// output virtual destructor
			fprintf
			(
				fpOut,
				"\t\tvirtual ~%s();\n",
				self->m_szClassName
			);
		}
		else
		{
			// output non-virtual destructor
			fprintf
			(
				fpOut,
				"\t\t ~%s();\n",
				self->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			"\t\n"
			"\t\t// %s interface\n"
			"\t\n"
			"\t\t// TODO: add member function declarations...\n"
			"\t\n"
			"\tprotected:\n"
			"\t\t// %s variables\n"
			"\t\n"
			"\t\t// TODO: add member variables...\n"
			"\t\n"
			"};\n"
			"\n"
			"\n",
			self->m_szClassName,
			self->m_szClassName
		);
		
		if(!self->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	//_%s_H_\n"
				"\n",
				all_uppers
			);
		}
	}
	else if(strcmp(self->m_szClassType, "GTK+ Class") == 0)
	{
		// output a GTK+ class header
		fprintf(fpOut, "/*\n * File: %s\n", self->m_szDeclFile);
		
		{
			gchar* username;
			gchar* email;
			
			username = getenv ("USERNAME");
			if (!username)
				username = getenv ("USER");
			
			email = getenv("EMAIL");
			fprintf( fpOut, " * Created by: %s <%s>\n", username, email );
		}
		
		{
			struct tm *t = GetNowTime();
			fprintf( fpOut, " * Created on: %s */\n\n", asctime(t) );
		}
		
		
		fprintf
		(
			fpOut,
			"#ifndef _%s_H_\n"
			"#define _%s_H_\n"
			"\n",
			all_uppers, 
			all_uppers
		);
		
		// output the includes
		fprintf
		(
			fpOut,
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
			"\n"
		);
		
		if(self->m_bInline)
		{
			// output some nice deliniation comments
			fprintf
			(
				fpOut,
				"/*\n"
				" * %s Declaration\n"
				" */\n"
				"\n",
				self->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			"typedef struct td_test {\n"
			"\t/* TODO: put your data here */\n"
			"} %s, *%sPtr;\n"
			"\n"
			"\n",
			self->m_szClassName,
			self->m_szClassName
		);
		
		if(self->m_bInline)
		{
			fprintf
			(
				fpOut,
				"/*\n"
				" * %s Forward Declarations\n"
				" */\n"
				"\n",
				self->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			"%s* %s_new(void);\n"
			"void %s_delete(%s* self);\n"
			"gboolean %s_init(%s* self);\n"
			"void %s_end(%s* self);\n"
			"\n"
			"\n",
			self->m_szClassName,
			self->m_szClassName,
			self->m_szClassName,
			self->m_szClassName,
			self->m_szClassName,
			self->m_szClassName,
			self->m_szClassName,
			self->m_szClassName
		);
		
		if(!self->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	/*_%s_H_*/\n"
				"\n",
				all_uppers
			);
		}
	}	
	free(all_uppers);
}


static void
GenerateSource(CG_Creator *self, FILE* fpOut)
{
	int i;
	gchar* all_uppers = (gchar*)malloc((strlen(self->m_szClassName) + 1) * sizeof(gchar));
	strcpy(all_uppers, self->m_szClassName);
	for(i = 0; i < strlen(all_uppers); i++)
	{
		all_uppers[i] = toupper(all_uppers[i]);
	}
	
	if(strcmp(self->m_szClassType, "Generic C++ Class") == 0)
	{
		if(!self->m_bInline)
		{
			fprintf(fpOut, "//\n// File: %s\n", self->m_szDeclFile);
			
			{
				gchar* username;
				gchar* email;
				
				username = getenv ("USERNAME");
				if (!username)
					username = getenv ("USER");
				
				email = getenv("EMAIL");
				fprintf( fpOut, "// Created by: %s <%s>\n", username, email );
			}
			
			{
				struct tm *t = GetNowTime();
				fprintf( fpOut, "// Created on: %s//\n\n", asctime(t) );
			}
			
			fprintf
			(
				fpOut,
				"#include \"%s\"\n"
				"\n"
				"\n",
				self->m_szDeclFile
			);
		}
		else
		{
			fprintf
			(
				fpOut,
				"//------------------------------------------------------------------------------\n"
				"// %s Implementation\n"
				"//------------------------------------------------------------------------------\n"
				"\n\n",
				self->m_szClassName
			);
		}
		
		if(strlen(self->m_szBaseClassName) > 0)
		{
			// output constructor with inheritence
			fprintf
			(
				fpOut,
				"%s::%s() : %s()\n",
				self->m_szClassName,
				self->m_szClassName,
				self->m_szBaseClassName
			);
		}
		else
		{
			// output constructor without inheritence
			fprintf
			(
				fpOut,
				"%s::%s()\n",
				self->m_szClassName,
				self->m_szClassName
			);	
		}
	
		fprintf
		(
			fpOut, 
			"{\n"
			"\t// TODO: put constructor code here\n"
			"}\n"
			"\n"
			"\n"
			"%s::~%s()\n"
			"{\n"
			"\t// TODO: put destructor code here\n"
			"}\n"
			"\n"
			"\n", 
			self->m_szClassName, 
			self->m_szClassName
		);
		
		if(self->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	//_%s_H_\n"
				"\n",
				all_uppers
			);
		}
	}
	else if(strcmp(self->m_szClassType, "GTK+ Class") == 0)
	{
		if(!self->m_bInline)
		{
			
			fprintf(fpOut, "/*\n * File: %s\n", self->m_szDeclFile);
			
			{
				gchar* username;
				gchar* email;
				
				username = getenv ("USERNAME");
				if (!username)
					username = getenv ("USER");
				
				email = getenv("EMAIL");
				fprintf( fpOut, " * Created by: %s <%s>\n", username, email );
			}
			
			{
				struct tm *t = GetNowTime();
				fprintf( fpOut, " * Created on: %s */\n\n", asctime(t) );
			}
			
			fprintf
			(
				fpOut,
				"#include \"%s\"\n"
				"\n"
				"\n",
				self->m_szDeclFile
			);
		}
		else
		{
			// output some nice deliniation comments
			fprintf
			(
				fpOut,
				"/*\n"
				" * %s Implementation\n"
				" */\n"
				"\n",
				self->m_szClassName
			);
		}
		
		fprintf
		(
			fpOut,
			
			// output constructor
			"%s* %s_new(void)\n"
			"{\n"
			"\t%s* self;\n"
			"\tself = g_new(%s, 1);\n"
			"\tif(NULL != self)\n"
			"\t{\n"
			"\t\tif(!%s_init(self))\n"
			"\t\t{\n"
			"\t\t\tg_free(self);\n"
			"\t\t\tself = NULL;\n"
			"\t\t}\n"
			"\t}\n"
			"\treturn self;\n"
			"}\n"
			"\n"
			"\n"
			
			// output destructor
			"void %s_delete(%s* self)\n"
			"{\n"
			"\tg_return_if_fail(NULL != self);\n"
			"\t%s_end(self);\n"
			"\tg_free(self);\n"
			"}\n"
			"\n"
			"\n"
			
			// output init
			"gboolean %s_init(%s* self)\n"
			"{\n"
			"\t/* TODO: put init code here */\n"
			"\n"
			"\treturn TRUE;\n"
			"}\n"
			"\n"
			"\n"
			
			// output deinit
			"void %s_end(%s* self)\n"
			"{\n"
			"\t/* TODO: put deinit code here */\n"
			"}\n"
			"\n"
			"\n",
			self->m_szClassName, 
			self->m_szClassName,
			self->m_szClassName, 
			self->m_szClassName, 
			self->m_szClassName, 
			self->m_szClassName, 
			self->m_szClassName, 
			self->m_szClassName, 
			self->m_szClassName, 
			self->m_szClassName, 
			self->m_szClassName,
			self->m_szClassName
		);
		
		if(self->m_bInline)
		{
			fprintf
			(
				fpOut,
				"#endif	/*_%s_H_*/\n"
				"\n",
				all_uppers
			);
		}
	}
	free(all_uppers);
}


static void
CG_GetStrings(CG_Creator *self)
{
	SAFE_FREE(self->m_szClassName);
	SAFE_FREE(self->m_szDeclFile);
	SAFE_FREE(self->m_szImplFile);
	SAFE_FREE(self->m_szBaseClassName);
	SAFE_FREE(self->m_szAccess);
	SAFE_FREE(self->m_szClassType);
	self->m_szClassName = gtk_editable_get_chars(GTK_EDITABLE(self->entry_class_name),0, -1);
	self->m_szDeclFile = gtk_editable_get_chars(GTK_EDITABLE(self->entry_header_file),0, -1);
	self->m_szImplFile = gtk_editable_get_chars(GTK_EDITABLE(self->entry_source_file),0, -1);
	self->m_szBaseClassName = gtk_editable_get_chars(GTK_EDITABLE(self->entry_base_class), 0, -1);
	self->m_szAccess = gtk_editable_get_chars(GTK_EDITABLE(self->combo_access_entry), 0, -1);
	self->m_szClassType = gtk_editable_get_chars(GTK_EDITABLE(self->combo_class_type_entry), 0, -1);
	self->m_bVirtualDestructor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->checkbutton_virtual_destructor));
	self->m_bInline = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->checkbutton_inline));
}


/*
 *------------------------------------------------------------------------------
 * Operations
 *------------------------------------------------------------------------------
 */

void
CG_Init(CG_Creator *self, ProjectDBase *pDB)
{
	self->m_bOK = FALSE;
	self->m_bUserEdited = FALSE;
	self->m_bUserSelectedHeader = FALSE;
	self->m_bUserSelectedSource = FALSE;
	self->m_bVirtualDestructor = FALSE;
	self->m_bInline = FALSE;
	self->m_szClassName = NULL;
	self->m_szDeclFile= NULL;
	self->m_szImplFile= NULL;
	self->m_szBaseClassName = NULL;
	self->m_szAccess = NULL;
	self->m_szClassType = NULL;
	self->m_pDB = pDB;
}


void
CG_Del(CG_Creator *self)
{
	SAFE_FREE(self->m_szClassName);
	SAFE_FREE(self->m_szDeclFile);
	SAFE_FREE(self->m_szImplFile);
	SAFE_FREE(self->m_szBaseClassName);
	SAFE_FREE(self->m_szAccess);
	SAFE_FREE(self->m_szClassType);
}


void
CG_Show(CG_Creator *self)
{
	if(NULL != CreateDialogClass(self))
	{
		gtk_widget_show (self->dlgClass);
		gtk_widget_draw_focus(self->entry_class_name);
		gtk_widget_set_sensitive(self->button_browse_header_file, FALSE);
		gtk_widget_set_sensitive(self->button_browse_source_file, FALSE);
		gtk_widget_set_sensitive(self->button_finish, FALSE);
	}
}


void
CreateCodeClass(CG_Creator* self, ProjectDBase *pDB)
{
	CG_Init(self, pDB);
	CG_Show(self);
}


GtkWidget* 
CreateDialogClass(CG_Creator *self)
{
  self->combo_access_items = NULL;
  self->combo_class_type_items = NULL;
  self->header_file_selection = NULL;
  self->source_file_selection = NULL;
  self->tooltips = gtk_tooltips_new ();
	
  self->dlgClass = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (self->dlgClass), "dlgClass", self->dlgClass);
  gtk_window_set_title (GTK_WINDOW (self->dlgClass), _("Class Builder"));
  gtk_window_set_default_size (GTK_WINDOW (self->dlgClass), 640, 480);

  self->fixed = gtk_fixed_new ();
  gtk_widget_ref (self->fixed);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "fixed", self->fixed,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->fixed);
  gtk_container_add (GTK_CONTAINER (self->dlgClass), self->fixed);

  self->button_help = gtk_button_new_with_label (_("Help"));
  gtk_widget_ref (self->button_help);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button_help", self->button_help,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button_help);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->button_help, 544, 424);
  gtk_widget_set_uposition (self->button_help, 544, 424);
  gtk_widget_set_usize (self->button_help, 80, 24);

  self->button_cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (self->button_cancel);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button_cancel", self->button_cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button_cancel);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->button_cancel, 448, 424);
  gtk_widget_set_uposition (self->button_cancel, 448, 424);
  gtk_widget_set_usize (self->button_cancel, 80, 24);

  self->button_finish = gtk_button_new_with_label (_("Finish"));
  gtk_widget_ref (self->button_finish);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button_finish", self->button_finish,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button_finish);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->button_finish, 352, 424);
  gtk_widget_set_uposition (self->button_finish, 352, 424);
  gtk_widget_set_usize (self->button_finish, 80, 24);

  self->entry_class_name = gtk_entry_new ();
  gtk_widget_ref (self->entry_class_name);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "entry_class_name", self->entry_class_name,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->entry_class_name);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->entry_class_name, 24, 152);
  gtk_widget_set_uposition (self->entry_class_name, 24, 152);
  gtk_widget_set_usize (self->entry_class_name, 184, 24);
  gtk_tooltips_set_tip (self->tooltips, self->entry_class_name, _("Enter the name for the class you want to add."), NULL);

  self->entry_header_file = gtk_entry_new ();
  gtk_widget_ref (self->entry_header_file);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "entry_header_file", self->entry_header_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->entry_header_file);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->entry_header_file, 232, 152);
  gtk_widget_set_uposition (self->entry_header_file, 232, 152);
  gtk_widget_set_usize (self->entry_header_file, 160, 24);
  gtk_tooltips_set_tip (self->tooltips, self->entry_header_file, _("Enter the declaration file name."), NULL);

  self->entry_source_file = gtk_entry_new ();
  gtk_widget_ref (self->entry_source_file);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "entry_source_file", self->entry_source_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->entry_source_file);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->entry_source_file, 440, 152);
  gtk_widget_set_uposition (self->entry_source_file, 440, 152);
  gtk_widget_set_usize (self->entry_source_file, 160, 24);
  gtk_tooltips_set_tip (self->tooltips, self->entry_source_file, _("Enter the implementation file name."), NULL);

  self->button_browse_header_file = gtk_button_new_with_label (_("..."));
  gtk_widget_ref (self->button_browse_header_file);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button_browse_header_file", self->button_browse_header_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button_browse_header_file);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->button_browse_header_file, 392, 152);
  gtk_widget_set_uposition (self->button_browse_header_file, 392, 152);
  gtk_widget_set_usize (self->button_browse_header_file, 24, 24);
  gtk_tooltips_set_tip (self->tooltips, self->button_browse_header_file, _("Browse for the declaration file name."), NULL);

  self->button_browse_source_file = gtk_button_new_with_label (_("..."));
  gtk_widget_ref (self->button_browse_source_file);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "button_browse_source_file", self->button_browse_source_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->button_browse_source_file);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->button_browse_source_file, 600, 152);
  gtk_widget_set_uposition (self->button_browse_source_file, 600, 152);
  gtk_widget_set_usize (self->button_browse_source_file, 24, 24);
  gtk_tooltips_set_tip (self->tooltips, self->button_browse_source_file, _("Browse for the implementation file name."), NULL);

  self->entry_base_class = gtk_entry_new ();
  gtk_widget_ref (self->entry_base_class);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "entry_base_class", self->entry_base_class,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->entry_base_class);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->entry_base_class, 24, 200);
  gtk_widget_set_uposition (self->entry_base_class, 24, 200);
  gtk_widget_set_usize (self->entry_base_class, 184, 24);
  gtk_tooltips_set_tip (self->tooltips, self->entry_base_class, _("Enter the name of the class your new class will inherit from."), NULL);

  self->colormap = gtk_widget_get_colormap(self->dlgClass);
  self->gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, self->colormap, &self->mask,
						    NULL, class_logo_xpm);
  self->pixmap_logo = gtk_pixmap_new(self->gdkpixmap, self->mask);
  gdk_pixmap_unref(self->gdkpixmap);
  gdk_bitmap_unref(self->mask);
  gtk_widget_ref (self->pixmap_logo);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "pixmap_logo", self->pixmap_logo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->pixmap_logo);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->pixmap_logo, 568, 8);
  gtk_widget_set_uposition (self->pixmap_logo, 568, 8);
  gtk_widget_set_usize (self->pixmap_logo, 64, 64);

  self->hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (self->hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "hseparator1", self->hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->hseparator1);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->hseparator1, 0, 72);
  gtk_widget_set_uposition (self->hseparator1, 0, 72);
  gtk_widget_set_usize (self->hseparator1, 640, 16);

  self->label_description = gtk_label_new (_("This plugin will create a class of the type you specify and add it to your project. "));
  gtk_widget_ref (self->label_description);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_description", self->label_description,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_description);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_description, 32, 40);
  gtk_widget_set_uposition (self->label_description, 32, 40);
  gtk_widget_set_usize (self->label_description, 400, 16);

  self->label_class_name = gtk_label_new (_("Class name: "));
  gtk_widget_ref (self->label_class_name);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_class_name", self->label_class_name,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_class_name);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_class_name, 24, 136);
  gtk_widget_set_uposition (self->label_class_name, 24, 136);
  gtk_widget_set_usize (self->label_class_name, 64, 16);
  gtk_label_set_justify (GTK_LABEL (self->label_class_name), GTK_JUSTIFY_LEFT);

  self->label_header_file = gtk_label_new (_("Header file:  "));
  gtk_widget_ref (self->label_header_file);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_header_file", self->label_header_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_header_file);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_header_file, 232, 136);
  gtk_widget_set_uposition (self->label_header_file, 232, 136);
  gtk_widget_set_usize (self->label_header_file, 64, 16);
  gtk_label_set_justify (GTK_LABEL (self->label_header_file), GTK_JUSTIFY_LEFT);

  self->label_source_file = gtk_label_new (_("Source file:  "));
  gtk_widget_ref (self->label_source_file);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_source_file", self->label_source_file,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_source_file);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_source_file, 440, 136);
  gtk_widget_set_uposition (self->label_source_file, 440, 136);
  gtk_widget_set_usize (self->label_source_file, 64, 16);
  gtk_label_set_justify (GTK_LABEL (self->label_source_file), GTK_JUSTIFY_LEFT);

  self->label_base_class = gtk_label_new (_("Base class:   "));
  gtk_widget_ref (self->label_base_class);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_base_class", self->label_base_class,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_base_class);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_base_class, 24, 184);
  gtk_widget_set_uposition (self->label_base_class, 24, 184);
  gtk_widget_set_usize (self->label_base_class, 64, 16);
  gtk_label_set_justify (GTK_LABEL (self->label_base_class), GTK_JUSTIFY_LEFT);

  self->label_access = gtk_label_new (_("Base class inheritance: "));
  gtk_widget_ref (self->label_access);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_access", self->label_access,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_access);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_access, 232, 184);
  gtk_widget_set_uposition (self->label_access, 232, 184);
  gtk_widget_set_usize (self->label_access, 120, 16);

  self->hseparator3 = gtk_hseparator_new ();
  gtk_widget_ref (self->hseparator3);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "hseparator3", self->hseparator3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->hseparator3);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->hseparator3, 0, 232);
  gtk_widget_set_uposition (self->hseparator3, 0, 232);
  gtk_widget_set_usize (self->hseparator3, 640, 16);

  self->hseparator2 = gtk_hseparator_new ();
  gtk_widget_ref (self->hseparator2);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "hseparator2", self->hseparator2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->hseparator2);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->hseparator2, 0, 400);
  gtk_widget_set_uposition (self->hseparator2, 0, 400);
  gtk_widget_set_usize (self->hseparator2, 640, 16);

  self->hseparator4 = gtk_hseparator_new ();
  gtk_widget_ref (self->hseparator4);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "hseparator4", self->hseparator4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->hseparator4);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->hseparator4, 0, 456);
  gtk_widget_set_uposition (self->hseparator4, 0, 456);
  gtk_widget_set_usize (self->hseparator4, 640, 16);

  self->checkbutton_inline = gtk_check_button_new_with_label (_("Inline the declaration and implementation"));
  gtk_widget_ref (self->checkbutton_inline);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "checkbutton_inline", self->checkbutton_inline,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->checkbutton_inline);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->checkbutton_inline, 232, 104);
  gtk_widget_set_uposition (self->checkbutton_inline, 232, 104);
  gtk_widget_set_usize (self->checkbutton_inline, 224, 24);
  gtk_tooltips_set_tip (self->tooltips, self->checkbutton_inline, _("If checked, the plugin will generate both the declaration and implementation in the header file and will not create a source file."), NULL);

  self->label_author = gtk_label_new (_("By:  Dave Huseby <huseby@linuxprogrammer.org>"));
  gtk_widget_ref (self->label_author);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_author", self->label_author,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_author);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_author, 0, 464);
  gtk_widget_set_uposition (self->label_author, 0, 464);
  gtk_widget_set_usize (self->label_author, 256, 16);
  gtk_widget_set_sensitive (self->label_author, FALSE);
  gtk_label_set_justify (GTK_LABEL (self->label_author), GTK_JUSTIFY_LEFT);

  self->label_version = gtk_label_new (_("V 0.0.2 (June 11, 2002)"));
  gtk_widget_ref (self->label_version);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_version", self->label_version,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_version);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_version, 528, 464);
  gtk_widget_set_uposition (self->label_version, 528, 464);
  gtk_widget_set_usize (self->label_version, 112, 16);
  gtk_widget_set_sensitive (self->label_version, FALSE);
  gtk_label_set_justify (GTK_LABEL (self->label_version), GTK_JUSTIFY_RIGHT);

  self->label_todo = gtk_label_new (_("TODO: add the ability to declare member functions and their parameters here."));
  gtk_widget_ref (self->label_todo);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_todo", self->label_todo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_todo);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_todo, 120, 312);
  gtk_widget_set_uposition (self->label_todo, 120, 312);
  gtk_widget_set_usize (self->label_todo, 400, 16);
  gtk_widget_set_sensitive (self->label_todo, FALSE);

  self->label_welcome = gtk_label_new (_("Welcome to the Anjuta Class Builder. "));
  gtk_widget_ref (self->label_welcome);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_welcome", self->label_welcome,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_welcome);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_welcome, 32, 16);
  gtk_widget_set_uposition (self->label_welcome, 32, 16);
  gtk_widget_set_usize (self->label_welcome, 184, 16);

  self->label_class_type = gtk_label_new (_("Class type:   "));
  gtk_widget_ref (self->label_class_type);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "label_class_type", self->label_class_type,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->label_class_type);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->label_class_type, 24, 88);
  gtk_widget_set_uposition (self->label_class_type, 24, 88);
  gtk_widget_set_usize (self->label_class_type, 64, 16);
  gtk_label_set_justify (GTK_LABEL (self->label_class_type), GTK_JUSTIFY_LEFT);

  self->combo_access = gtk_combo_new ();
  gtk_widget_ref (self->combo_access);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "combo_access", self->combo_access,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->combo_access);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->combo_access, 232, 200);
  gtk_widget_set_uposition (self->combo_access, 232, 200);
  gtk_widget_set_usize (self->combo_access, 184, 24);
  self->combo_access_items = g_list_append (self->combo_access_items, (gpointer) _("public"));
  self->combo_access_items = g_list_append (self->combo_access_items, (gpointer) _("protected"));
  self->combo_access_items = g_list_append (self->combo_access_items, (gpointer) _("private"));
  gtk_combo_set_popdown_strings (GTK_COMBO (self->combo_access), self->combo_access_items);
  g_list_free (self->combo_access_items);

  self->combo_access_entry = GTK_COMBO (self->combo_access)->entry;
  gtk_widget_ref (self->combo_access_entry);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "combo_access_entry", self->combo_access_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->combo_access_entry);
  gtk_tooltips_set_tip (self->tooltips, self->combo_access_entry, _("Choose the type of inheritence from the base class."), NULL);
  gtk_entry_set_editable (GTK_ENTRY (self->combo_access_entry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (self->combo_access_entry), _("public"));

  self->checkbutton_virtual_destructor = gtk_check_button_new_with_label (_("Virtual destructor"));
  gtk_widget_ref (self->checkbutton_virtual_destructor);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "checkbutton_virtual_destructor", self->checkbutton_virtual_destructor,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->checkbutton_virtual_destructor);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->checkbutton_virtual_destructor, 440, 200);
  gtk_widget_set_uposition (self->checkbutton_virtual_destructor, 440, 200);
  gtk_widget_set_usize (self->checkbutton_virtual_destructor, 112, 24);
  gtk_tooltips_set_tip (self->tooltips, self->checkbutton_virtual_destructor, _("If checked, the destructor of your class will be declared virtual."), NULL);

  self->combo_class_type = gtk_combo_new ();
  gtk_widget_ref (self->combo_class_type);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "combo_class_type", self->combo_class_type,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->combo_class_type);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->combo_class_type, 24, 104);
  gtk_widget_set_uposition (self->combo_class_type, 24, 104);
  gtk_widget_set_usize (self->combo_class_type, 184, 24);
  self->combo_class_type_items = g_list_append (self->combo_class_type_items, (gpointer) _("Generic C++ Class"));
  self->combo_class_type_items = g_list_append (self->combo_class_type_items, (gpointer) _("GTK+ Class"));
  gtk_combo_set_popdown_strings (GTK_COMBO (self->combo_class_type), self->combo_class_type_items);
  g_list_free (self->combo_class_type_items);

  self->combo_class_type_entry = GTK_COMBO (self->combo_class_type)->entry;
  gtk_widget_ref (self->combo_class_type_entry);
  gtk_object_set_data_full (GTK_OBJECT (self->dlgClass), "combo_class_type_entry", self->combo_class_type_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self->combo_class_type_entry);
  gtk_tooltips_set_tip (self->tooltips, self->combo_class_type_entry, _("Select the type of class you would like to create."), NULL);
  gtk_entry_set_editable (GTK_ENTRY (self->combo_class_type_entry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (self->combo_class_type_entry), _("Generic C++ Class"));

  gtk_object_set_data (GTK_OBJECT (self->dlgClass), "tooltips", self->tooltips);

  // setup callbacks
  gtk_signal_connect (GTK_OBJECT (self->dlgClass), "delete_event",
					  GTK_SIGNAL_FUNC (on_delete_event),
					  self);
  gtk_signal_connect (GTK_OBJECT (self->button_browse_header_file), "clicked",
                      GTK_SIGNAL_FUNC (on_header_browse_clicked),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->button_browse_source_file), "clicked",
                      GTK_SIGNAL_FUNC (on_source_browse_clicked),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->button_finish), "clicked",
					  GTK_SIGNAL_FUNC (on_finish_clicked),
					  self);
  gtk_signal_connect (GTK_OBJECT (self->button_cancel), "clicked",
					  GTK_SIGNAL_FUNC (on_cancel_clicked),
					  self);
  gtk_signal_connect (GTK_OBJECT (self->button_help), "clicked",
					  GTK_SIGNAL_FUNC (on_help_clicked),
					  self);
  gtk_signal_connect (GTK_OBJECT (self->entry_class_name), "changed",
                      GTK_SIGNAL_FUNC (on_class_name_changed),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->combo_class_type_entry), "changed",
                      GTK_SIGNAL_FUNC (on_class_type_changed),
                      self);
  gtk_signal_connect (GTK_OBJECT (self->checkbutton_inline), "toggled",
					  GTK_SIGNAL_FUNC (on_inline_toggled),
					  self);
  gtk_widget_grab_focus(self->entry_class_name);
  return self->dlgClass;
}

