[+ autogen5 template +]
[+DEFINE START-INDENT \+]
[+(out-push-new)\+]
[+ENDDEF \+]
[+DEFINE END-INDENT \+]
[+(for-each
	indent
	(string-split (out-pop #t) #\nl);
)\+]
[+ENDDEF \+]
[+(define
	indentation
	(lambda
		(tab)
		(let*
			(
				(use-tabs (not (zero? (or (string->number (get "UseTabs")) 8))))
				(tab-width (or (string->number (get "TabWidth")) 8))
				(indent-width (or (string->number (get "IndentWidth")) 1))
				(spaces (* tab indent-width))
				(tab-indent (if use-tabs (quotient spaces tab-width) 0))
				(space-indent (- spaces (* tab-indent tab-width)))
			)
			(string-append
				(make-string tab-indent #\ht)
				(make-string space-indent #\sp)
			)
		)
	)
)
(define
	indent
	(lambda
		(line)
		(let
			((tab (or (string-skip line #\ht) 0)))
			(emit 
				(string-append
					(indentation tab)
					(string-drop line tab)
					"\n"
				)
			)
		)	
	)
) \+]
[+DEFINE EMACS-MODELINE \+]
-*- Mode: [+MODE+]; indent-tabs-mode: [+(if (== (get "UseTabs") "1") "t" "nil")+]; c-basic-offset: [+IndentWidth+]; tab-width: [+TabWidth+] -*- [+ENDDEF \+]
