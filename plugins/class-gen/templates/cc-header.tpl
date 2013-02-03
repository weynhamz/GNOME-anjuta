[+ autogen5 template +]
[+INCLUDE (string-append "indent.tpl") \+]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
/* [+INVOKE EMACS-MODELINE MODE="C" \+] */
[+INVOKE START-INDENT\+]
/*
 * [+HeaderFile+][+IF (=(get "Headings") "1")+]
 * Copyright (C) [+(shell "date +%Y")+] [+AuthorName+] <[+AuthorEmail+]>[+ENDIF+]
 *
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "ProjectName") OWNER=(get "AuthorName") \+]
 */

#ifndef _[+ (string-upcase(string->c-name!(get "HeaderFile"))) +]_
#define _[+ (string-upcase(string->c-name!(get "HeaderFile"))) +]_

class [+ClassName+][+IF (not (=(get "BaseClass") ""))+]: [+Inheritance+] [+BaseClass+] [+ENDIF+]
{
[+FOR var IN public protected private+][+
	var+]:
[+
	FOR Elements +][+
		IF (=(get "Scope") (get "var")) +]	[+
			CASE (get "Implementation")+][+
			== "normal"+][+
			== "static"+]static [+
			== "virtual"+]virtual [+ 
			ESAC+][+
			IF (not (=(get "Type") "")) +][+Type+] [+ENDIF+][+
			Name+][+
			Arguments+][+
			IF (and(=(get "Inline") "1") (not(=(get "Arguments") "")))+] { /* TODO: Add implementation here */ }[+
			ELSE+];[+
			ENDIF+]
[+
		ENDIF+][+
	ENDFOR+]
[+
ENDFOR+]};

#endif // _[+ (string-upcase(string->c-name!(get "HeaderFile"))) +]_
[+INVOKE END-INDENT\+]
