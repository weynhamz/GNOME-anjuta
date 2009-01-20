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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 */

#ifndef _PROPERTIES_CXX_H_
#define _PROPERTIES_CXX_H_


#include <glib.h>

#include "PropSet.h"

class PropSetFile : public PropSet {
	bool lowerKeys;
	SString GetWildUsingStart(const PropSet &psStart, const char *keybase, const char *filename);
protected:
	static bool caseSensitiveFilenames;
public:
	PropSetFile(bool lowerKeys_=false);
	~PropSetFile();
	bool ReadLine(const char *data, bool ifIsTrue, const char *directoryForImports=0);
	void ReadFromMemory(const char *data, int len, const char *directoryForImports=0);
	bool Read(const char *filename, const char *directoryForImports);
	SString GetWild(const char *keybase, const char *filename);
	SString GetNewExpand(const char *keybase, const char *filename="");
	bool GetFirst(char **key, char **val);
	bool GetNext(char **key, char **val);
	static void SetCaseSensitiveFilenames(bool caseSensitiveFilenames_) {
		caseSensitiveFilenames = caseSensitiveFilenames_;
	}
};

#endif
