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
	uint editor_watch_id;

	Vala.CodeContext context;
	Vala.Map<string,Vala.SourceFile> source_files;
	BlockLocator locator = new BlockLocator ();

	AnjutaReport report;
	ValaProvider provider;

	Vala.Parser parser;
	Vala.Genie.Parser genie_parser;
	Vala.SymbolResolver resolver;
	Vala.SemanticAnalyzer analyzer;

	public override bool activate () {
		//debug("Activating ValaPlugin");
		context = new Vala.CodeContext();
		report = new AnjutaReport();
		report.docman = (IAnjuta.DocumentManager) shell.get_object("IAnjutaDocumentManager");
		context.report = report;
		context.profile = Vala.Profile.GOBJECT;

		var project = (IAnjuta.ProjectManager) shell.get_object("IAnjutaProjectManager");
		weak List<string> packages = project.get_packages();
		add_package(context, "glib-2.0");
		add_package(context, "gobject-2.0");

		var status = shell.get_status ();
		foreach(var pkg in packages) {
			if (!add_package(context, pkg))
				status.set("Package %s not found", pkg);
		}

		source_files = new Vala.HashMap<string, Vala.SourceFile>(str_hash, str_equal, direct_equal);

		weak List<File> sources = project.get_elements(Anjuta.ProjectNodeType.SOURCE);
		foreach (var src in sources) {
			if (src.get_path() != null && !source_files.contains(src.get_path())) {
				if (src.get_basename().has_suffix("vala") || src.get_basename().has_suffix("gs")) {
					var vsrc = new Vala.SourceFile(context, src.get_path());
					context.add_source_file(vsrc);
					var ns_ref = new Vala.UsingDirective (new Vala.UnresolvedSymbol (null, "GLib", null));
					vsrc.add_using_directive (ns_ref);
					context.root.add_using_directive (ns_ref);
					source_files[src.get_path()] = vsrc;
				} else if (src.get_basename().has_suffix("vapi")) {
					var vsrc = new Vala.SourceFile (context, src.get_path(), true);
					context.add_source_file(vsrc);
					source_files[src.get_path()] = vsrc;
				}
			}
		}
		ThreadFunc parse = () => {
			lock (context) {
				Vala.CodeContext.push(context);
				var report = context.report as AnjutaReport;

				parser = new Vala.Parser ();
				genie_parser = new Vala.Genie.Parser ();
				resolver = new Vala.SymbolResolver ();
				analyzer = new Vala.SemanticAnalyzer ();

				parser.parse (context);
				genie_parser.parse (context);
				if (report.errors_found ())
					return null;

				resolver.resolve (context);
				if (report.errors_found ())
					/* TODO: there may be missing packages */
					return null;

				analyzer.analyze (context);

				Vala.CodeContext.pop();
			}
		};

		try {
			Thread.create(parse, false);
			debug("Using threads");
		} catch (ThreadError err) {
			parse();
		}

		provider = new ValaProvider(this);
		editor_watch_id = add_watch("document_manager_current_document",
									(PluginValueAdded) editor_value_added,
									(PluginValueRemoved) editor_value_removed);

		return true;
	}

	public override bool deactivate () {
		//debug("Deactivating ValaPlugin");
		remove_watch(editor_watch_id, true);

		context = null;
		source_files = null;

		return true;
	}

	/* "document_manager_current_document" watch */
	public void editor_value_added(string name, Value value) {
		//debug("editor value added");
		assert (current_editor == null);
		assert (value.get_object() is IAnjuta.Editor);

		current_editor = value.get_object() as IAnjuta.Editor;
		if (current_editor != null) {
			if (current_editor is IAnjuta.EditorAssist)
				(current_editor as IAnjuta.EditorAssist).add(provider);
			if (current_editor is IAnjuta.EditorTip)
				(current_editor as IAnjuta.EditorTip).char_added += on_char_added;
			if (current_editor is IAnjuta.EditorHover)
				(current_editor as IAnjuta.EditorHover).hover_over += report.on_hover_over;
			if (current_editor is IAnjuta.FileSavable) {
				var file_savable = (IAnjuta.FileSavable) current_editor;
				file_savable.saved += (savable, gfile) => {
					/* gfile's type is Object, should be File */
					var file = (File) gfile;
					var source_file = source_files.get(file.get_path());
					string contents;
					try {
						file.load_contents (null, out contents, null, null);
					} catch (Error e) {
						// ignore
					}
					source_file.content = contents;
					update_file (source_file);
				};
			}
		}
		report.update_errors (current_editor);
	}
	public void editor_value_removed(string name) {
		//debug("editor value removed");
		if (current_editor is IAnjuta.EditorAssist)
			(current_editor as IAnjuta.EditorAssist).remove(provider);
		if (current_editor is IAnjuta.EditorTip)
			(current_editor as IAnjuta.EditorTip).char_added -= on_char_added;
		if (current_editor is IAnjuta.EditorHover)
			(current_editor as IAnjuta.EditorHover).hover_over -= report.on_hover_over;
		if (current_editor is IAnjuta.FileSavable)
			assert (SignalHandler.disconnect_matched (current_editor, SignalMatchType.DATA,
													  0, 0, null, null, this) == 1);

		current_editor = null;
	}

	public void on_char_added (IAnjuta.EditorTip editor, Object position, char ch) {
		var current_position = (IAnjuta.Iterable) position;
		/* include the just added char */
		current_position.next ();
		if (ch == '(') {
			var line_start = editor.get_line_begin_position(current_editor.get_lineno());
			var current_text = editor.get_text(line_start, current_position);
			provider.show_call_tip (editor, current_text);
		} else if (ch == ')') {
			editor.cancel ();
		}
	}
	internal Vala.Block get_current_block (IAnjuta.Editor editor) requires (editor is IAnjuta.File) {
		var file = editor as IAnjuta.File;

		var path = file.get_file().get_path();
		lock (context) {
			if (!(path in source_files)) {
				var src = new Vala.SourceFile(context, path, path.has_suffix("vapi"));
				context.add_source_file(src);
				source_files[path] = src;
				update_file(src);
			}
			return locator.locate(source_files[path], editor.get_lineno(), editor.get_column());
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
		} else if (sym is Vala.FormalParameter) {
			var fp = (Vala.FormalParameter) sym;
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
			context.root.add_using_directive (ns_ref);

			report.clear_error_indicators ();

			Vala.CodeContext.push (context);

			/* visit_source_file checks for the file extension */
			parser.visit_source_file (file);
			genie_parser.visit_source_file (file);

			if (!report.errors_found ()) {
				resolver.resolve (context);
				if (!report.errors_found ())
					analyzer.visit_source_file (file);
			}

			Vala.CodeContext.pop ();

			report.update_errors(current_editor);
		}
	}
}


/* Copied from valac */
public bool add_package (Vala.CodeContext context, string pkg) {
	if (context.has_package (pkg)) {
		// ignore multiple occurences of the same package
		return true;
	}
	var package_path = context.get_package_path (pkg, new string[]{});

	if (package_path == null) {
		return false;
	}

	context.add_package (pkg);

	context.add_source_file (new Vala.SourceFile (context, package_path, true));

	var deps_filename = Path.build_filename (Path.get_dirname (package_path), "%s.deps".printf (pkg));
	if (FileUtils.test (deps_filename, FileTest.EXISTS)) {
		try {
			string deps_content;
			ulong deps_len;
			FileUtils.get_contents (deps_filename, out deps_content, out deps_len);
			foreach (string dep in deps_content.split ("\n")) {
				if (dep != "") {
					if (!add_package (context, dep)) {
						context.report.err (null, "%s, dependency of %s, not found in specified Vala API directories".printf (dep, pkg));
					}
				}
			}
		} catch (FileError e) {
			context.report.err (null, "Unable to read dependency file: %s".printf (e.message));
		}
	}

	return true;
}

[ModuleInit]
public Type anjuta_glue_register_components (TypeModule module) {
    return typeof (ValaPlugin);
}
