/*  CcviewProject widget
 *
 *  by Naba Kumar <kh_naba@123india.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _CCVIEW_H_
#define _CCVIEW_H_

#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#define CCVIEW_TYPE_PROJECT          (ccview_project_get_type())
#define CCVIEW_PROJECT(obj)		   (GTK_CHECK_CAST ((obj), CCVIEW_TYPE_PROJECT, CcviewProject))
#define CCVIEW_PROJECT_CLASS(klass)  (GTK_CHECK_CLASS_CAST ((klass), CCVIEW_TYPE_PROJECT, CcviewProjectClass))
#define CCVIEW_IS_PROJECT(obj)       (GTK_CHECK_TYPE ((obj), CCVIEW_TYPE_PROJECT))
#define CCVIEW_IS_PROJECT_CLASS(klass) (GTK_CHECK_TYPE ((klass), CCVIEW_TYPE_PROJECT))

typedef struct _CcviewProject CcviewProject;
typedef struct _CcviewProjectClass  CcviewProjectClass;

struct _CcviewProject
{
/* Public */
	GtkVBox hbox;
	
	/* Notebook where cpane and fpane are put */
	GtkWidget* notebook;
	
/* Private */
	void* ccview;
};

struct _CcviewProjectClass
{
	GtkVBoxClass parent_class;
	
	void (*go_to) (CcviewProject* prj,
			gchar* file,
			gint line);
	
	void (*class_selected) (CcviewProject* prj,
			gchar* classname,
			gchar* file,
			gint line);
	
	void (*function_selected) (CcviewProject* prj,
			gchar* funcname,
			gchar* file,
			gint line);
	
	void (*file_selected)  (CcviewProject* prj,
			gchar* file);
	
	void (*update_start)  (CcviewProject* prj,
			gchar* dir);

	void (*updating_file)  (CcviewProject* prj,
			gchar* file);
	
	void (*update_end)  (CcviewProject* prj,
			gchar* dir);
	
	void (*add_text)  (CcviewProject* prj,
			gchar* file,
			gint line,
			gchar* text);

	void (*save_file)  (CcviewProject* prj,
			gchar* file);
	void (*update_cancelled) (CcviewProject *prj);
};

/* Widget stuffs */
GtkType	ccview_project_get_type	(void);

GtkWidget* ccview_project_new		(void);

/* Functions */

void ccview_project_set_directory(CcviewProject* prj, gchar *dir);
void ccview_project_clear(CcviewProject* prj);
void ccview_project_update(CcviewProject* prj);
void ccview_project_save(CcviewProject* prj);

void ccview_project_set_recursive (CcviewProject* prj, gboolean recurse);
void ccview_project_set_use_automake (CcviewProject* prj, gboolean use_automake);
void ccview_project_set_follow_includes (CcviewProject* prj, gboolean follow_includes);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* _CCVIEW_H_ */
