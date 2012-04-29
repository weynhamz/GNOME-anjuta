[+ autogen5 template +]
[+INCLUDE (string-append "indent.tpl") \+]
/* [+INVOKE EMACS-MODELINE MODE="C" \+] */
[+INVOKE START-INDENT\+]
/*
 * [+ProjectName+][+IF (=(get "Headings") "1")+]
 * Copyright (C) [+(shell "date +%Y")+] [+AuthorName+] <[+AuthorEmail+]>[+ENDIF+]
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "ProjectName") (get "AuthorName") " * ")+]
[+ == "LGPL" +][+(lgpl (get "ProjectName") (get "AuthorName") " * ")+]
[+ == "GPL"  +][+(gpl  (get "ProjectName")                    " * ")+]
[+ESAC+] */

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
