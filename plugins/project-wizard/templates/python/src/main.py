[+ autogen5 template +]
#!/usr/bin/python
#
# main.py
# Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
# 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "Name") (get "Author") "# ")+]
[+ == "LGPL" +][+(lgpl (get "Name") (get "Author") "# ")+]
[+ == "GPL"  +][+(gpl  (get "Name")                "# ")+]
[+ESAC+]

print "Hello World!"
