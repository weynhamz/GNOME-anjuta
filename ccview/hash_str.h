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

#ifndef __HASH_STR__H__
#define __HASH_STR__H__
#include <hash_map.h>
struct hash_str
{
    size_t operator()(const string& str) const
    {
        hash<const char *> hasher;
        return hasher(str.c_str());
    }
};

#endif
