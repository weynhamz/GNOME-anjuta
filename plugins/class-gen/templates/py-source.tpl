[+ AutoGen5 template
##
##  py-source.tpl
##  Copyright (C) 2010 Kenny Meyer <knny.myer@gmail.com>
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
##  GNU Library General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
+]
# -*- Mode: Python; Coding: utf-8; tab-width: 4 -*-
#
# [+ProjectName+][+IF (=(get "Headings") "1")+]
# Copyright (C) [+AuthorName+] [+(shell "date +%Y")+] <[+AuthorEmail+]>[+ENDIF+]
#
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "ProjectName") (get "AuthorName") "# ")+]
[+ == "LGPL" +][+(lgpl (get "ProjectName") (get "AuthorName") "# ")+]
[+ == "GPL"  +][+(gpl  (get "ProjectName")                    "# ")+]
[+ESAC+]

class [+ClassName+]([+IF (not (=(get "BaseClass") ""))+][+BaseClass+][+ENDIF+]):[+
FOR Constvars +][+
    IF (not (=(get "Name") "")) +]
    [+Name+] = [+Value+][+
    ENDIF +][+
ENDFOR +][+
FOR Methods +][+
    IF (not (=(get "Name") "")) +]
    def [+Name+][+Arguments+]:
        pass
[+
    ENDIF +][+
ENDFOR +]
