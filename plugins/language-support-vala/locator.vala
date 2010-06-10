/*
 * Copyright (C) 2008 Abderrahim Kitouni
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

/* Finds the innermost block containing the given location */
public class BlockLocator : Vala.CodeVisitor {
	double location;
	Vala.Block innermost = null;
	double innermost_begin;
	double innermost_end;

	public Vala.Block locate (Vala.SourceFile file, int line, int column) {
		// XXX : assumes that line length < 1000
		location = line + column / 1000.0;
		innermost = null;
		file.accept_children(this);
		return innermost;
	}
	public override void visit_block (Vala.Block b) {
		var begin = b.source_reference.first_line + b.source_reference.first_column / 1000.0;
		var end = b.source_reference.last_line + b.source_reference.last_column / 1000.0;

		if (begin <= location && location <= end)
			if (innermost == null || (innermost_begin <= begin && innermost_end >= end))
				innermost = b;
	}

	public override void visit_namespace (Vala.Namespace ns) {
		ns.accept_children(this);
	}
	public override void visit_class (Vala.Class cl) {
		cl.accept_children(this);
	}
	public override void visit_struct (Vala.Struct st) {
		st.accept_children(this);
	}
	public override void visit_interface (Vala.Interface iface) {
		iface.accept_children(this);
	}
	public override void visit_method (Vala.Method m) {
		m.accept_children(this);
	}
	public override void visit_creation_method (Vala.CreationMethod m) {
		m.accept_children(this);
	}
	public override void visit_property (Vala.Property prop) {
		prop.accept_children(this);
	}
	public override void visit_property_accessor (Vala.PropertyAccessor acc) {
		acc.accept_children(this);
	}
	public override void visit_constructor (Vala.Constructor c) {
		c.accept_children(this);
	}
	public override void visit_destructor (Vala.Destructor d) {
		d.accept_children(this);
	}
	public override void visit_if_statement (Vala.IfStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_switch_statement (Vala.SwitchStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_switch_section (Vala.SwitchSection section) {
		visit_block(section);
	}
	public override void visit_while_statement (Vala.WhileStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_do_statement (Vala.DoStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_for_statement (Vala.ForStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_foreach_statement (Vala.ForeachStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_try_statement (Vala.TryStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_catch_clause (Vala.CatchClause clause) {
		clause.accept_children(this);
	}
	public override void visit_lock_statement (Vala.LockStatement stmt) {
		stmt.accept_children(this);
	}
	public override void visit_lambda_expression (Vala.LambdaExpression expr) {
		expr.accept_children(this);
	}
}

