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

#ifndef __MAIN_WIN_HH__
#define __MAIN_WIN_HH__
#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include "cccmd.h"
#include "cccproj.hh"

enum CtreeNodeDataType{
	NTYPE_CLASS,
	NTYPE_FUNCTION,
	NTYPE_FILE,
	NTYPE_FOLDER
};

class CtreeNodeData
{
	CtreeNodeDataType type;
	string para1;
	string para2;

public:
	CtreeNodeData(CtreeNodeDataType t, const string& p1)
	{
		type = t;
		para1 = p1;
	}
	CtreeNodeData(CtreeNodeDataType t, const string& p1, const string& p2)
	{
		type = t;
		para1 = p1;
		para2 = p2;
	}
	CtreeNodeDataType get_type() const {return type;}
	const string& get_para1() const {return para1;}
	const string& get_para2() const {return para2;}
};

class CcviewMain;
class mw_pane
{
protected:
    CcviewMain *owner;
    GtkWidget *win;
	CtreeNodeData* current_data;

public:
    mw_pane(CcviewMain *own);
    GtkWidget *get_window(){return win;}
	void clear() {current_data = NULL;};
};

class class_pane: public mw_pane
{
    GtkWidget *class_tree;
    class_popup cpopup;
	
	GdkPixmap *class_pix, *struct_pix, *private_pix, *public_pix, *protected_pix;
	GdkBitmap *class_map, *struct_map, *private_map, *public_map, *protected_map;
	
	static void button_press (GtkWidget *w,GdkEventButton *evb, gpointer data);
	static void row_selected (GtkCList * clist, gint row, gint column,
				   GdkEvent * event, gpointer user_data);
    void func_select_function(const char *cl,const char *func);
	
public:
    class_pane(CcviewMain *own);
	//~class_pane(){};
    void clear();
    void reload();
    void popup_class_def();
    void popup_func_def();
    void popup_func_decl();
    void popup_addfunc();
	void popup_properties();
};

class file_pane: public mw_pane
{
    GtkWidget *file_tree;
    file_popup fpopup;
	
	GdkPixmap *ofolder_pix, *cfolder_pix, *file_pix;
	GdkBitmap *ofolder_map, *cfolder_map, *file_map;

    void load_file_dir_tree(dirref dptr, GtkCTreeNode *root, const string& path);
	static void button_press (GtkWidget *w,GdkEventButton *evb,gpointer data);
	static void row_selected (GtkCList * clist, gint row, gint column,
				   GdkEvent * event, gpointer user_data);
    void file_select_function(const char *fl);

public:
    file_pane(CcviewMain *own);
	//~file_pane(){};
    void clear();
	void reload();
    void popup_shell_here();
    void popup_open();
    void popup_properties();
};

class CcviewMain
{
    class_pane  cpane;
    file_pane  fpane;
    ccproject proj;
	GtkWidget* notebook;
	GtkObject* object;
	
	bool save_in_xml();
	bool load_from_xml();
    void reload();
	
public:
    
	CcviewMain();
    ~CcviewMain();
    ccproject& getproj(){return proj;}
    const ccproject & getproj() const {return proj;}
	GtkWidget* get_notebook(){return notebook;}
    static void scanning_ended(CcviewMain* cm);
	void set_object(GtkObject* obj){object = obj; getproj().set_object(obj);}
    GtkObject* get_object(){return object;}
	void clear();
	bool update();
	bool save(){return save_in_xml();}
	bool setdirectory(const char* dir);
};

#endif
