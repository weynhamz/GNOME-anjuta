/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta 
 * Copyright (C) Massimo Cora' 2009 <maxcvs@email.it> 
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCOPE_PARSER_H_
#define _SCOPE_PARSER_H_

#include <string>
#include <vector>
#include <map>

using namespace std;


std::string get_scope_name (const std::string &in,
							std::vector<std::string> &additionalNS,
							const std::map <std::string, std::string> &ignoreTokens);

#endif
