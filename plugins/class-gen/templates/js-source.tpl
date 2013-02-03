[+ AutoGen5 template
##
##  js-source.tpl
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
[+INCLUDE (string-append "indent.tpl") \+]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
[+INVOKE START-INDENT\+]
/* [+INVOKE EMACS-MODELINE MODE="javascript" \+] */
/*
 * [+ProjectName+][+IF (=(get "Headings") "1")+]
 * Copyright (C) [+(shell "date +%Y")+] [+AuthorName+] <[+AuthorEmail+]>[+ENDIF+]
 *
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "ProjectName") OWNER=(get "AuthorName") \+]
 */
[+
FOR Imports +][+
	IF (not (=(get "Name") "")) +]
const [+Name+] = imports.[+Module+];[+
	IF (last-for?) +]
[+
		ENDIF+][+
	ENDIF+][+
ENDFOR+][+
IF (not (=(get "BaseClass") "")) +]
function [+BaseClass+]([+Initargs+]){
	this._init([+Initargs+]);
}

[+BaseClass+].prototype = {
	_init: function([+Initargs+]) {
		// The Base class.
	}
};[+
ENDIF+]

function [+ClassName+]() {
	this._init([+Initargs+]);
}

[+ClassName+].prototype = {[+
IF (not (=(get "BaseClass") "")) +]
	__proto__ : [+BaseClass+].prototype,
[+
ENDIF+]
	_init: function([+Initargs+]) {[+
FOR Variables +][+
	IF (not (=(get "Name") "")) +]
		this.[+Name+] = [+Value+];[+
	ENDIF+][+
ENDFOR+]
	},
[+
FOR Methods +][+
	IF (not (=(get "Name") "")) +]
	[+Name+] : function[+Arguments+] {
		// TODO: Delete this line and add something useful[+
		IF (last-for?) +]
	}[+
		ELSE+]
	},
[+
		ENDIF+][+
	ENDIF+][+
ENDFOR+]
};
[+INVOKE END-INDENT\+]
