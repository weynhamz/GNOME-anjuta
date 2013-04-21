[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
[+INCLUDE (string-append "indent.tpl") \+]
#!/usr/bin/python
# [+INVOKE EMACS-MODELINE MODE="Python; coding: utf-8" +]
[+INVOKE START-INDENT\+]
#
# main.py
# Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
# 
[+INVOKE LICENSE-DESCRIPTION PFX="# " PROGRAM=(get "Name") OWNER=(get "Author") \+]

print "Hello World!"
[+INVOKE END-INDENT\+]
