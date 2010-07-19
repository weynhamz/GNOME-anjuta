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

public class ValaProvider : Object, IAnjuta.Provider {
	IAnjuta.Iterable start_pos;
	weak ValaPlugin plugin;

	static Regex member_access;
	static Regex member_access_split;
	static Regex function_call;

	static construct {
		try {
			member_access = new Regex("""((?:\w+(?:\s*\([^()]*\))?\.)*)(\w*)$""");
			member_access_split = new Regex ("""(\s*\([^()]*\))?\.""");
			function_call = new Regex("""(new )?((?:\w+(?:\s*\([^()]*\))?\.)*)(\w+)\s*\(([^(,)]+,)*([^(,)]*)$""");
		} catch(RegexError err) {
			critical("Regular expressions failed to compile : %s", err.message);
		}
	}

	public ValaProvider(ValaPlugin plugin) {
		this.plugin = plugin;
	}
	public unowned string get_name () throws GLib.Error {
		return "Vala";
	}
	public void populate (IAnjuta.Iterable iter) throws GLib.Error {
		var editor = plugin.current_editor as IAnjuta.EditorAssist;
		var line_start = editor.get_line_begin_position(editor.get_lineno());
		var current_text = editor.get_text(line_start, iter);

		MatchInfo match_info;
		if (! member_access.match(current_text, 0, out match_info))
			return;
		if (match_info.fetch(0).length < 2)
			return;

		start_pos = iter.clone();
		start_pos.set_position(iter.get_position() - (int) match_info.fetch(2).length);

		var names = member_access_split.split (match_info.fetch(1));

		var syms = plugin.lookup_symbol (construct_member_access (names), match_info.fetch(2),
		                                 true, plugin.get_current_block(editor));

		var proposals = new GLib.List<IAnjuta.EditorAssistProposal?>();
		foreach (var symbol in syms) {
			if (symbol is Vala.LocalVariable
			    && symbol.source_reference.first_line > editor.get_lineno())
				continue;

			var prop = IAnjuta.EditorAssistProposal();
			prop.data = symbol;
			prop.label = symbol.name;
			proposals.prepend(prop);
		}
		proposals.reverse();
		editor.proposals(this, proposals, true);
	}
	public unowned IAnjuta.Iterable get_start_iter () throws GLib.Error {
		return start_pos;
	}
	public void activate (IAnjuta.Iterable iter, void* data) {
		var sym = data as Vala.Symbol;
		var editor = plugin.current_editor as IAnjuta.EditorAssist;
		(editor as IAnjuta.Document).begin_undo_action();
		editor.erase(start_pos, iter);
		editor.insert(start_pos, sym.name, -1);
		(editor as IAnjuta.Document).end_undo_action();
	}

	public void show_call_tip (IAnjuta.EditorTip editor, string to_complete) {
		List<string> tips = null;

		MatchInfo match_info;
		if (! function_call.match(to_complete, 0, out match_info))
			return;

		var creation_method = (match_info.fetch(1) != "");
		var names = member_access_split.split (match_info.fetch(2));
		var syms = plugin.lookup_symbol (construct_member_access (names), match_info.fetch(3),
		                                 false, plugin.get_current_block (editor));

		foreach (var sym in syms) {
			Vala.List<Vala.FormalParameter> parameters = null;
			if (sym is Vala.Method) {
				parameters = ((Vala.Method) sym).get_parameters ();
			} else if (sym is Vala.Signal) {
				parameters = ((Vala.Signal) sym).get_parameters ();
			} else if (sym is Vala.Delegate) {
				parameters = ((Vala.Delegate) sym).get_parameters ();
			} else if (creation_method && sym is Vala.Class) {
				parameters = ((Vala.Class)sym).default_construction_method.get_parameters ();
			} else {
				return_if_reached ();
			}
			var calltip = new StringBuilder ("(");
			var first = true;
			foreach (var p in parameters) {
				if(first) {
					first = false;
				} else {
					calltip.append(", ");
				}
				if (p.ellipsis) {
					calltip.append("...");
				} else {
					calltip.append (p.variable_type.to_qualified_string());
					calltip.append (" ").append (p.name);
				}
			}
			calltip.append (")");
			tips.append (calltip.str);
		}
		editor.show (tips, editor.get_position ());
	}
	Vala.Expression construct_member_access (string[] names) {
		Vala.Expression expr = null;

		for (var i = 0; names[i] != null; i++) {
			if (names[i] != "") {
				expr = new Vala.MemberAccess (expr, names[i]);
				if (names[i+1] != null && names[i+1].chug ().has_prefix ("(")) {
					expr = new Vala.MethodCall (expr);
					i++;
				}
			}
		}

		return expr;
	}

}

