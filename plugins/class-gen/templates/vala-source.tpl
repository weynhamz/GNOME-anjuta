[+ AutoGen5 template
##
##  vala-source.tpl
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
/* [+INVOKE EMACS-MODELINE MODE="vala" \+] */
/*
 * [+SourceFile+][+IF (=(get "Headings") "1")+]
 * Copyright (C) [+(shell "date +%Y")+] [+AuthorName+] <[+AuthorEmail+]>[+ENDIF+]
 *
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "ProjectName") OWNER=(get "AuthorName") \+]
 */

[+ClassScope+] class [+ClassName+] : [+BaseClass+] {[+
FOR Properties+][+
	IF (=(get "Automatic") "Yes") +]
	[+Scope+] [+Type+] [+Name+] { get; set; }[+
	ELSE+]
	private [+Type+] _[+Name+] = [+Value+];[+
	ENDIF+][+
	IF (last-for?) +]
[+
	ENDIF+][+
ENDFOR+][+
FOR Properties+][+
	IF (=(get "Automatic") "No") +]
	[+Scope+] [+Type+] [+Name+] {[+
		IF (=(get "Getter") "YES")+]
		get { return _[+Name+] }[+
		ENDIF+][+
		IF (=(get "Setter") "YES")+]
		set { _[+Name+] = value }[+
		ENDIF+]
	}[+
	ENDIF+][+
ENDFOR+][+
FOR Signals+][+
	IF (first-for?) +]
	/* Signal definitions */[+ENDIF+]
	[+Scope+] signal void [+Name+] [+Arguments+];[+
ENDFOR+]

	// Constructor
		public [+ClassName+] () {

	}
[+
FOR Methods+][+
	IF (first-for?) +]
	/* Method definitions */[+ENDIF+]
	[+Scope+] [+Type+] [+Name+] [+Arguments+] {
		// TODO: Add implementation here.
	}[+
ENDFOR+]
}
[+INVOKE END-INDENT\+]
