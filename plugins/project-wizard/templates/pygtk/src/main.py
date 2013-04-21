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

from gi.repository import Gtk, GdkPixbuf, Gdk
import os, sys

[+IF (=(get "HaveBuilderUI") "1")+]
#Comment the first line and uncomment the second before installing
#or making the tarball (alternatively, use project variables)
UI_FILE = "src/[+NameHLower+].ui"
#UI_FILE = "/usr/local/share/[+NameHLower+]/ui/[+NameHLower+].ui"
[+ENDIF+]

class GUI:
	def __init__(self):
[+IF (=(get "HaveBuilderUI") "1")+]
		self.builder = Gtk.Builder()
		self.builder.add_from_file(UI_FILE)
		self.builder.connect_signals(self)

		window = self.builder.get_object('window')
[+ELSE+]
		window = Gtk.Window()
		window.set_title ("Hello World")
		window.connect_after('destroy', self.destroy)
[+ENDIF+]

		window.show_all()

	def destroy(window, self):
		Gtk.main_quit()

def main():
	app = GUI()
	Gtk.main()
		
if __name__ == "__main__":
	sys.exit(main())
[+INVOKE END-INDENT\+]
