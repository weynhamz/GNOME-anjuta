[+ autogen5 template +]
[+INCLUDE (string-append "indent.tpl") \+]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
/* [+INVOKE EMACS-MODELINE MODE="C" \+] */
[+INVOKE START-INDENT\+]
/*
 * [+SourceFile+][+IF (=(get "Headings") "1")+]
 * Copyright (C) [+(shell "date +%Y")+] [+AuthorName+] <[+AuthorEmail+]>[+ENDIF+]
 *
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "ProjectName") OWNER=(get "AuthorName") \+]
 */

#include "[+HeaderFile+]"

[+IF (=(get "Inline") "0")+][+
	FOR var IN public protected private+][+
		FOR Elements +][+
			IF (=(get "Scope") (get "var")) +][+
				IF (not (=(get "Arguments") ""))+][+
					IF (not (=(get "Type") "")) +][+Type+] [+ENDIF+][+
					ClassName+]::[+Name+][+
					Arguments+]
{
	// TODO: Add implementation here
}

[+
				ENDIF+][+
			ENDIF+][+
		ENDFOR+][+
	ENDFOR+][+
ENDIF+]
[+INVOKE END-INDENT\+]
