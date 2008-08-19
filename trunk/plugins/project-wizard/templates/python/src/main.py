[+ autogen5 template +]
#!/usr/bin/python
#
# main.py
# Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
# 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  "main.py" (get "Author") "# ")+]
[+ == "LGPL" +][+(lgpl "main.py" (get "Author") "# ")+]
[+ == "GPL"  +][+(gpl  "main.py"                "# ")+]
[+ESAC+]

print "Hello World!"
