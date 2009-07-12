[+ autogen5 template +]
/*
 * plugin.h
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "Name") (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl (get "Name") (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  (get "Name")                " * ")+]
[+ESAC+] */

[CCode (cprefix = "", lower_case_cprefix = "", cheader_filename = "config.h")]
namespace Config {
	public const string GETTEXT_PACKAGE;
	public const string PACKAGE_LOCALE_DIR;
	public const string ANJUTA_DATA_DIR;
	public const string ANJUTA_PLUGIN_DIR;
	public const string ANJUTA_IMAGE_DIR;
	public const string ANJUTA_GLADE_DIR;
	public const string ANJUTA_UI_DIR;
	public const string PACKAGE_DATA_DIR;
	public const string PACKAGE_SRC_DIR;
}

