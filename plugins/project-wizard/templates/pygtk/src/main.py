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

import sys
try:
    import gtk
except ImportError:
    sys.exit("pygtk not found.")

#Comment the first line and uncomment the second before installing
#or making the tarball (alternatively, use project variables)
UI_FILE = "data/[+NameHLower+].ui"
#UI_FILE = "/usr/local/share/[+NameHLower+]/ui/[+NameHLower+].ui"


class GUI:
    def __init__(self):
        self.builder = gtk.Builder()
        self.builder.add_from_file(UI_FILE)
        self.window = self.builder.get_object("window1")
        self.label = self.builder.get_object("label1")
        self.builder.connect_signals(self)
        
    def change_text(self, widget, *event):
        self.label.set_text("Hello, pygtk world!")

    def quit(self, widget, *event):
        gtk.main_quit()

def main():
    app = GUI()
    app.window.show()
    gtk.main()

if __name__ == "__main__":
    sys.exit(main())
