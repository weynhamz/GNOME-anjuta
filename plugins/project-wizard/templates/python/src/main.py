[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
#!/usr/bin/python
#
# main.py
# Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
# 
[+INVOKE LICENSE-DESCRIPTION PFX="# " PROGRAM=(get "Name") OWNER=(get "Author") \+]

print "Hello World!"
