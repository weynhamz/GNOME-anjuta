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

#include <glob.h>
#include <string.h>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "cccproj.hh"
#include "support.h"
#include "scanner.xpm"

executed_regex ccproject::includeregex("^#include +\"(.*)\"",REG_EXTENDED,1);
executed_regex ccproject::memberfunctregex( "[ \t]*([^ :]+) *:: *([^ \\(]+) *\\(",
                                            REG_EXTENDED,2);
executed_regex ccproject::classregex("^[ \t]*class[ \t]+([^ \t:,\n]+)"
                                     ,REG_EXTENDED,1);
executed_regex ccproject::structregex("^[ \t]*struct[ \t]+([^ \t:,\n]+)",
                                      REG_EXTENDED,1);

char ccclass::scratch[]="";
executed_regex ccclass::forward_dec_re(";[ \t]*$",REG_EXTENDED,0);
executed_regex ccclass::function_dec_re("([^\({:;\\*\\.\\\\/ \t\n&\\!]+) *\\(",
                                        REG_EXTENDED,1);

static string 
xml_encode(const string& str)
{
	string ret;
	char* chars;
	chars = (char*)xmlEncodeEntitiesReentrant(NULL, (xmlChar*)str.c_str());
	ret = chars;
	if(chars) g_free(chars);
	return ret;
}

static const char*
xml_get_prop(xmlNode* node, const char* key)
{
	return (const char*)xmlGetProp(node, (const xmlChar*)key);
}

inline static void 
outtabs(ostream & str, int depth)
{
	for (int i=0; i<depth; i++) str << "\t";
}

static bool
file_is_directory (const char * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return false;
	ret = stat (fn, &st);
	if (ret)
		return false;
	if (S_ISDIR (st.st_mode))
		return true;
	return false;
}

static bool
file_is_regular (const char * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return false;
	ret = stat (fn, &st);
	if (ret)
		return false;
	if (S_ISREG (st.st_mode))
		return true;
	return false;
}

const char*
get_file_extension (const char * file)
{
	char *pos;
	if (!file)
		return NULL;
	pos = strrchr (file, '.');
	if (pos)
		return ++pos;
	else
		return NULL;
}

static bool
file_is_c_file (const char * fn)
{
	const char* fileext = get_file_extension(fn);
	if (!fileext)
		return false;
	if (strcmp (fileext, "c")==0
		|| strcmp (fileext, "cc")==0
		|| strcmp (fileext, "cpp")==0
		|| strcmp (fileext, "h")==0
		|| strcmp (fileext, "hh")==0
		|| strcmp (fileext, "cxx")==0)
		return true;
	return false;
}

bool does_file_exist(const char *fname);

bool does_file_exist(const char *fname)
{
    ifstream f(fname);
    bool res=true;
    if(!f)
    {
        res=false;
    }
    return res;
}


template <class InputIterator>
int get_bracelevel(InputIterator first,InputIterator last)
{

    int res=0;
    while(first != last)
    {
        if(*first == '{') res ++;
        else if(*first == '}') res --;
        first++;
    }
    return res;
}

char *strip_strings(char *in)
{
    char *ptr;
    char *startstring=strchr(in,'\"');
    while(startstring != NULL)
    {
        ptr=strchr(startstring+1,'\"');
        if(ptr !=NULL)
        {
            memmove(startstring,ptr+1,strlen(ptr+1)+1);
            startstring=strchr(in,'\"');
        }
        else
        {
            *startstring='\0';
            startstring=NULL;
        }
    }
    return in;
}    

char * strip_comment(char *in,bool *incomment)
{
    char *res=in;
    strip_strings(in);
    char *ptr=strstr(in,"//"); // first strip C++ style
    if(ptr != NULL)
    {
        *ptr='\0';
    }
        /* now look at C style */
    if(*incomment)
    {
        res=strstr(in,"*/");
        if(res==NULL)
        {
            res=in + strlen(in);
            return res; // still in commaent
        }
        *incomment=false;
    }
    char *commstart;
	//char *commend;
    do
    {
        commstart= strstr(res,"/*");
        if(commstart !=NULL)
        {
            ptr=strstr(commstart,"*/");
            if(ptr==NULL)
            {
                *incomment=true;
                *commstart='\0';
                return res;
            }
            memmove(commstart,ptr+2,strlen(ptr+2)+1);
        }
    } while(commstart != NULL);
    return res;
}

file_context::file_context()
{
	linenum=-1;
	incomment=false;
	currdir = NULL;
	error=0;
	buff = NULL;
	str = NULL;
	mtime = 0;

	is_exist = false;
	is_c_file = false;
	is_dir = false;
	is_regular = false;
}

file_context::file_context(const file_context& ctx)
{
	linenum=-1;
	incomment=false;
	error = false;
	buff = NULL;
	str = NULL;
	dir = ctx.dir;
	currdir = ctx.currdir;
	filename = ctx.filename;
	full_filename = ctx.full_filename;
	mtime = 0;
	
	is_exist = ctx.is_exist;
	is_regular = ctx.is_regular;
	is_c_file = ctx.is_c_file;
	is_dir = ctx.is_dir;
	
	if (ctx.buff)
	{
		cerr << "Warning: Creating file_context from an already started context\n";
		start();
	}
}

bool file_context::start()
{
	if (buff)
	{
		cerr << "Warning: Context already started\n";
		return false; // Already started.
	}
	if (filename.empty())
		return false;
	buff = new char[BUFF_SIZE];
	str = new ifstream;
	str->open(full_filename.c_str());
	error = !(*str);
	linenum=-1;
	if(error)
	{
		cerr<<"Error opening file "<< full_filename << endl;
	}
	return error;
}

bool file_context::stop()
{
	if (!buff) return false;
	str->close();
	error =!(*str);
	delete buff;
	delete str;
	buff = NULL;
	str = NULL;
	return error;
}

char* file_context::fgets(char *s, int size)
{
	if (!buff)
	{ // Not started.
		cerr << "Warning: Context not yet started\n";
		return NULL;
	}
	str->getline(s,size);
	error = !(*str);
	if(!error) linenum++;
	return s;
}

void file_context::set_file(dirrefptr cdir, const string& fdir, const string& fname)
{
	filename = fname;
	dir = fdir;
	
	currdir = cdir;
	linenum = -1;
	incomment = false;
	mtime = 0;
	is_regular = false;
	is_exist = false;
	is_c_file = false;
	is_dir = false;
	if (!fname.empty())
	{
		struct stat fstat;
		
		full_filename = dir;
		full_filename += fname;
		if(stat(full_filename.c_str(),&fstat))return;
		mtime = fstat.st_mtime;
		
		is_regular = S_ISREG (fstat.st_mode);
		is_dir = S_ISDIR (fstat.st_mode);
		is_exist = (is_regular || is_dir);
		if(is_regular)
			is_c_file = file_is_c_file(full_filename.c_str());
	}
}

string file_context::get_relpath_filename() const
{
    string res = filename;
	if (filename.empty()) return res;

    string dtmp;
    tree_coll<string> *dptr = currdir;
    do
    {
        dtmp=dptr->get_data();
        if(dtmp.empty()==false)
        {
            dtmp += '/';
            res.insert(0,dtmp);
        }
        dptr=dptr->getparent();
    }
    while(dptr!=NULL);
    return res;
}

void memberfunction::setflag(unsigned int flag)
{
    if(flag == fpublic || flag==fprivate || flag== fprotected)
    {
        flags &= 0x0c;
    }
    flags |= flag;
}
memberfunction& memberfunction::operator=(const memberfunction& in)
{
    name=in.name;
    declfile=in.declfile;
    declline=in.declline;
    deffile=in.deffile;
    defline=in.defline;
    flags=in.flags;
	owner = in.owner;
    return *this;
}

bool memberfunction::is_defined() const
{
	if (deffile.empty())
		return false;
	string filename = owner->get_absolute_path(deffile);
	return file_is_regular (filename.c_str());
}

bool memberfunction::is_declared() const
{
	if (declfile.empty())
		return false;
	string filename = owner->get_absolute_path(declfile);
	return file_is_regular (filename.c_str());
}

bool memberfunction::save_in_xml (ostream& str, int depth) const
{
	bool ret = true;
	outtabs (str, depth);
	str << "<memberfunction";
	str << " name=\"" << xml_encode(name) <<"\"";
	str << " declfile=\"" << xml_encode(declfile) <<"\"";
	str << " declline=\"" << declline <<"\"";
	str << " deffile=\"" << xml_encode(deffile) <<"\"";
	str << " defline=\"" << defline <<"\"";
	str << " flags=\"" << flags << "\"";
	str << " prototype=\"" << xml_encode(prototype) <<"\"/>\n";
	return ret;
}

//cclass
ccclass& ccclass::operator=(const ccclass& in)
{
    name =in.name;
    declfile=in.declfile;
    declline=in.declline;
    endline=in.endline;
    functions=in.functions;
    owner=in.owner;
	is_struct = in.is_struct;
    return *this;
}

ostream& operator<<(ostream& str,const memberfunction& mf)
{
    str<<mf.name<<", "<<mf.declfile<<", "<<mf.declline
       <<", "<<mf.deffile<<", "<<mf.defline;
    return str;
}
    
ostream& operator<<(ostream& in,const ccclass& ccin)
{
    funct_map::const_iterator ptr;
    in<<"CLASS, "<<ccin.name<<", "<<ccin.declfile<<", "<<ccin.declline<<endl;
    if(!ccin.parents.empty())
    {
        in<<"PARENTS: ";
        vector<string>::const_iterator pptr;
        for(pptr=ccin.parents.begin();pptr != ccin.parents.end();pptr++)
        {
            in<<(*pptr)<<" ";
        }
        in << endl;
    }
    for(ptr=ccin.functions.begin();ptr != ccin.functions.end();ptr++)
    {
        in<<"F, "<< (*ptr).second <<endl;
    }
    return in;
}    

bool ccclass::is_defined() const
{
	string filename;
	if (declfile.empty())
		return false;
	filename = owner->get_absolute_path(declfile);
	return file_is_regular (filename.c_str());
}

char *  ccclass::get_parents(char *ptr, file_context& context)
{
    char *scr_ptr;
    scr_ptr=strchr(ptr,':');
    if(scr_ptr!=NULL)
    {
        ptr=scr_ptr;
        ptr++;
        bool done=false;
        bool parent_flag=false;
        //size_t offset=0;
        while(!done)
        {
            strncpy(scratch,ptr,BUFF_SIZE);
            scr_ptr=strpbrk(scratch,"():{};");
            if(scr_ptr != NULL)
            {
                done=true;
                *scr_ptr='\0';
                ptr += strlen(scratch);
            }
            scr_ptr=strtok(scratch," \t,");
            if(scr_ptr != NULL)
            {
                if(parent_flag || strcmp(scr_ptr,"public")==0 ||
                   strcmp(scr_ptr,"private")==0)
                {
                    parent_flag=true;
                    while(scr_ptr != NULL)
                    {
                        scr_ptr=strtok(NULL," \t,\n");
                        if(scr_ptr !=NULL)
                        {
                            parents.push_back(scr_ptr);
                        }
                    }
                }
            }
            if(!done)
            {
                ptr=context.fgets();
                if(context.get_error())return NULL;
                if(owner != NULL && owner->process_line(ptr, context)) done=true;
            }
        }
    }
    return ptr;
}

const char *ccclass::getdeffile() const
{
    string res;
    funct_pos ptr=get_first_function();
    while(ptr != get_last_function())
    {
        res=(*ptr).second.getdeffile();
        if(res != declfile)
        {
            break;
        }
        ptr++;
    }
    if(res.empty())
    {
        res=declfile;
    }
    return res.c_str();
}
int ccclass::get_new_def_line() const
{
    int line=0;
    int fline;
    string deffile=getdeffile();
    funct_pos ptr=get_first_function();
    while(ptr!=get_last_function())
    {
        fline=(*ptr).second.getdefline();
        if(deffile==(*ptr).second.getdeffile())
        {
            line=max(line,fline);
        }       
        ptr++;
    }
    return line-1;
}
        
    
int ccclass::get_new_decl_line(const char *section,bool *section_exists)const
{
    char buff[BUFF_SIZE];
    string sect_tag=section;
    sect_tag += ":";
    ifstream f(declfile.c_str());
    int res=-1;
    int i;
    //char *ptr;
    *section_exists=false;
    if(f)
    {
        for(i=0;i<=declline;i++)
        {
            f.getline(buff,BUFF_SIZE);
        }
        while(i <= endline && f)
        {
            f.getline(buff,BUFF_SIZE);
            if(strstr(buff,sect_tag.c_str())!= NULL)
            {
                res=i+1;
                *section_exists=true;
                break;
            }
            i++;
        }
    }
    if(*section_exists==false)
    {
        res=endline;
    }
    return res;
}        
    
char * ccclass::find_start(char *buff, file_context& context)
{
    char *res=strchr(buff,'{');
    while(res==NULL && !context.get_error())
    {
        buff=context.fgets();
        if(owner != NULL && owner->process_line(buff, context))
        {
            return NULL;
        }
        res=strchr(buff,'{');
    }
    return res;
}

void ccclass::scan_def(char *buff, file_context& context, bool is_s)
{
    if(forward_dec_re.regexec(buff)==0)
        return; // forward declaration
	is_struct = is_s;
    declfile=context.get_relpath_filename();
    declline=context.get_linenum();
    char *ptr=strstr(buff,name.c_str());
    ptr += name.size();
    ptr=get_parents(ptr, context);
    ptr=find_start(ptr, context);
    int brace_depth=0;
    if (ptr != NULL) brace_depth=1;
    ptr++;
    char *name_ptr;
    unsigned int curflag;
    if(is_struct)
        curflag=memberfunction::fpublic;
    else
        curflag=memberfunction::fprivate;
    while(brace_depth > 0)
    {
        if(function_dec_re.regexec(ptr)==0)
        {
            name_ptr=strstr(ptr,(*function_dec_re.begin()).c_str());
            brace_depth += get_bracelevel(ptr,name_ptr);
            if(brace_depth==1)
            {
               add_function_dec( (*function_dec_re.begin()).c_str(),
                                 context.get_relpath_filename().c_str(), context.get_linenum(),
                                 curflag);
            }
            brace_depth += get_bracelevel(name_ptr,ptr+strlen(ptr));
        }
        else
        {
            if(strstr(ptr,"private:")) curflag=memberfunction::fprivate;
            else if(strstr(ptr,"protected:")) curflag=memberfunction::fprotected;
            else if(strstr(ptr,"public:")) curflag=memberfunction::fpublic;
            brace_depth += get_bracelevel(ptr,ptr+strlen(ptr));
        }
        if(brace_depth > 0)
        {
            ptr=context.fgets();
            if(owner != NULL && owner->process_line(ptr, context)) brace_depth=0;
        }
    }
    endline=context.get_linenum();
}

void ccclass::add_function_dec(const char *funname,const char *finame,int lnum,
                               unsigned int flag)
{
    funct_map::iterator ptr=functions.find(funname);
    if(ptr==functions.end())
    {
        functions[funname].setname(funname);
		functions[funname].setowner(owner);
        ptr=functions.find(funname);
        if(ptr==functions.end())
            return;
    }
    (*ptr).second.setdeclfile(finame);
    (*ptr).second.setdeclline(lnum);
    if((*ptr).second.is_defined()==false)
    {
        (*ptr).second.setdeffile(finame);
        (*ptr).second.setdefline(lnum);
    }
    (*ptr).second.setflag(flag);
}
        

void ccclass::add_function_def(const char *funname,const char *finame,int lnum)
{
    funct_map::iterator ptr=functions.find(funname);
    if(ptr==functions.end())
    {
        functions[funname].setname(funname);
		functions[funname].setowner(owner);
        ptr=functions.find(funname);
        if(ptr==functions.end())
            return;
    }
    (*ptr).second.setdefline(lnum);
    (*ptr).second.setdeffile(finame);
}
const char * ccclass::get_file_from_function(const char *func) 
{
    return functions[func].getdeffile();
}
int ccclass::get_line_from_function(const char *func) 
{
    return functions[func].getdefline();
}
const char * ccclass::get_decl_file_from_function(const char *func) 
{
    return functions[func].getdeclfile();
}
int ccclass::get_decl_line_from_function(const char *func)
{
    return functions[func].getdeclline();
}

bool ccclass::save_in_xml (ostream& str, int depth) const
{
	bool ret = true;
	outtabs(str, depth);
	str << "<class";
	
	// Save class parameters
	str <<" is_struct=";
	if (is_struct) str << "\"1\"";
	else str <<"\"0\"";
	str << "  name=\"" << xml_encode(name) <<"\"";
	str << " declfile=\"" << xml_encode(declfile) <<"\"";
	str << " declline=\"" << declline << "\"";
	str << " endline=\"" << endline << "\">\n";
	
	// FIXME: Save parents vector
	
	// Save member functions
	funct_pos fptr = get_first_function();
	funct_pos fptr_last = get_last_function();
	while(fptr != fptr_last)
	{
		ret = get_function(fptr).save_in_xml (str, depth+1);
		if (!ret) break;
		fptr++;
	}
	outtabs(str, depth);
	str << "</class>\n";
	return ret;
}

bool ccclass::load_from_xml(xmlNode* xmlnode)
{
	if (!xmlnode) return false;
	if (strcmp ((char*)xmlnode->name, "class")!=0) return false;

	//Load class parameters.
	name = (char*)xml_get_prop(xmlnode, "name");
	is_struct = atoi(xml_get_prop(xmlnode, "is_struct"));
	declfile = (char*)xml_get_prop(xmlnode, "declfile");
	declline = atoi (xml_get_prop(xmlnode, "declline"));
	endline = atoi (xml_get_prop(xmlnode, "endline"));
	
	//FIXME: Load parents here.
	
	//Load memberfunctions.
	xmlNode *node = xmlnode->childs;
	while (node)
	{
		if (strcmp ((char*)node->name, "memberfunction")==0)
		{
			add_function_dec ((const char*)xml_get_prop(node, "name"),
				(const char*)xml_get_prop(node, "declfile"), atoi(xml_get_prop(node, "declline")),
				(unsigned int)atoi(xml_get_prop(node, "flags")));
			add_function_def ((const char*)xml_get_prop(node, "name"),
				(const char*)xml_get_prop(node, "deffile"), atoi(xml_get_prop(node, "defline")));
		}
		node = node->next;
	}
	return true;
}

// ccproject
ostream& operator<<(ostream& str,const ccproject& in)
{
    class_map::const_iterator ptr;
    for(ptr=in.classes.begin();ptr != in.classes.end();ptr++)
    {
        if((*ptr).second.is_defined() && str)
        {
            str<< (*ptr).second;
        }
    }
    return str;
}
void ccproject::strip_undefined()
{
    class_map::iterator ptr,tmp_ptr;
    ptr=classes.begin();
    while(ptr != classes.end())
    {
        tmp_ptr=ptr++;
        if((*tmp_ptr).second.is_defined()==false)
        {
            classes.erase(tmp_ptr);
        }
    }
}

class_map::iterator ccproject::get_class(const char *cname)
{
    class_map::iterator res=classes.find(cname);
    if(res==classes.end())
    {
        classes[cname].setname(cname);
        res=classes.find(cname);
        (*res).second.setowner(this);
    }
    return res;
}

bool ccproject::regex_check_for_class(char *buff,executed_regex & er,
                                      file_context & context,
                                      bool is_struct)
{
    bool res=false;
    if(er.regexec(buff)==0)
    {
        string class_str= *(er.begin());
        if(class_str.length() > 0)
        {
            (*get_class(class_str.c_str())).second.scan_def(buff, context, is_struct);
            res=true;
        }
    }
    return res;
}

ccclass& ccproject::get_class_from_name(const char *n) 
{
    return classes[n];
}
    
bool ccproject::check_for_class(char *buff, file_context & context)
{
    bool res=regex_check_for_class(buff, classregex, context, false);
    if(!res) res=regex_check_for_class(buff, structregex, context, true);
    return res;
}

bool ccproject::check_for_member_function(char *buff, file_context & context)
{
    bool res=false;
    //vector<string>::const_iterator ptr;
    if(memberfunctregex.regexec(buff)==0)
    {
        res=extract_function(memberfunctregex,context);
    }
    return res;
}
bool ccproject::extract_function(executed_regex& re,file_context &context)
{
    bool res=false;
    string class_str,funct_str;
    vector<string>::const_iterator ptr=re.begin();
    class_str= *ptr;
    ptr++;
    funct_str=*ptr;
    if(funct_str.length() > 0)
    {
        (*get_class(class_str.c_str())).second.
            add_function_def(funct_str.c_str(),
               context.get_relpath_filename().c_str(),context.get_linenum());
        res=true;
    }
	return res;
}
    
        
bool  ccproject::process_line(char *buff,file_context &context)
{
    bool res=check_for_include(buff, context);
    if(!res)
    {
        strip_comment(buff,&context.incomment);
        res=check_for_class(buff, context);
        if(!res)
        {
            res=check_for_member_function(buff, context);
        }
    }
    return res;
}

void ccproject::setdirectory(const char *name)
{
    directory=name;
    if(directory[directory.length()-1] != '/')
        directory += '/';
}
bool ccproject::automake_scan_dirs(list<string>& dlist, const string& dir)
{
    executed_regex subdirs("SUBDIRS *=",REG_EXTENDED,0);
    if(!subdirs.verify_regex(cerr))
        return false;
    ifstream file;
    string line,tmp;
    string::size_type p1,p2;
    bool res=false;
    file.open("Makefile.am");
    while(file)
    {
        getline(file,line);
        if(file && subdirs.regexec(line.c_str())==0)
        {
            while(file && line[line.length()-1]=='\\')
            {
                line.erase(line.length()-1);
                getline(file,tmp);
                line += tmp;
            }
            res=true;
            p1=line.find('=');
            p1=line.find_first_not_of(" \t",p1+1);
            p2=0;
            while(p2 != string::npos && p1 != string::npos)
            {
                p2=line.find_first_of(" \t",p1+1);
                if(p2==string::npos)
                {
                    tmp=line.substr(p1);
                }
                else
                {
                    tmp=line.substr(p1,p2-p1);
                    p1=line.find_first_not_of(" \t",p2);
                }
                if(!tmp.empty())
                {
                    
                    dlist.push_back(tmp);
                }
            }
        }
    }
    dlist.sort();
    dlist.unique();
    return res;
}
bool ccproject::automake_scan_files(list<string> &flist, const string& dir)
{
    executed_regex sources("_SOURCES *=",REG_EXTENDED,0);
    if(!sources.verify_regex(cerr))
        return false;
    ifstream file;
    string line,tmp;
    string::size_type p1,p2;
    bool res=false;
    file.open("Makefile.am");
    while(file)
    {
        getline(file,line);
        if(file && sources.regexec(line.c_str())==0)
        {
            while(file && line[line.length()-1]=='\\')
            {
                line.erase(line.length()-1);
                getline(file,tmp);
                line += tmp;
            }
            res=true;
            p1=line.find('=');
            p1=line.find_first_not_of(" \t",p1+1);
            p2=0;
            while(p2 != string::npos && p1 != string::npos)
            {
                p2=line.find_first_of(" \t",p1);
                if(p2==string::npos)
                {
                    tmp=line.substr(p1);
                }
                else
                {
                    tmp=line.substr(p1,p2-p1);
                    p1=line.find_first_not_of(" \t",p2);
                }
                if(!tmp.empty() && tmp[0] != '$')
                {
                    flist.push_back(tmp);
                }
            }
        }
    }
    return res;
}

bool ccproject::automake_scan_dirs_files (list<string>& dlist, list<string>& flist, const string& dir)
{
    bool dres, fres;
    dres=automake_scan_dirs(dlist, dir);
    fres=automake_scan_files(flist, dir);
    return dres || fres;
}

bool ccproject::glob_scan_dirs_files(list<string>& dlist, list<string>& flist, const string& dir)
{
    DIR *dptr;
    bool res=false;
    struct dirent *eptr;
    dptr=opendir(dir.c_str());
    if(dptr != NULL)
    {
        res=true;
        while((eptr=readdir(dptr))!=NULL)
        {
			string fullname = dir;
			fullname += eptr->d_name;
			
			if (eptr->d_name[0] != '.')
			{
				if(file_is_directory(fullname.c_str()) && strcmp(eptr->d_name, "..") != 0)
				{
					dlist.push_back(eptr->d_name);
				}
				else if (file_is_regular(fullname.c_str()))
				{
					flist.push_back(eptr->d_name);
				}
			}
        }
        closedir(dptr);
    }
    return res;
}
        
bool ccproject::get_dir_file_list(list<string>& dlist, list<string>& flist, const string& dir)
{
    bool res;
    if(use_automake)
    {
        res=automake_scan_dirs_files (dlist, flist, dir);
    }
    else
    {
        res=glob_scan_dirs_files (dlist, flist, dir);
    }
    return res;
}

bool ccproject::file_update_required(file_context& context)
{
	if(!context.get_is_c_file())
		return false;
	return (ftimes[context.get_relpath_filename()] < context.get_mtime());
}

bool ccproject::do_update(tree_coll<string>& currdir, const string& path)
{
    bool res = true;
	list<string> dirlist, filelist;
    list<string>::iterator fptr;
	
	string current_dir = get_absolute_path(path);
    
	res=get_dir_file_list(dirlist, filelist, current_dir);
	if (!res) return false;

	// Scan directories
	if(recurse_dirs)
    {
		fptr=dirlist.begin();
		while(fptr != dirlist.end() )
		{
			string newpath = path;
			newpath += (*fptr);
			newpath += '/';
			res=do_update(currdir.add_branch(*fptr), newpath) || res;
			fptr++;
		}
    }
	// Scan files
	fptr=filelist.begin();
	while(fptr!=filelist.end())
	{
		file_context context;
		context.set_file (&currdir, current_dir, *fptr);
		if(file_update_required(context))
			scanner.add(context);
		check_completed_files(context);
		fptr++;
	}
    return true;
}

void ccproject::clear()
{
	files.clear();
	classes.clear();
}

bool ccproject::update()
{
    bool res;
	
	//Don't clear the classes here. only the files.
	files.clear();
    if(chdir(directory.c_str()) < 0)
    {
        cerr<<"error changing to directory "<<directory<<endl;
        return false;
    }
    if(!includeregex.verify_regex(cerr) ||
       !memberfunctregex.verify_regex(cerr)   )
    {
        return false;
    }
	string path = "";
    res=do_update(files, path);
	scanner.start();
    return res;
}

bool ccproject::check_completed_files(file_context& context)
{
	const char* fname;
	fname = context.get_filename().c_str();
	ftimes[context.get_relpath_filename()] = context.get_mtime();
    if(context.get_currdir()->isfound(fname))
        return true;
    context.get_currdir()->add_item(fname);
    return false;
}

bool ccproject::check_for_include(char *buff, file_context& context)
{
    bool res=false;
	if(includeregex.regexec(buff)==0)
    {
        string fname= *(includeregex.begin());
        if(fname.length() > 0)
        {
            if (follow_includes)
			{
				file_context ctx;
				ctx.set_file (context.get_currdir(), context.get_dir(), fname);
				if(!ctx.get_error())
				{
					scan_file(ctx);
				}
			}
           	res=true;
        }
    }
    return res;
}

string ccproject::get_absolute_path(const string& path) const
{
	return directory + path;
}

bool ccproject::scan_file(file_context& context)
{
    char *ptr;
	string fullname = context.get_full_filename();
	if(!context.get_is_c_file()) return true;
	context.start();
	while(!context.get_error())
	{
		ptr = context.fgets();
		process_line(ptr, context);
	}
    return true;
}

const char *ccproject::get_file_from_class(const char *cl)
{
    string name(classes[cl].getdeclfile());
    return name.c_str();
}
int ccproject::get_line_from_class(const char *cl)
{
    return classes[cl].getdeclline();
}

const char *
ccproject::get_file_from_function(const char *cl,const char *func)
{
    string name(classes[cl].get_file_from_function(func));
    return name.c_str();
}

int ccproject::get_line_from_function(const char *cl,const char *func) 
{
    return classes[cl].get_line_from_function(func);
}
const char *ccproject::get_decl_file_from_function(const char *cl,const char *func)
{
    string name(classes[cl].get_decl_file_from_function(func));
    return name.c_str();
}

int ccproject::get_decl_line_from_function(const char *cl,const char *func) 
{
    return classes[cl].get_decl_line_from_function(func);
}

bool ccproject::save_in_xml (ostream& str, int depth)
{
	bool ret = true;
	outtabs (str, depth);
	str << "<ccproject>\n";
	outtabs (str, depth+1);
	str << "<classes>\n";
	
	// Save classes
	class_pos cptr = get_first_class();
	class_pos cptr_last = get_end_class();
    while(cptr != cptr_last)
    {
        ret = get_class(cptr).save_in_xml(str, depth+2);
		if (!ret) break;
        cptr++;
    }
	outtabs (str, depth+1);
	str << "</classes>\n";
	outtabs (str, depth+1);
	str << "<files>\n";
	files_save_in_xml(str, files, "", depth+2);
	outtabs (str, depth+1);
	str << "</files>\n";
	outtabs (str, depth);
	str << "</ccproject>\n";
	return ret;
}

bool ccproject::load_from_xml (xmlNode* root)
{
	xmlNode *node, *nextnode;
	
	if (!root || !root->name)
		return false;
	if (strcmp ((const char*)root->name, "ccproject") != 0)
		return false;
	if (!root->childs)
		return true; //no file, no class is a valid condition

	nextnode = root->childs;
	if (!nextnode) return false;
	if (strcmp ((char*)nextnode->name, "classes") != 0) return false;
	node = nextnode->childs;
	while (node)
	{
		if (strcmp ((char*)node->name, "class") == 0)
		{
			class_pos::iterator ptr;
			ptr = get_class (xml_get_prop(node, "name"));
			(*ptr).second.load_from_xml(node);
		}
		node = node->next;
	}
	nextnode = nextnode->next;
	if (!nextnode) return false;
	if (strcmp ((char*)nextnode->name, "files") != 0) return false;
	string path = "";
	files_load_from_xml(files, nextnode->childs, path);
	return true;
}

bool ccproject::files_load_from_xml (noconst_dirref dirptr, xmlNode* childs, const string& path)
{
	xmlNode* node;
	node = childs;
	bool ret = true;
	
	while(node)
	{
		const gchar* name;
		name = xml_get_prop(node, "name");
		if (name)
		{
			string newpath = path;
			newpath += name;
			string filename = get_absolute_path(newpath);
			// We cannot read untill we are sure the 'name'.
			
			if(strcmp((const char*)node->name, "directory")==0) // directory
			{
				if (file_is_directory(filename.c_str()))
				{
					ftimes[newpath] = atol(xml_get_prop(node, "mtime"));
					newpath += "/";
					ret = files_load_from_xml (dirptr.add_branch(name), node->childs, newpath);
				}
			}
			else if(strcmp((const char*)node->name, "file")==0) // file
			{
				if(file_is_regular(filename.c_str()))
				{
					ftimes[newpath] = atol(xml_get_prop(node, "mtime"));
					dirptr.add_item(name);
				}
			}
		}
		node = node->next;
	}
	return ret;
}

bool ccproject::files_save_in_xml(ostream& str, dirref dptr, const string& path, int depth)
{
	bool ret = true;
    dir_pos ds,de;
    file_pos fs,fe;
    ds=get_first_dir(dptr);
    de=get_end_dir(dptr);
    while(ds != de)
    {
		const char* dirname = (*ds).get_data().c_str();
		outtabs(str, depth);
		str << "<directory name=\"" << xml_encode(dirname) << "\"";
		string newpath = path;
		newpath += dirname;
		str << " mtime=\"" << ftimes[newpath] << "\">\n";
		newpath +="/";
        ret = files_save_in_xml(str, *ds, newpath, depth+1);
		if (!ret) break;
		outtabs(str, depth);
		str << "</directory>\n";
        ds++;
    }
    fs=get_first_file(dptr);
    fe=get_end_file(dptr);
    while(fs != fe)
    {
		const char* filename = (*fs).c_str();
		outtabs(str, depth);
		str << "<file name=\"" << xml_encode(filename) << "\"";
		string newpath = path;
		newpath += filename;
		str << " mtime=\"" << ftimes[newpath] << "\"/>\n";
        fs++;
    }
	return ret;
}

Scanner::Scanner(ccproject* prj)
{
	owner = prj;
	end_callback = NULL;
	notify_data = NULL;
	scanning = false;
	cancel_scanning = false;
	max_files = count = 0;
	use_progressbar = true;
	create_dialog();
}

Scanner::~Scanner()
{
	if (GTK_IS_WINDOW((win)))
		gtk_widget_destroy (win);
}

void Scanner::clear()
{
	contexts.clear();
	scanning = false;
	cancel_scanning = false;
	max_files = count = 0;
}

void
Scanner::create_dialog (void)
{
  GtkWidget *dialog_vbox1;
  GtkWidget *table1;
  GtkWidget *pixmap1;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;

  win = gnome_dialog_new (NULL, NULL);
  gtk_object_set_data (GTK_OBJECT (win), "Scanning and Updating files. Please Wait ...", win);
  gtk_widget_set_usize (win, 380, 0);
  gtk_window_set_policy (GTK_WINDOW (win), TRUE, TRUE, FALSE);

  dialog_vbox1 = GNOME_DIALOG (win)->vbox;
  gtk_object_set_data (GTK_OBJECT (win), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  table1 = gtk_table_new (2, 2, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (win), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), table1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 6);

  label = gtk_label_new (_("Updating file:"));
  gtk_widget_ref (label);
  gtk_object_set_data_full (GTK_OBJECT (win), "label", label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table1), label, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  colormap = gtk_widget_get_colormap (win);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
                                                   NULL, scanner_xpm);
  pixmap1 = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);

  gtk_widget_ref (pixmap1);
  gtk_object_set_data_full (GTK_OBJECT (win), "pixmap1", pixmap1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap1);
  gtk_table_attach (GTK_TABLE (table1), pixmap1, 0, 1, 0, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  progressbar = gtk_progress_bar_new ();
  gtk_widget_ref (progressbar);
  gtk_object_set_data_full (GTK_OBJECT (win), "progressbar", progressbar,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (progressbar);
  gtk_table_attach (GTK_TABLE (table1), progressbar, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  dialog_action_area1 = GNOME_DIALOG (win)->action_area;
  gtk_object_set_data (GTK_OBJECT (win), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (win), GNOME_STOCK_BUTTON_CANCEL);
  button1 = GTK_WIDGET (g_list_last (GNOME_DIALOG (win)->buttons)->data);
  gtk_widget_ref (button1);
  gtk_object_set_data_full (GTK_OBJECT (win), "button1", button1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button1);
  gtk_signal_connect (GTK_OBJECT(button1), "clicked", GTK_SIGNAL_FUNC(cancel_clicked), this);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
}

void Scanner::start()
{
	if (scanning) return;
	cancel_scanning = false;
	scanning = true;
	cur_pos = contexts.begin();
	last_pos = contexts.end();
	if(cur_pos == last_pos) return;
	count = 0;
	gtk_label_set_text (GTK_LABEL(label), "Updating file: ");
	gtk_progress_bar_update (GTK_PROGRESS_BAR(progressbar), 0);
	gtk_widget_show(win);
	gtk_signal_emit (owner->object, ccview_signals[UPDATE_START], owner->getdirectory());
	gtk_idle_add (scan_context, this);
}

gint
Scanner::scan_context (gpointer data)
{
	Scanner* scn = (Scanner*)data;
	if ((scn->cur_pos == scn->last_pos) || scn->cancel_scanning)
	{
		scn->clear();
	    scn->owner->strip_undefined();
		if (scn->end_callback) (*(scn->end_callback))(scn->notify_data);
		gtk_widget_hide(scn->win);
		scn->scanning = false;
		return false;
	}
	scn->count++;
	gtk_signal_emit (scn->owner->object, ccview_signals[UPDATING_FILE],
			scn->cur_pos->get_filename().c_str());
	scn->owner->scan_file (*(scn->cur_pos));
	gtk_progress_bar_update (GTK_PROGRESS_BAR(scn->progressbar),
		(float)scn->count/scn->max_files);
	string s = "Updating file: ";
	s += scn->cur_pos->get_filename();
	gtk_label_set_text (GTK_LABEL(scn->label), s.c_str());
	scn->cur_pos++;
	return true;
}

void
Scanner::cancel_clicked(GtkObject* obj, gpointer data)
{
	Scanner* scn = (Scanner*)data;
	scn->cancel();
}
