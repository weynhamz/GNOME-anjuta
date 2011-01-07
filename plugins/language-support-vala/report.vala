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
		var mark = editor as IAnjuta.Markable;
		if (ind == null && mark == null)
			return;

		if (ind != null)
			ind.clear ();
		if (mark != null)
			mark.delete_all_markers (IAnjuta.MarkableMarker.MESSAGE);

		foreach (var e in errors) {
			if (e.source.file.filename.has_suffix (((IAnjuta.Document)editor).get_filename ())) {
				if (ind != null) {
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
				if (editor is IAnjuta.Markable) {
					mark.mark(e.source.first_line, IAnjuta.MarkableMarker.MESSAGE, e.message);
				}
			}

		}
	}
	public void clear_error_indicators () {
		errors = new Vala.ArrayList<Error?>();
		foreach (var doc in docman.get_doc_widgets ()) {
			if (doc is IAnjuta.Indicable)
				((IAnjuta.Indicable)doc).clear ();
			if (doc is IAnjuta.Markable)
				((IAnjuta.Markable)doc).delete_all_markers (IAnjuta.MarkableMarker.MESSAGE);
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
