[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
[+INCLUDE (string-append "indent.tpl") \+]
# [+INVOKE EMACS-MODELINE MODE="Python" \+]
[+INVOKE START-INDENT\+]
#!/usr/bin/python
#
# main.py
# Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
# 
[+INVOKE LICENSE-DESCRIPTION PFX="# " PROGRAM=(get "Name") OWNER=(get "Author") \+]

print "Hello World!"
[+INVOKE END-INDENT\+]
