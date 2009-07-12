[+ autogen5 template +]
/*
 * plugin.vala
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 *
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "Name") (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl (get "Name") (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  (get "Name")                " * ")+]
[+ESAC+] */

using GLib;
using Anjuta;

public class [+PluginClass+] : Plugin {
[+IF (=(get "HasUI") "1") +]
	static string XML_FILE;
[+ENDIF+][+IF (=(get "HasGladeFile") "1") +]
	static string UI_FILE;
[+ENDIF+][+IF (or (=(get "HasUI") "1") (=(get "HasGladeFile") "1") ) +]
	static construct {
		// workaround for bug 538166, should be const
[+IF (=(get "HasUI") "1") +]		XML_FILE = Config.ANJUTA_DATA_DIR + "/ui/[+NameHLower+].xml";
[+ ENDIF +][+IF (=(get "HasGladeFile") "1") +]		UI_FILE = Config.ANJUTA_DATA_DIR + "/glade/[+NameHLower+].ui";
[+ ENDIF +]
	}
[+ ENDIF +]
[+IF (=(get "HasUI") "1") +]
	private int uiid = 0;
	private Gtk.ActionGroup action_group;
[+ENDIF+][+IF (=(get "HasGladeFile") "1") +]
	private Gtk.Widget widget = null;
[+ENDIF+]
[+IF (=(get "HasUI") "1") +]
	const Gtk.ActionEntry[] actions_file = {
		{
			"ActionFileSample",          /* Action name */
			Gtk.STOCK_NEW,               /* Stock icon, if any */
			N_("_Sample action"),        /* Display label */
			null,                        /* short-cut */
			N_("Sample action"),         /* Tooltip */
			on_sample_action_activate    /* action callback */
		}
	};

	public void on_sample_action_activate (Gtk.Action action) {

		/* Query for object implementing IAnjutaDocumentManager interface */
		var docman = (IAnjuta.DocumentManager) shell.get_object ("IAnjutaDocumentManager");
		var editor = (IAnjuta.Editor) docman.get_current_document ();

		/* Do whatever with plugin */

	}
[+ENDIF+]
	public override bool activate () {

		//DEBUG_PRINT ("%s", "[+PluginClass+]: Activating [+PluginClass+] plugin ...");
[+IF (=(get "HasUI") "1") +]
		/* Add all UI actions and merge UI */
		var ui = shell.get_ui ();
		action_group = ui.add_action_group_entries ("ActionGroupFile[+NameHLower+]",
													_("Sample file operations"),
													actions_file,
													Config.GETTEXT_PACKAGE, true,
													this);
		uiid = ui.merge (XML_FILE);
[+ENDIF+][+IF (=(get "HasGladeFile") "1") +]
		/* Add plugin widgets to Shell */
		try {
			var builder = new Builder ();
			builder.add_from_file (UI_FILE);

			widget = builder.get_object ("top_widget") as widget;

			shell.add_widget (widget, "[+PluginClass+]Widget",
						  _("[+PluginClass+] widget"), null,
						  ShellPlacement.BOTTOM);
		} catch (Error e) {
			stderr.printf ("Could not load UI: %s\n", e.message);
			return false;
		}

[+ENDIF+]
		return true;
	}

	public override bool deactivate () {
		//DEBUG_PRINT ("%s", "[+PluginClass+]: Dectivating [+PluginClass+] plugin ...");
[+IF (=(get "HasGladeFile") "1") +]
		shell.remove_widget (widget);
[+ENDIF+][+IF (=(get "HasUI") "1") +]
		var ui = shell.get_ui ();
		ui.remove_action_group (action_group);
		ui.unmerge (uiid);
[+ENDIF+]	
		return true;
	}
}

[ModuleInit]
public GLib.Type anjuta_glue_register_components (GLib.TypeModule module) {
    return typeof ([+PluginClass+]);
}
