#!/bin/perl -w
# 
# Copyright (C) 2004 Naba Kumar  <naba@gnome.org>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

use strict;
use Data::Dumper;

my $idl_file = "libanjuta.idl";

open (INFILE, "<$idl_file")
	or die "Can not open IDL file for reading";

my %marshallers = ();
my $parent_class = "";
my $current_class = "";
my @classes;

my $level = 0;
my $inside_block = 0;
my $inside_comment = 0;
my $comments = "";
my $line = "";
my $linenum = 1;

my $data_hr = {};

while ($line = <INFILE>)
{
	if ($line =~ /\/\*/) {
		$inside_comment = 1;
	}
	if ($inside_comment)
	{
		$comments .= $line;
		if ($line =~ /\*\//) {
			$inside_comment = 0;
		}
		next;
	}
	chomp ($line);
	## Remove comments
	$line =~ s/\\\\.*$//;
	$line =~ s/^\s+//;
	$line =~ s/\s+$//;
	next if ($line =~ /^\s*$/);
		
	if (is_block_begin($line))
	{
		$inside_block = 1;
		$level++;
		next;
	}
	
	my $class;
	if (is_class($line, \$class))
	{
		if ($current_class ne "")
		{
			$parent_class = $current_class;
		}
		push (@classes, $current_class);
		$current_class = $class;
		compile_class($data_hr, $parent_class, $current_class);
		next;
	}
	my $method_hr = {};
	if (is_method($line, $method_hr)) {
		die "Parse error at $idl_file:$linenum: Class name expected"
			if ($current_class eq "");
		die "Parse error at $idl_file:$linenum: Block begin expected"
			if (!$inside_block);
		my $comments_in = $comments;
		if ($comments ne "")
		{
			$comments = "";
		}
		compile_method($data_hr, $parent_class, $current_class, $method_hr, $comments_in);
		next;
	}
	if (is_block_end($line))
	{
		$inside_block = 0;
		$level--;
		$current_class = "";
		$parent_class = "";
		if (@classes > 0)
		{
			$current_class = pop (@classes);
		}
		if (@classes > 0)
		{
			$parent_class = pop(@classes);
			push (@classes, $parent_class);
		}
		next;
	}
}

## print Dumper($data_hr);
generate_files($data_hr);

sub is_block_begin
{
	my ($line) = @_;
	if ($line =~ /^\s*\{\s*$/)
	{
		## print "Block begin\n";
		return 1;
	}
	return 0;
}

sub is_block_end
{
	my ($line) = @_;
	if ($line =~ /^\s*\}\s*$/)
	{
		## print "Block End\n";
		return 1;
	}
	return 0;
}

sub is_class
{
	my ($line, $class_ref) = @_;
	if ($line =~ /^\s*([\w|_]+)\s*$/)
	{
		$$class_ref = $1;
		## print "Class: $line\n";
		return 1;
	}
	return 0;
}

sub is_method
{
	my ($line, $method_hr) = @_;
	if ($line =~ s/([\w_]+)\s*\((.*)\)\s*\;\s*$//)
	{
		my $function = $1;
		my $args = $2;
		my $rettype = $line;
		$function =~ s/^\s+//;
		$function =~ s/\s+$//;
		$args =~ s/^\s+//;
		$args =~ s/\s+$//;
		$rettype =~ s/^\s+//;
		$rettype =~ s/\s+$//;
		if ($rettype =~ s/\:\:$//) {
			$function = "::".$function;
		}
		$rettype =~ s/\s+$//;
		
		$method_hr->{'rettype'} = $rettype;
		$method_hr->{'function'} = $function;
		$method_hr->{'args'} = $args;
		## print "Function: $line\n";
		return 1;
	}
	return 0;
}

sub compile_class
{
	my ($data_hr, $parent, $class) = @_;
	die "Error $idl_file:$linenum: Class $class already exist"
		if (ref($data_hr->{$class}) eq 'HASH');
	$data_hr->{$class} = {};
	my $class_ref = $data_hr->{$class};
	
	$class_ref->{'__parent'} = $parent;
}

sub compile_method
{
	my ($data_hr, $parent, $class, $method_hr, $comments) = @_;
	die "Error $idl_file:$linenum: Class $class doesn't exist"
		if (ref($data_hr->{$class}) ne 'HASH');
	my $class_hr = $data_hr->{$class};
	my $method = $method_hr->{'function'};
	
	die "Error $idl_file:$linenum: Method $method already defined for class $class."
		if (ref($class_hr->{$method}) eq 'HASH');
	
	$method_hr->{'__comments'} = $comments;
	$class_hr->{$method} = $method_hr;
}

## GObject based C class files
sub generate_files
{
	my ($data_hr) = @_;
	foreach my $c (sort keys %$data_hr)
	{
		my $parent = $data_hr->{$c}->{'__parent'};
		print "Generating Interface $c";
		if ($parent ne '') {
			print ": $parent";
		}
		print "\n";
		generate_class($c, $data_hr->{$c});
	}
	## Write marshallers
	my $filename = "iface-marshallers.list";
	open (OUTFILE, ">$filename")
		or die "Can not open $filename for writing";
	foreach my $m (sort keys %marshallers)
	{
		$m =~ s/__/\:/;
		$m =~ s/_/\,/g;
		print OUTFILE "$m\n";
	}
	close (OUTFILE);
}

sub get_canonical_names
{
	my ($class, $cano_name_ref, $cano_macro_prefix_ref,
		$cano_macro_suffix_ref, $cano_macro_ref) = @_;
	my $c = $class;
	
	while ($c =~ s/([A-Z])/\@$1\@/)
	{
		
		my $uw = $1;
		## print "Word separator: $uw\n";
		my $lw = lc($uw);
		$c =~ s/\@$uw\@/_$lw/;
	}
	$c =~ s/_(\w)_/_$1/g;
	$c =~ s/^_//;
	my $prefix = "";
	my $suffix = $c;
	if ($suffix =~ s/(\w+?)_//)
	{
		$prefix = $1;
	}
	
	$$cano_name_ref = $c;
	$$cano_macro_prefix_ref = uc($prefix);
	$$cano_macro_suffix_ref = uc($suffix);
	$$cano_macro_ref = uc($c);
}

sub construct_marshaller
{
	my ($rettype, $args) = @_;
	## Type mapping
	my %type_map = (
		"void" => "G_TYPE_NONE",
		"GValue" => "G_TYPE_BOXED",
		"gchar*" => "G_TYPE_STRING",
		"constgchar*" => "G_TYPE_STRING",
		"gint" => "G_TYPE_INT",
		"gboolean" => "G_TYPE_BOOLEAN",
		"GInterface*" => "G_TYPE_INTERFACE",
		## G_TYPE_CHAR
		"guchar*" => "G_TYPE_UCHAR",
		"constguchar*" => "G_TYPE_UCHAR",
		"guint" => "G_TYPE_UINT",
		"glong" => "G_TYPE_LONG",
		"gulong" => "G_TYPE_ULONG",
		"gint64" => "G_TYPE_INT64",
		"guint64" => "G_TYPE_UINT64",
		## G_TYPE_ENUM
		## G_TYPE_FLAGS
		"gfloat" => "G_TYPE_FLOAT",
		"gdouble" => "G_TYPE_DOUBLE",
		"gpointer" => "G_TYPE_POINTER",
		"GValue*" => "G_TYPE_BOXED",
		## G_TYPE_PARAM
		"GObject*" => "G_TYPE_OBJECT"
	);
	$rettype =~ s/\s//g;
	my $rtype = $type_map{$rettype};
	if (!defined($rtype) or $rtype eq '')
	{
		die "Can not find GType for arg type '$rettype'. Fix it first.";
	}
	my $args_list = $rtype.",\n\t\t\t__arg_count__";
	$rtype =~ s/^G_TYPE_//;
	$rtype =~ s/NONE/VOID/;
	my $marshal_index = $rtype. "_";
	my $arg_count = 0;
	if ($args ne '')
	{
		my @params;
		my @margs = split(",", $args);
		foreach my $one_arg (@margs)
		{
			if ($one_arg =~ s/([\w_]+)$//)
			{
				$one_arg =~ s/\s//g;
				my $oarg = $type_map{$one_arg};
				if (!defined($oarg) or $oarg eq '')
				{
					die "Can not find GType for arg type '$one_arg'. Fix it first.";
				}
				$args_list .= ",\n\t\t\t$oarg";
				$oarg =~ s/^G_TYPE_//;
				$oarg =~ s/NONE/VOID/;
				$marshal_index .= "_$oarg";
				$arg_count++;
			}
		}
	}
	$args_list =~ s/__arg_count__/$arg_count/;
	if ($arg_count == 0)
	{
		$args_list .= ",\n\t\t\tNULL";
		$marshal_index .= "_VOID";
	}
	$marshallers{$marshal_index} = 1;
	$marshal_index = "iface_cclosure_marshal_$marshal_index";
	return $marshal_index.",\n\t\t\t".$args_list;
}

sub generate_class
{
	my ($class, $class_hr) = @_;
	my $parent = $class_hr->{'__parent'};
	
	my $prefix;
	my $macro_prefix;
	my $macro_suffix;
	my $macro_name;
	
	get_canonical_names($class, \$prefix, \$macro_prefix,
						\$macro_suffix, \$macro_name);
	
	my $macro_type = "${macro_prefix}_TYPE_${macro_suffix}";
	my $macro_assert = "${macro_prefix}_IS_${macro_suffix}";
	$macro_type =~ /^_/;
	$macro_assert =~ /^_/;
	
	my $parent_iface = "GTypeInterface";
	my $parent_type = "G_TYPE_INTERFACE";
	if ($parent ne "")
	{
		my ($pprefix, $pmacro_name, $pmacro_prefix, $pmacro_suffix);
		$parent_iface = "${parent}Iface";
		
		get_canonical_names($parent, \$pprefix, \$pmacro_prefix,
							\$pmacro_suffix, \$pmacro_name);
		$parent_type = $pmacro_prefix."_TYPE_".$pmacro_suffix;
	}
	
	print "\tmethod prefix: $prefix\n";
	print "\tmacro prefix: $macro_prefix\n";
	print "\tmacro suffix: $macro_suffix\n";
	print "\tmacro name: $macro_name\n\n";

	my $filename = "$prefix.h";
	$filename =~ s/_/-/g;

	my $answer =
"/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  $filename -- Autogenerated from $idl_file
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _${macro_name}_H_
#define _${macro_name}_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define $macro_type (${prefix}_get_type ())
#define ${macro_name}(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), , $class))
#define ${macro_assert}(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), $macro_type))
#define ${macro_name}_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), $macro_type, ${class}Iface))

#define ${macro_name}_ERROR ${prefix}_error_quark()

typedef struct _$class $class;
typedef struct _${class}Iface ${class}Iface;

struct _${class}Iface {
	$parent_iface g_iface;
	
";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = $class_hr->{$m}->{'rettype'};
		my $args = $class_hr->{$m}->{'args'};
		if ($args ne '')
		{
			$args = ", ".$args;
		}
		if ($func =~ s/^\:\://)
		{
			$answer .= "\t/* Signal */\n";
			$answer .= "\t$rettype (*$func) ($class obj${args});\n";
		}
	}
	$answer .= "\n";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = $class_hr->{$m}->{'rettype'};
		my $args = $class_hr->{$m}->{'args'};
		if ($args ne '')
		{
			$args .= ", ";
		}
		if ($func !~ /^\:\:/)
		{
			$answer .= "\t$rettype (*$func) ($class obj, ${args}GError **err);\n";
		}
	}
	$answer .=
"
};
GQuark ${prefix}_error_quark     (void);
GType  ${prefix}_get_type        (void);

";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = $class_hr->{$m}->{'rettype'};
		my $args = $class_hr->{$m}->{'args'};
		if ($args ne '')
		{
			$args .= ", ";
		}
		if ($func !~ /^\:\:/)
		{
			$answer .= "${rettype} ${prefix}_$func ($class obj, ${args}GError **err);\n\n";
		}
	}
	$answer .=
"
G_END_DECLS

#endif
";
	open (OUTFILE, ">$filename")
		or die "Can not open $filename for writing";
	print OUTFILE $answer;
	close (OUTFILE);

	## Source file.
	
	$answer = "";
	my $headerfile = $filename;
	$filename = "$prefix.c";
	$filename =~ s/_/-/g;
	my $dash_prefix = $prefix;
	$dash_prefix =~ s/_/-/g;
	$answer = 
"/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  $filename -- Autogenerated from $idl_file
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include \"$headerfile\"

GQuark 
${prefix}_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string (\"${dash_prefix}-quark\");
	}
	
	return quark;
}

";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = $class_hr->{$m}->{'rettype'};
		my $args = $class_hr->{$m}->{'args'};
		my $params = "";
		next if ($func =~ /^\:\:/);

		if ($args ne '')
		{
			my @params;
			my @margs = split(",", $args);
			foreach my $one_arg (@margs)
			{
				if ($one_arg =~ /([\w_]+)$/)
				{
					push @params, $1;
				}
			}
			$params = join (", ", @params);
			$params .= ", ";
		}
		
		if ($args ne '')
		{
			$args .= ", ";
		}
		my $comments_out = $class_hr->{$m}->{'__comments'};
		$comments_out =~ s/^\t+//mg;
		$answer .= "${comments_out}${rettype}\n${prefix}_$func ($class *obj, ${args}GError **err)\n";
		$answer .= "{\n\t";
		if ($rettype ne "void") {
			$answer .= "return ";
		}
		$answer .= "${macro_name}_GET_IFACE (shell)->$func (obj, ${params}err);";
		$answer .="\n}\n\n";
	}
	$answer .=
"static void
${prefix}_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	
	if (!initialized) {
";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = $class_hr->{$m}->{'rettype'};
		my $args = $class_hr->{$m}->{'args'};
		if ($args ne '')
		{
			$args = ", ".$args;
		}
		if ($func =~ s/^\:\://)
		{
			my $marshaller = construct_marshaller($rettype, $args);
			$answer .= "\t\t/* Signal */";
			$answer .="\n\t\tg_signal_new (\"$func\",
			$macro_type,
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (${class}Iface, $func),
			NULL, NULL,
			${marshaller});\n\n";
		}
	}
	
	$answer .=
"
		initialized = TRUE;
	}
}

GType
${prefix}_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (${class}Iface),
			${prefix}_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		type = g_type_register_static ($parent_type, \"$class\", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;			
}
";
	open (OUTFILE, ">$filename")
		or die "Can not open $filename for writing";
	print OUTFILE $answer;
	close (OUTFILE);
}
