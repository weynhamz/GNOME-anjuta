/*  CcviewProject widget
 *
 *  Original code by Ron Jones <ronjones@xnet.com>
 *  Complete rewrite by Naba Kumar <kh_naba@123india.com>
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

#include "ccview_main.h"
#include "gtk_help.h"
#include <set>
#include "func_dialog.h"

#include "ofolder.xpm"
#include "cfolder.xpm"
#include "file.xpm"
#include "class.xpm"
#include "struct.xpm"
#include "public.xpm"
#include "private.xpm"
#include "protected.xpm"

static void
destroy_node_data(CtreeNodeData* data)
{
	if(data)
		delete data;
}

mw_pane::mw_pane(CcviewMain *own)
{
    owner=own;
	current_data = NULL;
}

file_pane::file_pane(CcviewMain *own): mw_pane(own),fpopup(this)
{
    win=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_show (win);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    file_tree=gtk_ctree_new(1,0);
	gtk_widget_show (file_tree);
	gtk_clist_set_column_auto_resize (GTK_CLIST (file_tree), 0, TRUE);
    gtk_clist_set_selection_mode(GTK_CLIST(file_tree),GTK_SELECTION_BROWSE);
	gtk_container_add (GTK_CONTAINER(win), file_tree);
	gtk_ctree_set_line_style (GTK_CTREE(file_tree), GTK_CTREE_LINES_DOTTED);
	gtk_signal_connect (GTK_OBJECT (file_tree), "select_row",
				GTK_SIGNAL_FUNC(row_selected), this);
	gtk_signal_connect(GTK_OBJECT(file_tree),"button_press_event",
                           GTK_SIGNAL_FUNC(button_press),this);
	
	ofolder_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&ofolder_map, NULL, ofolder_xpm);
	cfolder_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&cfolder_map, NULL, cfolder_xpm);
	file_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&file_map, NULL, file_xpm);
}

void file_pane::clear()
{
    gtk_clist_clear (GTK_CLIST(file_tree));
	mw_pane::clear();
}

void file_pane::reload()
{
    GtkCTreeNode* root;
	gchar* dum_array[1];
	
	gtk_clist_freeze (GTK_CLIST (file_tree));
	clear();
	
	char* dir = (gchar*)owner->getproj().getdirectory();
	if (!dir) return;
	if (!strlen(dir)) return;
	char* ptr = g_basename(dir);
	
	dum_array[0] = ptr;
	root = gtk_ctree_insert_node (GTK_CTREE (file_tree),
					NULL, NULL, dum_array, 5, cfolder_pix,
					cfolder_map, ofolder_pix, ofolder_map, FALSE, TRUE);
	string path = "";
	CtreeNodeData* data = new CtreeNodeData(NTYPE_FOLDER, path);
	gtk_ctree_node_set_row_data_full(GTK_CTREE(file_tree),
			root, data, (GtkDestroyNotify)destroy_node_data);
	
	load_file_dir_tree(owner->getproj().gettopdir(), root, path);
	gtk_ctree_sort_recursive(GTK_CTREE(file_tree), root);
	gtk_clist_thaw (GTK_CLIST (file_tree));
}

void file_pane::load_file_dir_tree(dirref dptr, GtkCTreeNode *root, const string& path)
{
    GtkCTreeNode *item;
	gchar* dum_array[1];
    dir_pos ds,de;
    file_pos fs,fe;
    
	ds=owner->getproj().get_first_dir(dptr);
    de=owner->getproj().get_end_dir(dptr);
    fs=owner->getproj().get_first_file(dptr);
    fe=owner->getproj().get_end_file(dptr);
	if(ds == de && fs == fe) return;

	while(ds != de)
    {
		string newpath = path;
		dum_array[0] = (gchar *)(*ds).get_data().c_str();
		newpath += dum_array[0];
		item = gtk_ctree_insert_node (GTK_CTREE (file_tree),
						root, NULL, dum_array, 5, cfolder_pix,
						cfolder_map, ofolder_pix, ofolder_map, FALSE, FALSE);
		CtreeNodeData* data = new CtreeNodeData(NTYPE_FOLDER, path, newpath);
		gtk_ctree_node_set_row_data_full(GTK_CTREE(file_tree),
				item, data, (GtkDestroyNotify)destroy_node_data);
		newpath += '/';
        load_file_dir_tree(*ds, item, newpath);
        ds++;
    }
    while(fs != fe)
	{
		string newpath = path;
		dum_array[0] = (gchar *)(*fs).c_str();
		newpath += dum_array[0];
		item = gtk_ctree_insert_node (GTK_CTREE (file_tree),
						root, NULL, dum_array, 5, file_pix,
						file_map, file_pix, file_map, TRUE, FALSE);
		CtreeNodeData* data = new CtreeNodeData(NTYPE_FILE, path, newpath);
		gtk_ctree_node_set_row_data_full(GTK_CTREE(file_tree),
				item, data, (GtkDestroyNotify)destroy_node_data);
        fs++;
    }
}

void file_pane::button_press(GtkWidget *w, GdkEventButton *evb, gpointer data)
{
	file_pane *pane=(file_pane *)data;
	if(evb->button == 3)
	{
		pane->fpopup.popup((guint)evb->x_root,(guint)evb->y_root,evb->time);
	}
}

void
file_pane::row_selected (GtkCList * clist,
				   gint row,
				   gint column,
				   GdkEvent * event, gpointer user_data)
{
	gchar *name;
	GdkPixmap *pixc, *pixo;
	GdkBitmap *maskc, *masko;
	guint8 space;
	gboolean is_leaf, expanded;
	GtkCTreeNode *node;
	
	file_pane* pane = (file_pane*)user_data;

	g_return_if_fail (pane != NULL);
	node = gtk_ctree_node_nth (GTK_CTREE (pane->file_tree), row);
	pane->current_data = (CtreeNodeData*)
		gtk_ctree_node_get_row_data (GTK_CTREE (pane->file_tree),
					     GTK_CTREE_NODE (node));
	gtk_ctree_get_node_info (GTK_CTREE (pane->file_tree),
				 node,
				 &name, &space,
				 &pixc, &maskc, &pixo, &masko, &is_leaf,
				 &expanded);

	if (!pane->current_data)
		return;
	if (!is_leaf)
		return;
	if (event == NULL)
		return;
	if (event->type != GDK_2BUTTON_PRESS)
		return;
	if (((GdkEventButton *) event)->button != 1)
		return;
	pane->popup_open();
}

void file_pane::file_select_function(const char *fl)
{
	gtk_signal_emit (owner->get_object(), ccview_signals[GO_TO], fl, -1);
}

#define SHELL_COMMAND "gnome-terminal"
static void
do_shell_command(const char *path) 
{
	pid_t pid;
	pid=fork();
	if(pid==0)
	{
		chdir(path);
		system(SHELL_COMMAND);
		_exit(0);
	}
}

void file_pane::popup_shell_here()
{
	if (current_data == NULL) return;
	if (current_data->get_type() == NTYPE_FOLDER
		&& !current_data->get_para2().empty())
	{
		string dir = owner->getproj().get_absolute_path(current_data->get_para2());
		do_shell_command(dir.c_str());
	}
	else if (current_data->get_type() == NTYPE_FILE
		&& !current_data->get_para1().empty())
	{
		string dir = owner->getproj().get_absolute_path(current_data->get_para1());
		do_shell_command(dir.c_str());
	}
}

void file_pane::popup_open()
{
	if (current_data == NULL) return;
	if (current_data->get_type() == NTYPE_FILE
		&& !current_data->get_para2().empty())
	{
		string filename = owner->getproj().get_absolute_path(current_data->get_para2());
		file_select_function(filename.c_str());
	}
}

void file_pane::popup_properties()
{

}

class_pane::class_pane(CcviewMain *own) : mw_pane(own),cpopup(this)
{
    win=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_show (win);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    class_tree=gtk_ctree_new(1,0);
	gtk_widget_show (class_tree);
	gtk_clist_set_column_auto_resize (GTK_CLIST (class_tree), 0, TRUE);
    gtk_clist_set_selection_mode(GTK_CLIST(class_tree),GTK_SELECTION_BROWSE);
	gtk_container_add (GTK_CONTAINER(win), class_tree);
	gtk_ctree_set_line_style (GTK_CTREE(class_tree), GTK_CTREE_LINES_DOTTED);
	gtk_signal_connect (GTK_OBJECT (class_tree), "select_row",
			GTK_SIGNAL_FUNC(row_selected), this);
	gtk_signal_connect(GTK_OBJECT(class_tree),"button_press_event",
            GTK_SIGNAL_FUNC(button_press),this);

	class_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&class_map, NULL, class_xpm);
	struct_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&struct_map, NULL, struct_xpm);
	private_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&private_map, NULL, private_xpm);
	public_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&public_map, NULL, public_xpm);
	protected_pix = 
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(win),
			&protected_map, NULL, protected_xpm);
}

void class_pane::clear()
{
	gtk_clist_clear(GTK_CLIST (class_tree));
	mw_pane::clear();
}

void class_pane::reload()
{
	GtkCTreeNode *class_parent, *struct_parent;
	gchar* dum_array[1];
    
	//set<string> openclasses;
	//tree_save_expanded(GTK_TREE(class_tree),openclasses);
	//tree_restore_selection(GTK_TREE(class_tree),openclasses,sel_class,sel_func);
	
	/* Add the root in the ctree */
	gtk_clist_freeze (GTK_CLIST (class_tree));
	clear();
	
	dum_array[0] = "Classes";
	class_parent = gtk_ctree_insert_node (GTK_CTREE (class_tree),
					NULL, NULL, dum_array, 5, class_pix,
					class_map, class_pix, class_map, FALSE, FALSE);
	dum_array[0] = "Structs";
	struct_parent = gtk_ctree_insert_node (GTK_CTREE (class_tree),
					NULL, NULL, dum_array, 5, struct_pix,
					struct_map, struct_pix, struct_map, FALSE, FALSE);

	class_pos cptr = owner->getproj().get_first_class();
	class_pos cptr_last = owner->getproj().get_end_class();
    while(cptr != cptr_last)
    {
		const gchar* cls_name;
		GtkCTreeNode *item;
		
		const ccclass& cls = owner->getproj().get_class(cptr);
		cls_name = cls.getname();
		dum_array[0] = (gchar*)cls_name;
       
		if (cls.get_is_struct())
		{
			item = gtk_ctree_insert_node (GTK_CTREE (class_tree),
							struct_parent, NULL, dum_array, 5, struct_pix,
							struct_map, struct_pix, struct_map, FALSE, FALSE);
		}
		else
		{
			item = gtk_ctree_insert_node (GTK_CTREE (class_tree),
							class_parent, NULL, dum_array, 5, class_pix,
							class_map, class_pix, class_map, FALSE, FALSE);
		}
		string clstring = cls_name;
 		CtreeNodeData* data = new CtreeNodeData(NTYPE_CLASS, clstring);
		gtk_ctree_node_set_row_data_full(GTK_CTREE(class_tree),
				item, data, (GtkDestroyNotify)destroy_node_data);
		
        funct_pos fptr = cls.get_first_function();
		funct_pos fptr_last = cls.get_last_function();
        while(fptr != fptr_last)
        {
			GdkPixmap* p;
			GdkBitmap* m;
			GtkCTreeNode* subitem;
			
			const gchar* funct_name = cls.get_function(fptr).getname();
			dum_array[0] = (gchar*)funct_name;
			int flag=cls.get_function_from_name(funct_name).get_flags();
			if(flag & memberfunction::fprivate)
			{
				p = private_pix;
				m = private_map;
			}
			else if(flag & memberfunction::fprotected)
			{
				p = protected_pix;
				m = protected_map;
			}
			else
			{
				p = public_pix;
				m = public_map;
			}
			if (cls.get_is_struct())
			{
				subitem = gtk_ctree_insert_node (GTK_CTREE (class_tree),
								item, NULL, dum_array, 5, p,
								m, p, m, TRUE, FALSE);
			}
			else
			{
				subitem = gtk_ctree_insert_node (GTK_CTREE (class_tree),
								item, NULL, dum_array, 5, p,
								m, p, m, TRUE, FALSE);
			}
			string fnstring = funct_name;
			data = new CtreeNodeData(NTYPE_FUNCTION, clstring, fnstring);
			gtk_ctree_node_set_row_data_full(GTK_CTREE(class_tree),
					subitem, data, (GtkDestroyNotify)destroy_node_data);
            fptr++;
		}
        cptr++;
    }
	gtk_ctree_sort_recursive (GTK_CTREE(class_tree), class_parent);
	gtk_ctree_sort_recursive (GTK_CTREE(class_tree), struct_parent);
	gtk_clist_thaw(GTK_CLIST(class_tree));
}

void
class_pane::row_selected (GtkCList * clist,
				   gint row,
				   gint column,
				   GdkEvent * event, gpointer user_data)
{
	gchar *name;
	GdkPixmap *pixc, *pixo;
	GdkBitmap *maskc, *masko;
	guint8 space;
	gboolean is_leaf, expanded;
	GtkCTreeNode *node;
	
	class_pane* pane = (class_pane*)user_data;

	g_return_if_fail (pane != NULL);
	node = gtk_ctree_node_nth (GTK_CTREE (pane->class_tree), row);
	pane->current_data = (CtreeNodeData*)
		gtk_ctree_node_get_row_data (GTK_CTREE (pane->class_tree),
					     GTK_CTREE_NODE (node));
	gtk_ctree_get_node_info (GTK_CTREE (pane->class_tree),
				 node,
				 &name, &space,
				 &pixc, &maskc, &pixo, &masko, &is_leaf,
				 &expanded);

	if (!pane->current_data)
		return;
	if (!is_leaf)
		return;
	if (event == NULL)
		return;
	if (event->type != GDK_2BUTTON_PRESS)
		return;
	if (((GdkEventButton *) event)->button != 1)
		return;
	pane->popup_func_def();
}

void
class_pane::button_press(GtkWidget *w,
		GdkEventButton *evb, gpointer data)
{
	class_pane *pane=(class_pane *)data;
	if(evb->button==3)
	{
		pane->cpopup.popup((guint)evb->x_root,(guint)evb->y_root,evb->time);
	}
}

void class_pane::func_select_function(const char *cl,const char *func)
{
    string fn=owner->getproj().get_file_from_function(cl,func);
    int fline;
    fline=owner->getproj().get_line_from_function(cl,func);
    gtk_signal_emit (owner->get_object(), ccview_signals[GO_TO],
		owner->getproj().get_absolute_path(fn).c_str(), fline);
}

void class_pane::popup_addfunc()
{
 	func_dlg fd;
    int line;
    bool hastag;
    string tag;
    string tline;
    string declfname;
	string deffname;
	int type;
	
	if(!current_data) return;
	if(current_data->get_para1().empty())return;
	if(current_data->get_type() != NTYPE_FUNCTION
		&& current_data->get_type() != NTYPE_CLASS) return;

	string clname = current_data->get_para1();
	if(fd.gomodal())
    {
        type=fd.gettype();
        if(type==func_dlg::fpublic)
        {
            tag="public";
        }
        else if(type==func_dlg::fprotected)
        {
            tag="protected";
        }
        else
        {
            tag="private";
        }
        const ccclass&  cl=owner->getproj().get_class_from_name(clname.c_str());
        deffname = owner->getproj().get_absolute_path(cl.getdeffile());
        declfname = owner->getproj().get_absolute_path(cl.getdeclfile());
        
		gtk_signal_emit (owner->get_object(), ccview_signals[SAVE_FILE], deffname.c_str());
        gtk_signal_emit (owner->get_object(), ccview_signals[SAVE_FILE], declfname.c_str());
/*
        if(owner->getproj().isupdaterequired())
        {
            owner->getproj().update();
        }
*/
        line=cl.get_new_decl_line(tag.c_str(),&hastag);
        if(line >= 0)
        {
            if(!hastag)
            {
                tag += ":";
		        gtk_signal_emit (owner->get_object(), ccview_signals[ADD_TEXT],
					declfname.c_str(), line, tag.c_str());
                line++;
            }
            tline=fd.getfuncdecl(clname.c_str());
	        gtk_signal_emit (owner->get_object(), ccview_signals[ADD_TEXT],
					declfname.c_str(), line, tline.c_str());
	        gtk_signal_emit (owner->get_object(), ccview_signals[SAVE_FILE], declfname.c_str());
/*
            if(deffname == declfname)
            {
                owner->getproj().update();
            }
*/
            line=cl.get_new_def_line();
            if(line> 0)
            {
		        gtk_signal_emit (owner->get_object(), ccview_signals[SAVE_FILE], deffname.c_str());
                tline=fd.getfuncdef(clname.c_str());
		        gtk_signal_emit (owner->get_object(), ccview_signals[ADD_TEXT],
					deffname.c_str(), line, tline.c_str());
                line++;
	        	gtk_signal_emit (owner->get_object(), ccview_signals[ADD_TEXT],
					deffname.c_str(), line, "{");
                line++;
	        	gtk_signal_emit (owner->get_object(), ccview_signals[ADD_TEXT],
					deffname.c_str(), line, "\t/* FIXME: Add your code here. */");
                line++;
		        gtk_signal_emit (owner->get_object(), ccview_signals[ADD_TEXT],
					deffname.c_str(), line, "}");
		        gtk_signal_emit (owner->get_object(), ccview_signals[SAVE_FILE], deffname.c_str());
            }
        }
    }
}

void class_pane::popup_func_def()
{
	if(!current_data) return;
	if(current_data->get_type() != NTYPE_FUNCTION) return;
	if(current_data->get_para1().empty()
		|| current_data->get_para2().empty()) return;

	string cl = current_data->get_para1();
	string fn = current_data->get_para2();
	func_select_function(cl.c_str(), fn.c_str());
}

void class_pane::popup_func_decl()
{
	if(!current_data) return;
	if(current_data->get_type() != NTYPE_FUNCTION) return;
	if(current_data->get_para1().empty()
		|| current_data->get_para2().empty()) return;

	string cl = current_data->get_para1();
	string fn = current_data->get_para2();
    string fl = owner->getproj().get_decl_file_from_function(
        cl.c_str(),fn.c_str());
    int fline=owner->getproj().get_decl_line_from_function(
        cl.c_str(), fn.c_str());
    gtk_signal_emit (owner->get_object(), ccview_signals[GO_TO],
		owner->getproj().get_absolute_path(fl).c_str(), fline);
}

void class_pane::popup_class_def()
{
	if(!current_data) return;
	if(current_data->get_para1().empty())return;
	if(current_data->get_type() == NTYPE_FUNCTION
		|| current_data->get_type() == NTYPE_CLASS)
	{
		string cl = current_data->get_para1();
		string fn=owner->getproj().get_file_from_class(cl.c_str());
		int fline=owner->getproj().get_line_from_class(cl.c_str());
		gtk_signal_emit (owner->get_object(), ccview_signals[GO_TO],
			owner->getproj().get_absolute_path(fn).c_str(), fline);
	}
}

void class_pane::popup_properties()
{
}
