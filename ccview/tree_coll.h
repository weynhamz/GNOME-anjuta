/*  CcviewProject widget
 *
 *  Original code by Ron Jones <ronjones@xnet.com>
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

#ifndef __TREE_COLL_H
#define __TREE_COLL_H
#include <list>
#include <string>
#include <algo.h>
// this is really designed for cacheing directory contents.
template < class T >
class tree_coll
{
    T data;
    tree_coll<T> *parent;
    list <tree_coll<T> > branches;
    list <T> items;
public:
    typedef list<T>::const_iterator item_iterator;
    typedef list<tree_coll<T> >::const_iterator branch_iterator;
    tree_coll(){parent=NULL;}
    tree_coll(const tree_coll& in){
        *this=in;
    }
    tree_coll(const T& in){data=in;parent=NULL;}
    tree_coll & operator=(const tree_coll& in){
        data=in.data;
        branches=in.branches;
        items=in.items;
        parent=in.parent;
        return *this;
    }
    void setparent( tree_coll<T> & in){
        parent=&in;
    }
    tree_coll<T> * getparent(){
        return parent;
    }
    void clear()
    {
        list<tree_coll>::iterator ptr=branches.begin();
        while(ptr!= branches.end())
        {
            (*ptr).clear();
            ptr++;
        }
        branches.clear();
        items.clear();
    }
    item_iterator item_begin() const {return items.begin();}
    item_iterator item_end() const {return items.end();}
    branch_iterator branch_begin() const {return branches.begin();}
    branch_iterator branch_end() const {return branches.end();}
    void set_data(const T& in){data=in;}
    const T& get_data() const {return data;}
    tree_coll& add_branch(const T& in){
        branches.push_back(tree_coll(in));
        branches.back().setparent(*this);
        return branches.back();
    }
    void add_item(const T& in){
        items.push_back(in);
    }
    bool  isfound(const T& in){
        bool res=false;
        if(find(items.begin(),items.end(),in)!=items.end())
        {
            res=true;
        }
        return res;
    }
    bool isfoundbranchdata(const T& in) const {
        bool res=false;
        branch_iterator ptr=branches.begin();
        while(ptr != branches.end())
        {
            if((*ptr).data==in)
            {
                res=true;
                break;
            }
            ptr++;
        }
        return res;
    }
    branch_iterator findbranchdata(const T& in) const {
        branch_iterator ptr=branches.begin();
        while(ptr != branches.end())
        {
            if((*ptr).data==in)
            {
                break;
            }
            ptr++;
        }
        return ptr;
    }
};

#endif // __TREE_COLL_H
