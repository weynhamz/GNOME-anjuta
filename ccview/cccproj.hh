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

#ifndef  _CCPROJ_HH__
#define  _CCPROJ_HH__
#include <string>
#include <fstream>
#include <regex.h>
#include <time.h>
#include <hash_map.h>
#include <set.h>
#include <list.h>
#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include "tree_coll.h"
#include "hash_str.h"
#include "cregex.hh"

const int BUFF_SIZE=1024;

/*
 * Gtk signals for ccview widget
 * declared here, because we need access here
 */
enum {
    GO_TO,
    CLASS_SELECTED,
	FUNCTION_SELECTED,
	FILE_SELECTED,
	UPDATE_START,
	UPDATING_FILE,
	UPDATE_END,
	ADD_TEXT,
	SAVE_FILE,
	UPDATE_CANCELLED,
    LAST_SIGNAL
};
extern guint ccview_signals[LAST_SIGNAL];

class ccproject; // this will be a good test...

typedef tree_coll<string>& noconst_dirref;
typedef const tree_coll<string>& dirref;
typedef tree_coll<string> *dirrefptr;

char *strip_comment(char *in,bool *incomment);

class file_context
{
	string dir;
    string filename;
    string full_filename;
	
	bool is_exist;
	bool is_regular;
	bool is_c_file;
	bool is_dir;
	time_t mtime;
	
	int linenum;
    char *buff;
	
	dirrefptr currdir;
	ifstream *str;
	bool error;

public:
    bool incomment;
    file_context();
	file_context(const file_context& ctx);
	file_context(dirrefptr cdir, const string& fdir, const string& fname)
	{
		set_file (cdir, fdir, fname);
	}
	~file_context(){stop();}
    void set_file(dirrefptr cdir, const string& fdir, const string& fname);
    
	int operator++(){return ++linenum;}
	
	bool start();
	bool stop();
    
	char *fgets(char *s, int size);
    char *fgets(){return fgets(buff, BUFF_SIZE);}
	
	bool get_is_regular() const {return is_regular;}
	bool get_is_c_file() const {return is_c_file;}
	bool get_is_exist() const {return is_exist;}
	bool get_is_directory() const {return is_dir;}
	time_t get_mtime() const {return mtime;};
    
	int get_linenum() const {return linenum;}
    const string& get_filename() const {return filename;}
    const string& get_full_filename() const {return full_filename;}
	const string& get_dir() const {return dir;}
	const ifstream& get_stream() const {return (*str);}
	const char* get_buffer() const {return buff;}
	dirrefptr get_currdir() {return currdir;}
	int get_error() {return error;}
	string get_relpath_filename() const;
};

class memberfunction
{
    string declfile;
    int declline;
    string deffile;
    int defline;
    unsigned int flags;
    string name;
	string prototype;
	ccproject* owner;
	
  public:
    void setflag(unsigned int flag);
    memberfunction()
    {
        declline=-1;
        defline=-1;
        flags=0;
		owner = NULL;
    }
    memberfunction(const memberfunction& in)
    {
        *this=in;
    }
    enum fnc_flags
    {
        fpublic=0,
        fprivate=0x1,
        fprotected=0x2,
        fstatic=0x4,
        fvirtual=0x8
    };
    memberfunction& operator=(const memberfunction& in);
	bool is_defined() const;
	bool is_declared() const;
	void setowner(ccproject* own){owner = own;}
    int getdeclline() const {return declline+1;}
    void setdeclline(int linein){declline=linein;}
    int getdefline() const {return defline+1;}
    void setdefline(int linein){defline=linein;}
    const char *getname() const {return name.c_str();}
    void setname(const char *namein){name=namein;}
    void setdeclfile(const char *namein) {declfile=namein;}
    const char *getdeclfile() const {return declfile.c_str();}
    void setdeffile(const char *namein){deffile=namein;}
    const char *getdeffile() const  {return deffile.c_str();}
    friend ostream& operator<<(ostream& str,const memberfunction& mf);
    unsigned int get_flags() const {return flags;}
	
	bool save_in_xml (ostream& str, int depth) const;
};

typedef hash_map<string,memberfunction,hash_str> funct_map;
typedef funct_map::const_iterator funct_pos;

class ccclass
{
protected:
    funct_map  functions;
    vector<string> parents;
    static executed_regex forward_dec_re;
    static executed_regex function_dec_re;
	bool is_struct;
    string name;
    string declfile;
    int declline;
    int endline;
    static char scratch[BUFF_SIZE];
    ccproject *owner;
	
    char * get_parents(char *buff, file_context& context);
    char *find_start(char *buf, file_context& context);
    void add_function_dec(const char *funname,const char *finame,int lnum,
                          unsigned int flag);
public:
    ccclass(){owner = NULL; is_struct = false;}
    ccclass(const ccclass& in)
    {
        *this=in;
    }
    bool is_defined() const;
	void set_is_struct(bool is_s){is_struct = is_s;}
	bool get_is_struct() const {return is_struct;}
    const char *getname() const {return name.c_str();}
    void setname(const char *namein){name=namein;}
    void setdeclfile(const char *namein){declfile=namein;}
    const char *getdeclfile() const {return declfile.c_str();}
    const char *getdeffile() const;
    int getdeclline() const {return declline+1;}
    ccclass& operator=(const ccclass& in);
    friend ostream& operator << (ostream& in,const ccclass& ccin);
    void add_function_def(const char *funname,const char *finame,int lnum);
    void scan_def(char *buff, file_context &context, bool is_struct);
    void setowner(ccproject *in){owner=in;}
    funct_pos get_first_function() const {return functions.begin();}
    funct_pos get_last_function() const {return functions.end();}
    const memberfunction& get_function(funct_pos in) const {return (*in).second;}
    const memberfunction& get_function_from_name(const char *nin) const
	{
		ccclass* cls=(ccclass*)this; // FIXME: Typecasting from const* to non-const
        return cls->functions[nin];
	}
    const char* get_file_from_function(const char *func);
    int get_line_from_function(const char *func);
    const char* get_decl_file_from_function(const char *func);
    int get_decl_line_from_function(const char *func);
    int get_new_decl_line(const char *section,bool *section_exists) const;
    int get_new_def_line() const;
		
	bool save_in_xml (ostream& str, int depth) const;
	bool load_from_xml (xmlNode* node);
};

typedef hash_map<string,ccclass,hash_str> class_map;
typedef class_map::const_iterator class_pos;
typedef list<string>::const_iterator file_pos;
typedef list<tree_coll<string> >::const_iterator dir_pos;
typedef hash_map<string,time_t,hash_str> file_time_map;

class Scanner
{
	ccproject* owner;
	list<file_context> contexts;
	list<file_context>::iterator cur_pos;
	list<file_context>::iterator last_pos;
	GtkWidget *progressbar, *win, *label;
	gint max_files;
	gint count;
	
	void (*end_callback)(gpointer data);
	gpointer notify_data;
	bool scanning;
	bool cancel_scanning;
	bool use_progressbar;
	
	void create_dialog();
	static gint scan_context (gpointer scanner);
	static void cancel_clicked(GtkObject* obj, gpointer scanner);

public:
	Scanner(ccproject* prj);
	~Scanner();
	void set_notify_end (void (*end_cb)(gpointer scanner), gpointer data)
	{
		end_callback = end_cb;
		notify_data = data;
	}
	void set_use_progressbar(bool use)
	{
		use_progressbar = use;
		if(!use)
			gtk_widget_hide(win);
		else if (scanning)
			gtk_widget_show(win);
	}
	void add(file_context &ctx){contexts.push_back (ctx); max_files++;}
	void start();
	void cancel(){cancel_scanning = true;}
	void clear();
};

struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

class ccproject
{
    tree_coll<string> files;
    file_time_map ftimes;
    bool use_automake;
    bool recurse_dirs;
	bool follow_includes;
    class_map classes;
    string directory;
	Scanner scanner;
	GtkObject* object;
	
    static executed_regex includeregex;
    static executed_regex memberfunctregex;
    static executed_regex classregex;
    static executed_regex structregex;
    
	bool scan_file(file_context& fcontext);
    bool check_completed_files(file_context& fcontext);
    bool check_for_include(char *buff, file_context& fcontext);
    bool check_for_class(char *buff, file_context & context);
    bool regex_check_for_class(char *buff, executed_regex & er,
            file_context & context, bool is_struct);
    bool check_for_member_function(char *buff, file_context &context);
    class_map::iterator get_class(const char *cname);
	
	bool file_update_required(file_context& context);

public:
	Scanner& get_scanner(){return scanner;}
	void set_object(GtkObject* obj){object = obj;}
    bool glob_scan_dirs_files(list<string>& dlist, list<string>& flist, const string& dir);
    void set_use_automake(bool in){use_automake=in;}
    void set_recurse(bool in){recurse_dirs=in;}
	void set_follow_includes(bool in){follow_includes=in;}
    ccproject(): scanner(this){use_automake=true; recurse_dirs=true; follow_includes=true;}
    bool  process_line(char *buff, file_context& fcontext);
    void setdirectory(const char *name);
    const char *getdirectory() const {return directory.c_str();}
	void clear();
    bool update();
    class_pos get_first_class() const {return classes.begin();}
    class_pos get_end_class() const {return classes.end();}
    const ccclass& get_class(class_pos in) const {return (*in).second;}
    ccclass& get_class_from_name(const char *n);
    friend ostream& operator<<(ostream& str,const ccproject& in);
    const char* get_file_from_function(const char *cl,const char *func);
    int get_line_from_function(const char *cl,const char *func);
    const char *get_decl_file_from_function(const char *cl,const char *func);
    int get_decl_line_from_function(const char *cl,const char *func);
    const char *get_file_from_class(const char *cl);
    int get_line_from_class(const char *cl);
    file_pos get_first_file(dir_pos dp){return (*dp).item_begin();}
    file_pos get_end_file(dir_pos dp){return (*dp).item_end();}
    file_pos get_first_file(dirref dr){return dr.item_begin();}
    file_pos get_end_file(dirref dr){return dr.item_end();}
    dirref gettopdir(){return files;}
    dir_pos get_first_dir(dirref dr){return dr.branch_begin();}
    dir_pos get_end_dir(dirref dr){return dr.branch_end();}
    string get_absolute_path (const string& path) const;
    void strip_undefined();
	
	bool save_in_xml (ostream& str, int depth);
	bool load_from_xml (xmlNode* node);
	
protected:
    bool automake_scan_dirs(list<string>& dlist, const string& dir);
    bool automake_scan_files(list<string>& flist, const string& dir);
    bool automake_scan_dirs_files(list<string>& dlist, list<string>& flist, const string& dir);
    bool get_dir_list(list<string>& dlist);
    bool extract_function(executed_regex& re, file_context &context);
    bool get_dir_file_list(list<string>& dlist, list<string>& flist, const string& dir);
    bool do_update(tree_coll<string> &currdir, const string& path);
	bool files_save_in_xml (ostream& str, dirref dptr, const string& path, int depth);
	bool files_load_from_xml (noconst_dirref dirptr, xmlNode* childs, const string& path);
	friend class Scanner;
};

#endif 
