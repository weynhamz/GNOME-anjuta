/*
 * Copyright (C) 2008-2010 Abderrahim Kitouni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

using GLib;
using Anjuta;

public class ValaPlugin : Plugin {
	internal weak IAnjuta.Editor current_editor;
	internal GLib.Settings settings = new GLib.Settings ("org.gnome.anjuta.cpp");
	uint editor_watch_id;
	ulong project_loaded_id;

	Vala.CodeContext context;
	Cancellable cancel;
	BlockLocator locator = new BlockLocator ();

	AnjutaReport report;
	ValaProvider provider;

	Vala.Parser parser;
	Vala.Genie.Parser genie_parser;

	Vala.Set<string> current_sources = new Vala.HashSet<string> (str_hash, str_equal);
	ValaPlugin () {
		Object ();
	}
	public override bool activate () {
		debug("Activating ValaPlugin");
		report = new AnjutaReport();
		report.docman = (IAnjuta.DocumentManager) shell.get_object("IAnjutaDocumentManager");
		parser = new Vala.Parser ();
		genie_parser = new Vala.Genie.Parser ();

		init_context ();

		provider = new ValaProvider(this);
		editor_watch_id = add_watch("document_manager_current_document",
		                            editor_value_added,
		                            editor_value_removed);

		return true;
	}

	public override bool deactivate () {
		debug("Deactivating ValaPlugin");
		remove_watch(editor_watch_id, true);

		cancel.cancel ();
		lock (context) {
			context = null;
		}

		return true;
	}

	void init_context () {
		context = new Vala.CodeContext();
		context.profile = Vala.Profile.GOBJECT;
		context.report = report;
		report.clear_error_indicators ();

		cancel = new Cancellable ();

		/* This doesn't actually parse anything as there are no files yet,
		   it's just to set the context in the parsers */
		parser.parse (context);
		genie_parser.parse (context);

		current_sources = new Vala.HashSet<string> (str_hash, str_equal);

	}

	void parse () {
		try {
			Thread.create<void>(() => {
				lock (context) {
					Vala.CodeContext.push(context);
					var report = context.report as AnjutaReport;

					foreach (var src in context.get_source_files ()) {
						if (src.get_nodes ().size == 0) {
							debug ("parsing file %s", src.filename);
							genie_parser.visit_source_file (src);
							parser.visit_source_file (src);
						}

						if (cancel.is_cancelled ()) {
							Vala.CodeContext.pop();
							return;
						}
					}

					if (report.errors_found () || cancel.is_cancelled ()) {
						Vala.CodeContext.pop();
						return;
					}

					context.check ();
					Vala.CodeContext.pop();
				}
			}, false);
		} catch (ThreadError err) {
			warning ("cannot create thread : %s", err.message);
		}
	}

	void add_project_files () {
		var project = (IAnjuta.ProjectManager) shell.get_object("IAnjutaProjectManager");
		Vala.CodeContext.push (context);

		var sources = project.get_elements(Anjuta.ProjectNodeType.SOURCE);
		foreach (var src in sources) {
			var path = src.get_path();
			if (path == null)
				continue;
			if (path.has_suffix (".vala") || path.has_suffix (".vapi") || path.has_suffix (".gs")) {
				if (path in current_sources) {
					debug ("file %s already added", path);
				} else {
					context.add_source_filename (path);
					current_sources.add (path);
					debug ("file %s added", path);
				}
			} else {
				debug ("file %s skipped", path);
			}
		}
		if (!context.has_package ("gobject-2.0")) {
			context.add_external_package("glib-2.0");
			context.add_external_package("gobject-2.0");
			debug ("standard packages added");
		} else {
			debug ("standard packages already added");
		}

		var packages = project.get_packages();
		foreach (var pkg in packages) {
			if (context.has_package (pkg)) {
				debug ("package %s skipped", pkg);
			} else if (context.add_external_package(pkg)) {
				debug ("package %s added", pkg);
			} else {
				/* TODO: try to look at VALAFLAGS */
				debug ("package %s not found", pkg);
			}
		}
		Vala.CodeContext.pop();
	}

	public void on_project_loaded (IAnjuta.ProjectManager pm, Error? e) {
		if (context == null)
			return;
		add_project_files ();
		parse ();
		pm.disconnect (project_loaded_id);
		project_loaded_id = 0;
	}

	/* "document_manager_current_document" watch */
	public void editor_value_added (Anjuta.Plugin plugin, string name, Value value) {
		debug("editor value added");
		assert (current_editor == null);
		if (!(value.get_object() is IAnjuta.Editor)) {
			/* a glade document, for example, isn't an editor */
			return;
		}

		current_editor = value.get_object() as IAnjuta.Editor;
		var current_file = value.get_object() as IAnjuta.File;

		var pm = (IAnjuta.ProjectManager) shell.get_object("IAnjutaProjectManager");
		var project = pm.get_current_project ();

		if (!project.is_loaded()) {
			if (project_loaded_id == 0)
				project_loaded_id = pm.project_loaded.connect (on_project_loaded);
		} else {
			var cur_gfile = current_file.get_file ();
			if (cur_gfile == null) {
				// File hasn't been saved yet
				return;
			}

			if (!(cur_gfile.get_path () in current_sources)) {
				cancel.cancel ();
				lock (context) {
					init_context ();
					add_project_files ();
				}

				parse ();
			}
		}
		if (current_editor != null) {
			if (current_editor is IAnjuta.EditorAssist)
				(current_editor as IAnjuta.EditorAssist).add(provider);
			if (current_editor is IAnjuta.EditorTip)
				current_editor.char_added.connect (on_char_added);
			if (current_editor is IAnjuta.FileSavable) {
				var file_savable = (IAnjuta.FileSavable) current_editor;
				file_savable.saved.connect (on_file_saved);
			}
			if (current_editor is IAnjuta.EditorGladeSignal) {
				var gladesig = current_editor as IAnjuta.EditorGladeSignal;
				gladesig.drop_possible.connect (on_drop_possible);
				gladesig.drop.connect (on_drop);
			}
		}
		report.update_errors (current_editor);
	}
	public void editor_value_removed (Anjuta.Plugin plugin, string name) {
		debug("editor value removed");
		if (current_editor is IAnjuta.EditorAssist)
			(current_editor as IAnjuta.EditorAssist).remove(provider);
		if (current_editor is IAnjuta.EditorTip)
			current_editor.char_added.disconnect (on_char_added);
		if (current_editor is IAnjuta.FileSavable) {
			var file_savable = (IAnjuta.FileSavable) current_editor;
			file_savable.saved.disconnect (on_file_saved);
		}
		if (current_editor is IAnjuta.EditorGladeSignal) {
			var gladesig = current_editor as IAnjuta.EditorGladeSignal;
			gladesig.drop_possible.disconnect (on_drop_possible);
			gladesig.drop.disconnect (on_drop);
		}
		current_editor = null;
	}

	public void on_file_saved (IAnjuta.FileSavable savable, File file) {
		foreach (var source_file in context.get_source_files ()) {
			if (source_file.filename != file.get_path())
				continue;

			string contents;
			try {
				file.load_contents (null, out contents, null, null);
				source_file.content = contents;
				update_file (source_file);
			} catch (Error e) {
				// ignore
			}
			return;
		}
	}

	public void on_char_added (IAnjuta.Editor editor, IAnjuta.Iterable position, char ch) {
		if (!settings.get_boolean (ValaProvider.PREF_CALLTIP_ENABLE))
			return;

		var editortip = editor as IAnjuta.EditorTip;
		if (ch == '(') {
			provider.show_call_tip (editortip);
		} else if (ch == ')') {
			editortip.cancel ();
		}
	}

	/* tries to find the opening brace of the scope the current position before calling
	 * get_current_context since the source_reference of a class or namespace only
	 * contain the declaration not the entire "content" */
	Vala.Symbol? get_scope (IAnjuta.Editor editor, IAnjuta.Iterable position) {
		var braces = new List<string> ();

		do {
			var current_char = (position as IAnjuta.EditorCell).get_character ();
			if (current_char == "}") {
				braces.prepend (current_char);
			} else if (current_char == "{") {
				if (braces != null && braces.data == "}") {
					braces.delete_link (braces);
				} else {
					// a scope which contains the current position
					do {
						position.previous ();
						current_char = (position as IAnjuta.EditorCell).get_character ();
					} while (! current_char.get_char ().isalnum ());

					return get_current_context (editor, position);
				}
			}
		} while (position.previous ());

		return null;
	}

	public bool on_drop_possible (IAnjuta.EditorGladeSignal editor, IAnjuta.Iterable position) {
		var scope = get_current_context (editor, position);
		if (scope is Vala.Block) {
			return false;
		} else if (scope == null) {
			scope = get_scope (editor, position.clone ());
			if (scope == null || scope is Vala.Namespace || scope is Vala.Class)
				return true;

			return false;
		}

		return false;
	}

	public void on_drop (IAnjuta.EditorGladeSignal editor, IAnjuta.Iterable position, string signal_data) {
		var data = signal_data.split (":");
		var widget_name = data[0];
		var signal_name = data[1].replace ("-", "_");
		var handler_name = data[2];
		var swapped = (data[4] == "1");
		var scope = get_scope (editor, position.clone ());
		var builder = new StringBuilder ();

		var scope_prefix = "";
		if (scope != null) {
			scope_prefix = scope.get_lower_case_cprefix ();
			if (handler_name.has_prefix (scope_prefix))
				handler_name = handler_name.substring (scope_prefix.length);
		}
		var handler_cname = scope_prefix + handler_name;

		if (data[2] != handler_cname && !swapped) {
			builder.append_printf ("[CCode (cname=\"%s\", instance_pos=-1)]\n", data[2]);
		} else if (data[2] != handler_cname) {
			builder.append_printf ("[CCode (cname=\"%s\")]\n", data[2]);
		} else if (!swapped) {
			builder.append ("[CCode (instance_pos=-1)]\n");
		}

		var widget = lookup_symbol_by_cname (widget_name);
		var sigs = symbol_lookup_inherited (widget, signal_name, false);
		if (sigs == null || !(sigs.data is Vala.Signal))
			return;
		Vala.Signal sig = (Vala.Signal) sigs.data;

		builder.append_printf ("void %s (", handler_name);

		if (swapped) {
			builder.append_printf ("%s sender", widget.get_full_name ());

			foreach (var param in sig.get_parameters ()) {
				builder.append_printf (", %s %s", param.variable_type.data_type.get_full_name (), param.name);
			}
		} else {
			foreach (var param in sig.get_parameters ()) {
				builder.append_printf ("%s %s, ", param.variable_type.data_type.get_full_name (), param.name);
			}

			builder.append_printf ("%s sender", widget.get_full_name ());
		}

		builder.append_printf (") {\n\n}\n");

		editor.insert (position, builder.str, -1);
	}

	Vala.Symbol? lookup_symbol_by_cname (string cname, Vala.Symbol parent=context.root) {
		var sym = parent.scope.lookup (cname);
		if (sym != null)
			return sym;

		var symtab = parent.scope.get_symbol_table ();
		foreach (var name in symtab.get_keys ()) {
			if (cname.has_prefix (name)) {
				return lookup_symbol_by_cname (cname.substring (name.length), parent.scope.lookup (name));
			}
		}
		return null;
	}

	internal Vala.Symbol get_current_context (IAnjuta.Editor editor, IAnjuta.Iterable? position=null) requires (editor is IAnjuta.File) {
		var file = editor as IAnjuta.File;

		var path = file.get_file().get_path();
		lock (context) {
			Vala.SourceFile source = null;
			foreach (var src in context.get_source_files()) {
				if (src.filename == path) {
					source = src;
					break;
				}
			}
			if (source == null) {
				source = new Vala.SourceFile (context,
				                              path.has_suffix("vapi") ? Vala.SourceFileType.PACKAGE:
					                                                    Vala.SourceFileType.SOURCE,
				                              path);
				context.add_source_file(source);
				update_file(source);
			}
			int line; int column;
			if (position == null) {
				line = editor.get_lineno ();
				column = editor.get_column ();
			} else {
				line = editor.get_line_from_position (position);
				column = editor.get_line_begin_position (line).diff (position);
			}
			return locator.locate(source, line, column);
		}
	}

	internal List<Vala.Symbol> lookup_symbol (Vala.Expression? inner, string name, bool prefix_match,
									 Vala.Block block) {
		List<Vala.Symbol> matching_symbols = null;

		lock (context) {
			if (inner == null) {
				for (var sym = (Vala.Symbol) block; sym != null; sym = sym.parent_symbol) {
					matching_symbols.concat (symbol_lookup_inherited (sym, name, prefix_match));
				}

				foreach (var ns in block.source_reference.file.current_using_directives) {
					matching_symbols.concat (symbol_lookup_inherited (ns.namespace_symbol, name, prefix_match));
				}
			} else if (inner.symbol_reference != null) {
					matching_symbols.concat (symbol_lookup_inherited (inner.symbol_reference, name, prefix_match));
			} else if (inner is Vala.MemberAccess) {
				var inner_ma = (Vala.MemberAccess) inner;
				var matching = lookup_symbol (inner_ma.inner, inner_ma.member_name, false, block);
				if (matching != null)
					matching_symbols.concat (symbol_lookup_inherited (matching.data, name, prefix_match));
			} else if (inner is Vala.MethodCall) {
				var inner_inv = (Vala.MethodCall) inner;
				var inner_ma = inner_inv.call as Vala.MemberAccess;
				if (inner_ma != null) {
					var matching = lookup_symbol (inner_ma.inner, inner_ma.member_name, false, block);
					if (matching != null)
						matching_symbols.concat (symbol_lookup_inherited (matching.data, name, prefix_match, true));
				}
			}
		}
		return matching_symbols;
	}
	List<Vala.Symbol> symbol_lookup_inherited (Vala.Symbol? sym, string name, bool prefix_match, bool invocation = false) {
		List<Vala.Symbol> result = null;

		// This may happen if we cannot find all the needed packages
		if (sym == null)
			return result;

		var symbol_table = sym.scope.get_symbol_table ();
		if (symbol_table != null) {
			foreach (string key in symbol_table.get_keys()) {
				if (((prefix_match && key.has_prefix (name)) || key == name)) {
					result.append (symbol_table[key]);
				}
			}
		}
		if (invocation && sym is Vala.Method) {
			var func = (Vala.Method) sym;
			result.concat (symbol_lookup_inherited (func.return_type.data_type, name, prefix_match));
		} else if (sym is Vala.Class) {
			var cl = (Vala.Class) sym;
			foreach (var base_type in cl.get_base_types ()) {
				result.concat (symbol_lookup_inherited (base_type.data_type, name, prefix_match));
			}
		} else if (sym is Vala.Struct) {
			var st = (Vala.Struct) sym;
			result.concat (symbol_lookup_inherited (st.base_type.data_type, name, prefix_match));
		} else if (sym is Vala.Interface) {
			var iface = (Vala.Interface) sym;
			foreach (var prerequisite in iface.get_prerequisites ()) {
				result.concat (symbol_lookup_inherited (prerequisite.data_type, name, prefix_match));
			}
		} else if (sym is Vala.LocalVariable) {
			var variable = (Vala.LocalVariable) sym;
			result.concat (symbol_lookup_inherited (variable.variable_type.data_type, name, prefix_match));
		} else if (sym is Vala.Field) {
			var field = (Vala.Field) sym;
			result.concat (symbol_lookup_inherited (field.variable_type.data_type, name, prefix_match));
		} else if (sym is Vala.Property) {
			var prop = (Vala.Property) sym;
			result.concat (symbol_lookup_inherited (prop.property_type.data_type, name, prefix_match));
		} else if (sym is Vala.Parameter) {
			var fp = (Vala.Parameter) sym;
			result.concat (symbol_lookup_inherited (fp.variable_type.data_type, name, prefix_match));
		}

		return result;
	}
	void update_file (Vala.SourceFile file) {
		lock (context) {
			/* Removing nodes in the same loop causes problems (probably due to ReadOnlyList)*/
			var nodes = new Vala.ArrayList<Vala.CodeNode> ();
			foreach (var node in file.get_nodes()) {
				nodes.add(node);
			}
			foreach (var node in nodes) {
				file.remove_node (node);
				if (node is Vala.Symbol) {
					var sym = (Vala.Symbol) node;
					if (sym.owner != null)
						/* we need to remove it from the scope*/
						sym.owner.remove(sym.name);
					if (context.entry_point == sym)
						context.entry_point = null;
				}
			}
			file.current_using_directives = new Vala.ArrayList<Vala.UsingDirective>();
			var ns_ref = new Vala.UsingDirective (new Vala.UnresolvedSymbol (null, "GLib"));
			file.add_using_directive (ns_ref);

			report.clear_error_indicators ();

			parse ();

			report.update_errors(current_editor);
		}
	}
}

[ModuleInit]
public Type anjuta_glue_register_components (TypeModule module) {
    return typeof (ValaPlugin);
}
