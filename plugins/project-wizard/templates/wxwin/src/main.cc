[+ autogen5 template +]
/*
 * main.cc
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 *
[+CASE (get "License") +]
[+ == "BSD" +][+(bsd "main.cc" (get "Author") "\t")+]
[+ == "LGPL" +][+(lgpl "main.cc" (get "Author") "\t")+]
[+ == "GPL" +][+(gpl "main.cc"  "\t")+]
[+ESAC+] */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <wx/wx.h>

class MyApp : public wxApp
{
  public:
    virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
  wxFrame *frame = new wxFrame((wxFrame *)NULL, -1, "Hello World",
                               wxPoint(50, 50), wxSize(450, 340));

  frame->Show(TRUE);
  return TRUE;
}
