/*
 * properties_cxx.h Copyright (C) 2004 Johannes Schmid
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _PROPERTIES_CXX_H_
#define _PROPERTIES_CXX_H_


#include <glib.h>

#include "PropSet.h"

class PropSetFile : public PropSet {
public:
	PropSetFile() {};
	~PropSetFile() {};
	bool ReadLine(char *data, bool ifIsTrue, const char *directoryForImports=0);
	void ReadFromMemory(const char *data, int len, const char *directoryForImports=0);
	void Read(const char *filename, const char *directoryForImports);
};

#endif
