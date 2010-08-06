/*
 * Copyright (C) 2008-2009 Abderrahim Kitouni
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

 /* A Vala.Report subclass for reporting errors in Anjuta UI */
public class AnjutaReport : Vala.Report {
	struct Error {
		public Vala.SourceReference source;
		public bool error;
		public string message;
	}
	public IAnjuta.DocumentManager docman { get; set; }
	Vala.List<Error?> errors = new Vala.ArrayList<Error?>();
	public void update_errors (IAnjuta.Editor editor) {
		var ind = editor as IAnjuta.Indicable;
		if (ind == null)
			return;
		ind.clear ();
		foreach (var e in errors) {
			if (e.source.file.filename.has_suffix (((IAnjuta.Document)editor).get_filename ())) {
				/* begin_iter should be one cell before to select the first character */
				var begin_iter = editor.get_line_begin_position (e.source.first_line);
				for (var i = 1; i < e.source.first_column; i++)
					begin_iter.next ();
				var end_iter = editor.get_line_begin_position (e.source.last_line);
				for (var i = 0; i < e.source.last_column; i++)
					end_iter.next ();
				ind.set(begin_iter, end_iter, e.error ? IAnjuta.IndicableIndicator.CRITICAL :
														IAnjuta.IndicableIndicator.WARNING);
			}

		}
	}
	public void clear_error_indicators () {
		errors = new Vala.ArrayList<Error?>();
		foreach (var doc in (List<Gtk.Widget>)docman.get_doc_widgets ()) {
			if (doc is IAnjuta.Indicable)
				((IAnjuta.Indicable)doc).clear ();
		}
	}
	public void on_hover_over (IAnjuta.EditorHover editor, Object pos) {
		var position = (IAnjuta.Iterable) pos;
		lock (errors) {
			foreach (var error in errors) {
				if (!error.source.file.filename.has_suffix (((IAnjuta.Document)editor).get_filename ()))
					continue;

				var begin = editor.get_line_begin_position (error.source.first_line);
				for (var i = 0; i < error.source.first_column; i++)
					begin.next();

				var end = editor.get_line_begin_position (error.source.last_line);
				for (var i = 0; i < error.source.last_column; i++)
					end.next();

				if (position.compare(begin) >= 0 && position.compare(end) <= 0) {
					editor.display(position, error.message);
					return;
				}
			}
		}
	}
	public override void warn (Vala.SourceReference? source, string message) {
		if (source == null)
			return;

		lock (errors) {
			errors.add(Error () {source = source, message = message, error = false});
		}
	}
	public override void err (Vala.SourceReference? source, string message) {
		if (source == null)
			return;

		lock (errors) {
			errors.add(Error () {source = source, message = message, error = true});
		}
	}
	public bool errors_found () {
		return (errors.size != 0);
	}
}
