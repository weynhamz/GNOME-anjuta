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

#include <gnome.h>
#include <string>

using namespace std;

class func_dlg
{
    string func;
    GtkWidget *bstatic;
    GtkWidget *bconst;
    GtkWidget *bpublic;
    GtkWidget *bprotected;
    GtkWidget *bprivate;
    bool qstatic;
    bool qconst;
    unsigned int ftype;
    GtkWidget *win;
    GtkWidget *fentry;
public:
    enum func_type
    {
        fpublic,
        fprotected,
        fprivate
    };
    func_dlg();
    bool gomodal();
    int gettype() const {return ftype;} 
    const char *get_func() const {return func.c_str();}
    bool isstatic()const {return qstatic;}
    bool isconst() const {return qconst;}
    string getfuncdecl(const char *class_name) const;
    string getfuncdef(const char *class_name) const;
};

