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

#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <gnome.h>
#include "ccview_main.h"
#include "ccview.h"

static void ccview_project_class_init(CcviewProjectClass *_class);
static void ccview_project_init(CcviewProject *ccview_project);
static void ccview_project_destroy (GtkObject* obj);

/*
 * For certain reason, the signal enums has been put in ccview_main.h
 * and the ccview_signals variable is declared global
 */
guint ccview_signals[LAST_SIGNAL] = { 0 };

static CcviewMain* 
get_class_pointer(CcviewProject* prj)
{
	g_return_val_if_fail(prj != NULL, NULL);
	g_return_val_if_fail(CCVIEW_IS_PROJECT(prj), NULL);
	return (CcviewMain*)prj->ccview;
}

GtkType
ccview_project_get_type(void)
{
	static GtkType ccview_project_type=0;
	if(!ccview_project_type)
	{
		static const GtkTypeInfo ccview_project_info = 
		{	
			"CcviewProject",
			sizeof(CcviewProject),
			sizeof(CcviewProjectClass),
			(GtkClassInitFunc) ccview_project_class_init,
			(GtkObjectInitFunc) ccview_project_init,
			NULL,
			NULL,
			(GtkClassInitFunc)NULL,
		};
		ccview_project_type = gtk_type_unique(GTK_TYPE_VBOX, &ccview_project_info);
	}
	return(ccview_project_type);
}

static GtkVBoxClass* parent_class = NULL;

static void
ccview_project_class_init(CcviewProjectClass *klass)
{
	GtkObjectClass *object_class;
	parent_class = (GtkVBoxClass*)gtk_type_class (gtk_vbox_get_type ());
	object_class = (GtkObjectClass*)klass;
	
	object_class->destroy = ccview_project_destroy;
	
	ccview_signals[GO_TO] = gtk_signal_new(
								"go_to",
								GTK_RUN_LAST,
								object_class->type,
								GTK_SIGNAL_OFFSET(CcviewProjectClass, go_to),
								gtk_marshal_NONE__INT_POINTER,
								GTK_TYPE_NONE, 2,
								GTK_TYPE_STRING,
								GTK_TYPE_INT);
	
	ccview_signals[CLASS_SELECTED] = gtk_signal_new(
							   "class_selected",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, class_selected),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 2,
							   GTK_TYPE_STRING,
							   GTK_TYPE_STRING,
							   GTK_TYPE_INT);
	
	ccview_signals[FUNCTION_SELECTED] = gtk_signal_new(
							   "function_selected",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, function_selected),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 2,
							   GTK_TYPE_STRING,
							   GTK_TYPE_STRING,
							   GTK_TYPE_INT);
	
	ccview_signals[FILE_SELECTED] = gtk_signal_new(
							   "file_selected",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, file_selected),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 1,
							   GTK_TYPE_STRING);
	
	ccview_signals[UPDATE_START] = gtk_signal_new(
							   "update_start",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, update_start),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 1,
							   GTK_TYPE_STRING);
	
	ccview_signals[UPDATING_FILE] = gtk_signal_new(
							   "updating_file",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, updating_file),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 1,
							   GTK_TYPE_STRING);

	ccview_signals[UPDATE_END] = gtk_signal_new(
							   "update_end",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, update_end),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 1,
							   GTK_TYPE_STRING);
	
	ccview_signals[ADD_TEXT] = gtk_signal_new(
							   "add_text",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, add_text),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 3,
							   GTK_TYPE_STRING,
							   GTK_TYPE_INT,
							   GTK_TYPE_STRING);

	ccview_signals[SAVE_FILE] = gtk_signal_new(
							   "save_file",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET(CcviewProjectClass, save_file),
							   gtk_marshal_NONE__INT_POINTER,
							   GTK_TYPE_NONE, 1,
							   GTK_TYPE_STRING);

	ccview_signals[UPDATE_CANCELLED] = gtk_signal_new ("update_cancelled",
							   GTK_RUN_LAST,
							   object_class->type,
							   GTK_SIGNAL_OFFSET (CcviewProjectClass, update_cancelled),
							   gtk_signal_default_marshaller,
							   GTK_TYPE_NONE, 0);
	
	gtk_object_class_add_signals (object_class, ccview_signals, LAST_SIGNAL);

	klass->go_to = NULL;
	klass->class_selected = NULL;
}

static void
ccview_project_init(CcviewProject *prj)
{
	CcviewMain* ccview;

	ccview = new CcviewMain;
	ccview->set_object(GTK_OBJECT(prj));
	prj->ccview = (void*)ccview;
	prj->notebook = ccview->get_notebook();
	gtk_widget_ref (prj->notebook);
	gtk_box_pack_start (GTK_BOX (prj), prj->notebook, TRUE, TRUE, 0);
}

GtkWidget*
ccview_project_new()
{
	CcviewProject* ccview_project;
	
	ccview_project = (CcviewProject*)gtk_type_new (CCVIEW_TYPE_PROJECT);
	return GTK_WIDGET(ccview_project);
}

static void
ccview_project_destroy (GtkObject* obj)
{
	GtkWidget* prj;
	
	g_return_if_fail(obj != NULL);
	g_return_if_fail(CCVIEW_IS_PROJECT(obj));

	prj = GTK_WIDGET(obj);
	delete (CcviewProject*)(CCVIEW_PROJECT(prj)->ccview);
	gtk_widget_unref (CCVIEW_PROJECT(prj)->notebook);
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (obj);
}

void
ccview_project_set_directory(CcviewProject* prj, gchar *dir)
{
	CcviewMain* win;
	win = get_class_pointer (prj);
	g_return_if_fail(win != NULL);
	g_return_if_fail(dir != NULL);

	win->setdirectory(dir);
}

void
ccview_project_update(CcviewProject* prj)
{
	CcviewMain* win;
	win = get_class_pointer (prj);
	g_return_if_fail(win != NULL);
	
	win->update();
}

void
ccview_project_save(CcviewProject* prj)
{
	CcviewMain* win;
	win = get_class_pointer (prj);
	g_return_if_fail(win != NULL);
	win->save();
}

void
ccview_project_set_recursive (CcviewProject* prj, gboolean recurse)
{
	CcviewMain* win;
	win = get_class_pointer (prj);
	g_return_if_fail(win != NULL);

	win->getproj().set_recurse(recurse);
}

void
ccview_project_set_use_automake (CcviewProject* prj, gboolean use_automake)
{
	CcviewMain* win;
	win = get_class_pointer (prj);
	g_return_if_fail(win != NULL);

	win->getproj().set_use_automake(use_automake);
}

void
ccview_project_set_follow_includes (CcviewProject* prj, gboolean follow_includes)
{
	CcviewMain* win;
	win = get_class_pointer (prj);
	g_return_if_fail(win != NULL);

	win->getproj().set_follow_includes (follow_includes);
}

void
ccview_project_clear(CcviewProject* prj)
{
	CcviewMain* win;
	win = get_class_pointer (prj);
	g_return_if_fail(win != NULL);
	
	win->clear();
}
