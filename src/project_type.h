/* GPL Disclamer ... */

/* (c) Johannes Schmid */

#ifndef _PROJECT_TYPE_H
#define _PROJECT_TYPE_H

#include <glib.h>

typedef struct _Project_Type Project_Type;

struct _Project_Type
{
	gint id;
	gchar* name;
	gchar* save_string;
	
	gchar* cflags;
	gchar* ldadd;
	
	gchar* configure_macros;
	
	gchar* autogen_file;
	
	gboolean gnome_support;
	gboolean glade_support;
};

Project_Type* 
load_project_type(gint ID);

void 
free_project_type(Project_Type *type);

#endif
