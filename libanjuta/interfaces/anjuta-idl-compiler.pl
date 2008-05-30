#!/usr/bin/perl -w
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

use strict;
use Data::Dumper;

if (@ARGV != 1)
{
	die "Usage: perl anjuta-idl-compiler.pl module_name";
}

## Types starting with prefix mentioned in
## @known_type_prefixes array are automatically assumed to be GObject derived
## class and their type and assertion checks are determined based on that.
## This automation is skipped if the type is found in %not_classes hash table.
my $known_type_prefixes = [
	"Gdk",
	"Gtk",
	"Gnome",
	"Anjuta",
	"IAnjuta",
];

## Add your types which are not classes despite starting with above prefixes
my $not_classes = {
	"GtkTreeIter" => 1,
};

## Additional non-standard type mappings.
my $type_map = {
	"void" => {
		"gtype" => "G_TYPE_NONE",
	},
	"gchar*" => {
		"gtype" => "G_TYPE_STRING",
		"fail_return" => "NULL"
	},
	"constgchar*" => {
		"gtype" => "G_TYPE_STRING",
		"fail_return" => "NULL"
	},
	"gchar" => {
		"gtype" => "G_TYPE_CHAR",
		"fail_return" => "0"
	},
	"gint" => {
		"gtype" => "G_TYPE_INT",
		"fail_return" => "-1"
	},
	"gboolean" => {
		"gtype" => "G_TYPE_BOOLEAN",
		"fail_return" => "FALSE"
	},
	"GInterface*" => {
		"gtype" => "G_TYPE_INTERFACE",
		"assert" => "G_IS_INTERFACE (__arg__)",
		"fail_return" => "NULL"
	},
	## G_TYPE_CHAR
	"guchar*" => {
		"gtype" => "G_TYPE_UCHAR",
		"assert" => "__arg__ != NULL",
		"fail_return" => "NULL"
	},
	"constguchar*" => {
		"gtype" => "G_TYPE_UCHAR",
		"assert" => "__arg__ != NULL",
		"fail_return" => "NULL"
	},
	"guint" => {
		"gtype" => "G_TYPE_UINT",
		"fail_return" => "0"
	},
	"glong" => {
		"gtype" => "G_TYPE_LONG",
		"fail_return" => "-1"
	},
	"gulong" => {
		"gtype" => "G_TYPE_ULONG",
		"fail_return" => "0"
	},
	"gint64" => {
		"gtype" => "G_TYPE_INT64",
		"fail_return" => "-1"
	},
	"guint64" => {
		"gtype" => "G_TYPE_UINT64",
		"fail_return" => "0"
	},
	## G_TYPE_ENUM
	## G_TYPE_FLAGS
	"gfloat" => {
		"gtype" => "G_TYPE_FLOAT",
		"fail_return" => "0"
	},
	"gdouble" => {
		"gtype" => "G_TYPE_DOUBLE",
		"fail_return" => "0"
	},
	"gpointer" => {
		"gtype" => "G_TYPE_POINTER",
		"assert" => "__arg__ != NULL",
		"fail_return" => "NULL"
	},
	"GValue*" => {
		"gtype" => "G_TYPE_BOXED",
		"assert" => "G_IS_VALUE(__arg__)",
		"fail_return" => "NULL"
	},
	"GError*" => {
		"gtype" => "G_TYPE_BOXED",
		"type" => "G_TYPE_ERROR",
		"fail_return" => "NULL"
	},	
	## G_TYPE_PARAM
	"GObject*" => {
		"gtype" => "G_TYPE_OBJECT",
		"assert" => "G_IS_OBJECT(__arg__)",
		"fail_return" => "NULL"
	}
};

my $module_name = $ARGV[0];
my $idl_file = "$module_name.idl";
open (INFILE, "<$idl_file")
	or die "Can not open IDL file for reading";

my %marshallers = ();
my @global_includes;
my @class_includes;
my %class_privates = ();
my @header_files;
my @source_files;
my $parent_class = "";
my $current_class = "";
my @classes;
my @level = ();
my $inside_block = 0;
my $inside_comment = 0;
my $comments = "";
my $line = "";
my $linenum = 1;
my $data_hr = {};
my @collector = ();
my $struct = "";
my $enum = "";
my $typedef = "";

while ($line = <INFILE>)
{
	if (is_comment_begin($line)) {
		if (current_level(@level) ne "comment") {
			push @level, "comment";
		}
	}
	if (current_level(@level) eq "comment")
	{
		$comments .= $line;
		if (is_comment_end($line)) {
			splice @level, @level - 1, 1;
		}
		$linenum++;
		next;
	}
	chomp ($line);
	## Remove comments
	$line =~ s/\/\/.*$//;
	$line =~ s/^\s+//;
	$line =~ s/\s+$//;
	if ($line =~ /^\s*$/)
	{
 		$linenum++;
		next;
	}
	
	if ($line =~ /^\#include/)
	{
		if (@level <= 0)
		{
			push @global_includes, $line;
		}
		else 
		{
			my $includes_lr = $class_includes[@class_includes-1];
			push @$includes_lr, $line;
		}
		next;
	}
	if (is_block_begin($line))
	{
		$linenum++;
		next;
	}
	
	my $class;
	if (is_interface($line, \$class))
	{
		push @level, "interface";
		if ($current_class ne "")
		{
			$parent_class = $current_class;
		}
		push (@class_includes, []);
		push (@classes, $current_class);
		$current_class = $class;
		my $comments_in = get_comments();
		compile_class($data_hr, $parent_class, $comments_in, $current_class);
		$linenum++;
		next;
	}
	elsif (is_struct($line, \$class))
	{
		push @level, "struct";
		$struct = $class;
		$linenum++;
		next;
	}
	elsif (is_enum($line, \$class))
	{
		push @level, "enum";
		$enum = $class;
		$linenum++;
		next;
	}
	elsif (is_typedef($line, \$typedef))
	{
		die "Parse error at $idl_file:$linenum: typedefs should only be in interface"
			if (current_level(@level) ne "interface");
		die "Parse error at $idl_file:$linenum: Class name expected"
			if ($current_class eq "");
		my $comments_in = get_comments();
		compile_typedef($data_hr, $current_class, $comments_in, $typedef);
		$typedef = "";
		$linenum++;
		next;
	}

	my $method_hr = {};
	if (is_method($line, $method_hr)) {
		die "Parse error at $idl_file:$linenum: Methods should only be in interface"
			if (current_level(@level) ne "interface");
		die "Parse error at $idl_file:$linenum: Class name expected"
			if ($current_class eq "");
		my $comments_in = get_comments();
		compile_method($data_hr, $parent_class, $current_class, $comments_in, $method_hr);
		$linenum++;
		next;
	}
	my $define_hr = {};
	if (is_define($line, $define_hr))
	{
		die "Parse error at $idl_file:$linenum: Defines should only be in interface"
			if (current_level(@level) ne "interface");
		die "Parse error at $idl_file:$linenum: Class name expected"
			if ($current_class eq "");
		my $comments_in = get_comments();
		compile_define($data_hr, $current_class, $comments_in, $define_hr);
		$linenum++;
		next;
	}
	if (is_block_end($line))
	{
		if (current_level(@level) eq "interface")
		{
			compile_inclues($data_hr, $current_class);
			splice @class_includes, @class_includes - 1, 1;
			$current_class = "";
			$parent_class = "";
			if (@classes > 0)
			{
				$current_class = splice @classes, @classes - 1, 1;
			}
			if (@classes > 0)
			{
				$parent_class = $classes[@classes -1];
			}
			$linenum++;
		}
		elsif (current_level(@level) eq "struct")
		{
			my $comments_in = get_comments();
			$not_classes->{"$current_class$struct"} = "1";
			compile_struct($data_hr, $current_class, $struct,
						   $comments_in, @collector);
			@collector = ();
			$struct = "";
			$linenum++;
		}
		elsif (current_level(@level) eq "enum")
		{
			my $comments_in = get_comments();
			$not_classes->{"$current_class$enum"} = "1";
			compile_enum($data_hr, $current_class, $enum,
						 $comments_in, @collector);
			@collector = ();
			$enum = "";
			$linenum++;
		}
		splice @level, @level - 1, 1;
		next;
	}
	die "Parse error at $idl_file:$linenum: Type name or method expected"
		if (current_level(@level) ne "struct" and
			current_level(@level) ne "enum");
	push @collector, $line;
	$linenum++;
}

## print Dumper($data_hr);
## print Dumper(\%class_privates);
## print Dumper($not_classes);

generate_files($data_hr);

sub is_comment_begin
{
	my ($line) = @_;
	if ($line =~ /\/\*/)
	{
		## print "Comment begin\n";
		return 1;
	}
	return 0;
}

sub is_comment_end
{
	my ($line) = @_;
	if ($line =~ /\*\//)
	{
		## print "Block End\n";
		return 1;
	}
	return 0;
}

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

sub is_interface
{
	my ($line, $class_ref) = @_;
	if ($line =~ /^\s*interface\s*([\w|_]+)\s*$/)
	{
		$$class_ref = $1;
		## print "Interface: $line\n";
		return 1;
	}
	return 0;
}

sub is_enum
{
	my ($line, $class_ref) = @_;
	if ($line =~ /^\s*enum\s*([\w\d|_]+)\s*$/)
	{
		my $enum_name = $1;
		add_class_private ($current_class, $enum_name);
		$$class_ref = $enum_name;
		## print "Enum: $line\n";
		return 1;
	}
	return 0;
}

sub is_struct
{
	my ($line, $class_ref) = @_;
	if ($line =~ /^\s*struct\s*([\w\d|_]+)\s*$/)
	{
		my $struct_name = $1;
		$$class_ref = $struct_name;
		add_class_private ($current_class, $struct_name);
		## print "Struct: $line\n";
		return 1;
	}
	return 0;
}

sub is_typedef
{
	my ($line, $typedef_ref) = @_;
	if ($line =~ /^\s*typedef\s*/)
	{
		## Check if it is variable typedef and grab the typedef name.
		if ($line =~ /([\w_][\w\d_]+)\s*;\s*$/)
		{
			add_class_private ($current_class, $1);
		}
		## Check if it is function typedef and grab the typedef name.
		elsif ($line =~ /\(\s*\*([\w_][\w\d_]+)\s*\)/)
		{
			add_class_private ($current_class, $1);
		}
		$$typedef_ref = $line;
		## print "Typedef: $line\n";
		return 1;
	}
	return 0;
}

sub is_method
{
	my ($line, $method_hr) = @_;
	if ($line =~ s/([\w_][\w\d_]*)\s*\((.*)\)\s*\;\s*$//)
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
		
		$method_hr->{'function'} = $function;
		$method_hr->{'rettype'} = $rettype;
		$method_hr->{'args'} = $args;
		## print "Function: $line\n";
		return 1;
	}
	return 0;
}

sub is_define
{
	my ($line, $define_hr) = @_;
	if ($line =~ /^\s*#define\s+([\w_][\w\d_]*)\s+(.*)\s*$/)
	{
		$define_hr->{'name'} = $1;
		$define_hr->{'value'} = $2;
		## print "Define: $line\n";
		return 1;
	}
	return 0;
}

sub normalize_namespace
{
	my ($current_class, $text) = @_;
	
	my $iter_class = $current_class;
	while (1)
	{
		if (defined ($class_privates{$iter_class}))
		{
			foreach my $p (@{$class_privates{$iter_class}})
			{
				$text =~ s/\b$p\b/$iter_class$p/g;
			}
		}
		if (defined ($data_hr) &&
			defined ($data_hr->{$iter_class}) &&
			defined ($data_hr->{$iter_class}->{"__parent"}))
		{
			$iter_class = $data_hr->{$iter_class}->{"__parent"};
		}
		else
		{
			last;
		}
	}
	return $text;
}

sub current_level
{
	## @l = @_;
	## return splice @l, @l - 1, 1;
	return $_[@_-1] if (@_ > 0);
	return "";
}

sub get_comments
{
	my $comments_in = $comments;
	if ($comments ne "")
	{
		$comments = "";
	}
	$comments_in =~ s/^\t+//mg;
	return $comments_in;
}

sub add_class_private
{
	my ($class, $type) = @_;
	if (!defined ($class_privates{$class}))
	{
		$class_privates{$class} = [];
	}
	push (@{$class_privates{$class}}, $type);
}

sub is_class_private
{
	my ($class, $type) = @_;
	if (defined ($class_privates{$class}))
	{
		foreach my $p (@{$class_privates{$class}})
		{
			if ($p eq $type)
			{
				return 1;
			}
		}
	}
	return 0;
}

sub compile_class
{
	my ($data_hr, $parent, $comments, $class) = @_;
	die "Error $idl_file:$linenum: Class $class already exist"
		if (ref($data_hr->{$class}) eq 'HASH');
	$data_hr->{$class} = {};
	my $class_ref = $data_hr->{$class};
	
	$class_ref->{'__parent'} = $parent;
	$class_ref->{'__comments'} = $comments;
}

sub compile_method
{
	my ($data_hr, $parent, $class, $comments, $method_hr) = @_;
	die "Error $idl_file:$linenum: Class $class doesn't exist"
		if (ref($data_hr->{$class}) ne 'HASH');
	my $class_hr = $data_hr->{$class};
	my $method = $method_hr->{'function'};
	
	die "Error $idl_file:$linenum: Method $method already defined for class $class."
		if (ref($class_hr->{$method}) eq 'HASH');
	
	$method_hr->{'__comments'} = $comments;
	$class_hr->{$method} = $method_hr;
}

sub compile_inclues
{
	my ($data_hr, $class) = @_;
	die "Error $idl_file:$linenum: Class $class doesn't exist"
		if (ref($data_hr->{$class}) ne 'HASH');
	my $class_hr = $data_hr->{$class};
	
	if (ref($class_hr->{"__include"}) ne 'ARRAY')
	{
		my @includes;
		foreach my $inc (@global_includes)
		{
			## Normalize includes.
			$inc =~ s/\"(.+)\"/\<$module_name\/interfaces\/$1\>/;
			push @includes, $inc;
		}
		my $class_incs_lr = $class_includes[@class_includes-1];
		foreach my $inc (@$class_incs_lr)
		{
			$inc =~ s/\"(.+)\"/\<$module_name\/interfaces\/$1\>/;
			push @includes, $inc;
		}
		$class_hr->{'__include'} = \@includes;
	}	
}

sub compile_define
{
	my ($data_hr, $current_class, $comments, $define_hr) = @_;
	my $class_hr = $data_hr->{$current_class};
	if (!defined($class_hr->{"__defines"}))
	{
		$class_hr->{"__defines"} = [];
	}
	my $defines_lr = $class_hr->{'__defines'};
		
	$define_hr->{'__comments'} = $comments;
	push (@$defines_lr, $define_hr);
}

sub compile_typedef
{
	my ($data_hr, $current_class, $comments, $typedef) = @_;
	my $class_hr = $data_hr->{$current_class};
	if (!defined($class_hr->{"__typedefs"}))
	{
		$class_hr->{"__typedefs"} = [];
	}
	my $typedefs_lr = $class_hr->{"__typedefs"};
	push (@$typedefs_lr, $typedef);
}

sub compile_enum
{
	my ($data_hr, $parent, $class, $comments, @collector) = @_;
	my $class_hr = $data_hr->{$parent};
	if (!defined($class_hr->{"__enums"}))
	{
		$class_hr->{"__enums"} = {};
	}
	my $enums_hr = $class_hr->{"__enums"};
	
	my @data = @collector;
	$enums_hr->{$class} = {};
	$enums_hr->{$class}->{"__parent"} = $parent;
	$enums_hr->{$class}->{"__comments"} = $comments;
	$enums_hr->{$class}->{"__data"} = \@data;
	
	## Add to @not_classes and $type_map
	{
		my $prefix;
		my $macro_prefix;
		my $macro_suffix;
		my $macro_name;
		my $type_map_item_hr = {};
		my $enum_fullname = "$parent$class";
		
		# Add to @not_classes
		$not_classes->{$enum_fullname} = 1;
		
		# Add to $type_map
		get_canonical_names($enum_fullname, \$prefix, \$macro_prefix,
							\$macro_suffix, \$macro_name);
		## print "Enum Fullname: $enum_fullname\n";
		$type_map_item_hr->{'gtype'} = 'G_TYPE_ENUM';
		$type_map_item_hr->{'rettype'} = '0';
		$type_map_item_hr->{'type'} = $macro_prefix."_TYPE_".$macro_suffix;
		$type_map->{$enum_fullname} = $type_map_item_hr;
	}
}

sub compile_struct
{
	my ($data_hr, $parent, $class, $comments, @collector) = @_;
	my $class_hr = $data_hr->{$parent};
	if (!defined($class_hr->{"__structs"}))
	{
		$class_hr->{"__structs"} = {};
	}
	my $structs_hr = $class_hr->{"__structs"};
	
	my @data = @collector;
	$structs_hr->{$class} = {};
	$structs_hr->{$class}->{"__parent"} = $parent;
	$structs_hr->{$class}->{"__comments"} = $comments;
	$structs_hr->{$class}->{"__data"} = \@data;
	
	## Add to @not_classes and $type_map
	{
		my $prefix;
		my $macro_prefix;
		my $macro_suffix;
		my $macro_name;
		my $type_map_item_hr = {};
		my $struct_fullname = "$parent$class";
		
		# Add to @not_classes
		$not_classes->{$struct_fullname} = 1;
		
		# Add to $type_map
		## print "haha struct $struct_fullname\n";
		$type_map_item_hr->{'gtype'} = 'G_TYPE_POINTER';
		$type_map_item_hr->{'rettype'} = 'NULL';
		$type_map_item_hr->{'type'} = "G_TYPE_POINTER";
		$type_map->{"${struct_fullname}*"} = $type_map_item_hr;
	}
}

## GObject based C class files
sub generate_files
{
	my ($data_hr) = @_;
	foreach my $c (sort keys %$data_hr)
	{
		my $parent = $data_hr->{$c}->{'__parent'};
		print "Evaluating Interface $c";
		if ($parent ne '') {
			print ": $parent";
		}
		print "\n";
		generate_class($c, $data_hr->{$c});
	}
	write_marshallers();
	write_header();
	write_makefile();
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

sub get_arg_type_info
{
	my ($type, $info) = @_;
	if (defined ($type_map->{$type}))
	{
		if (defined ($type_map->{$type}->{$info}))
		{
			return $type_map->{$type}->{$info};
		}
	}
}

sub get_return_type_val
{
	my ($rettype) = @_;
	my $fail_ret = get_arg_type_info($rettype, "fail_return");
	if (!defined($fail_ret) || $fail_ret eq "")
	{
		## Return NULL for pointer types
		if ($rettype =~ /\*$/ ||
			$rettype =~ /gpointer/)
		{
			$fail_ret = "NULL";
		}
		else
		{
			$fail_ret = "0";
		}
	}
	return $fail_ret;
}

sub get_arg_assert
{
	my ($rettype, $type_arg, $force) = @_;
	my ($type, $arg);
	if ($type_arg =~ s/([\w_][\w\d_]*)+$//)
	{
		$arg = $1;
		$type = $type_arg;
		$type =~ s/\s//g;
	}
	if (!defined($force) || $force eq "")
	{
		$force = 0;
	}
	my $ainfo = get_arg_type_info($type, "assert");
	if (!defined($ainfo) || $ainfo eq "")
	{
		my $saved_type = $type;
		$type =~ s/\*//g;
		
		## Check if it is registred non-class type
		foreach my $nc (keys %$not_classes)
		{
			if ($type eq $nc)
			{
				return "";
			}
		}
		# Correctly handle pointers to points (e.g AnjutaType** xy)
		if ($saved_type =~ /\*\*$/)
		{
			return "";
		}
		## Autodetect type assert
		if ($force ||
			(($saved_type =~ /\*$/) &&
			 ($type =~ /^Gtk/ ||
			  $type =~ /^Gdk/ ||
			  $type =~ /^Gnome/ || 
			  $type =~ /^Anjuta/ || 
			  $type =~ /^IAnjuta/)))
		{
			my $prefix;
			my $macro_prefix;
			my $macro_suffix;
			my $macro_name;
			get_canonical_names($type, \$prefix, \$macro_prefix,
								\$macro_suffix, \$macro_name);
			$ainfo = $macro_prefix."_IS_".$macro_suffix."(__arg__)";
		}
		else
		{
			## die "Cannot determine assert macro for type '$type'. Fix it first";
			return "";
		}
	}
	if ($rettype eq "void")
	{
		$ainfo =~ s/__arg__/$arg/;
		my $ret = "g_return_if_fail ($ainfo);";
		return $ret;
	}
	else
	{
		my $fail_ret = get_return_type_val ($rettype);
		if (defined($fail_ret))
		{
			$ainfo =~ s/__arg__/$arg/;
			my $ret = "g_return_val_if_fail ($ainfo, $fail_ret);";
			return $ret;
		}
	}
	die "Cannot determine failed return value for type '$rettype'. Fix it first";
}

sub construct_marshaller
{
	my ($rettype, $args) = @_;
	## Type mapping
	$rettype =~ s/\s//g;
	my $rtype = get_arg_type_info ($rettype, "gtype");
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
			if ($one_arg =~ s/([\w_][\w\d_]*)$//)
			{
				$one_arg =~ s/\s//g;
				my $arg_gtype = get_arg_type_info ($one_arg, "gtype");
				my $arg_type = get_arg_type_info ($one_arg, "type");
				
				if (!defined($arg_gtype) or $arg_gtype eq '')
				{
					die "Can not find GType for arg type '$one_arg'. Fix it first.";
				}
				if (!defined($arg_type) or $arg_type eq '')
				{
					$arg_type = $arg_gtype;
				}
				$args_list .= ",\n\t\t\t$arg_type";
				$arg_gtype =~ s/^G_TYPE_//;
				$arg_gtype =~ s/NONE/VOID/;
				$marshal_index .= "_$arg_gtype";
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
	$marshal_index = "${module_name}_iface_cclosure_marshal_$marshal_index";
	return $marshal_index.",\n\t\t\t".$args_list;
}

sub convert_ret
{
	my ($ret) = @_;
	
	if ($ret =~ /const List<.*>/)
	{
		return "const GList*";
	}
	elsif ($ret =~ /List<.*>/)
	{
		return "GList*";
	}
	else
	{
		return $ret;
	}
}

sub convert_args
{
	my ($args) = @_;
	my @argsv = split(',', $args);
	foreach my $arg (@argsv)
	{
		if ($arg =~ /const List<.*>.*/)
		{
			my @arg_name = split(' ', $arg);
			$arg = join(' ',"const GList*", $arg_name[-1]);
		}
		elsif ($arg =~ /.*List<.*>.*/)
		{
			my @arg_name = split(' ', $arg);
			$arg = join(' ', "GList*", $arg_name[-1]);
		}
	}
	$args = join(', ', @argsv);
	return $args;
}

sub enum_split
{
	(my $e) = @_;
	$e = substr($e, 0, 1) . "_" . substr($e, 1);
	return $e;
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
	my $parent_type = "G_TYPE_OBJECT";
	my $parent_include = "";
	if ($parent ne "")
	{
		my ($pprefix, $pmacro_name, $pmacro_prefix, $pmacro_suffix);
		$parent_iface = "${parent}Iface";
		
		get_canonical_names($parent, \$pprefix, \$pmacro_prefix,
							\$pmacro_suffix, \$pmacro_name);
		$parent_type = $pmacro_prefix."_TYPE_".$pmacro_suffix;
		$pprefix =~ s/_/-/g;
		$parent_include = $pprefix.".h";
	}
	
	## print "\tmethod prefix: $prefix\n";
	## print "\tmacro prefix: $macro_prefix\n";
	## print "\tmacro suffix: $macro_suffix\n";
	## print "\tmacro name: $macro_name\n\n";

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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _${macro_name}_H_
#define _${macro_name}_H_

";
	my $class_incs_lr = $class_hr->{"__include"};
	foreach my $inc (@$class_incs_lr)
	{
		$answer .= "$inc\n";
	}
	if ($parent_include ne "")
	{
		$answer .= "#include <$module_name/interfaces/$parent_include>\n";
	}

	$answer .=
"
G_BEGIN_DECLS

#define $macro_type (${prefix}_get_type ())
#define ${macro_name}(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), $macro_type, $class))
#define ${macro_assert}(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), $macro_type))
#define ${macro_name}_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), $macro_type, ${class}Iface))

";
	# Enum GType macros.
	my $enums_hr = $class_hr->{"__enums"};
	if (defined ($enums_hr))
	{
		foreach my $e (sort keys %$enums_hr)
		{
			# add an underscore if necassery
			my $se = $e;
			$se =~ s/[a-z][A-Z]/enum_split($&)/e;
			my $e_upper = uc($se);
			my $e_lower = lc($se);
			my $e_macro = "${macro_type}_${e_upper}";
			my $e_proto = "${prefix}_${e_lower}_get_type";
			$answer .=
"#define $e_macro ($e_proto())
";
		}
		$answer .= "\n";
	}
	
	$answer .=
"#define ${macro_name}_ERROR ${prefix}_error_quark()

";

	## Added defines;
	my $defines_lr = $class_hr->{"__defines"};
	if (defined ($defines_lr))
	{
		foreach my $d (@$defines_lr)
		{
			my $comments_out = $d->{'__comments'};
			my $name = $d->{'name'};
			my $value = $d->{'value'};
			$answer .= "${comments_out}#define\t${macro_name}_${name}\t${value}\n\n";
		}
		$answer .= "\n";
	}

	$answer .=
"typedef struct _$class $class;
typedef struct _${class}Iface ${class}Iface;\n\n";

	## Added enums;
	if (defined ($enums_hr))
	{
		foreach my $e (sort keys %$enums_hr)
		{
			my $comments_out = $enums_hr->{$e}->{'__comments'};
			$answer .= "${comments_out}typedef enum {\n";
			foreach my $d (@{$enums_hr->{$e}->{"__data"}})
			{
				$answer .= "\t${macro_name}_$d\n";
			}
			$answer .= "} ${class}$e;\n\n";
		}
	}
	## Added structs;
	my $structs_hr = $class_hr->{"__structs"};
	if (defined ($structs_hr))
	{
		foreach my $s (sort keys %$structs_hr)
		{
			my $comments_out = $structs_hr->{$s}->{'__comments'};
			$answer .= "${comments_out}typedef struct _${class}$s ${class}$s;\n";
			$answer .= "struct _${class}$s {\n";
			foreach my $d (@{$structs_hr->{$s}->{"__data"}})
			{
				$d = normalize_namespace ($class, $d);
				$answer .= "\t$d\n";
			}
			$answer .= "};\n\n";
		}
	}

	## Added Typedefs
	my $typedefs_lr = $class_hr->{"__typedefs"};
	if (defined ($typedefs_lr))
	{
		foreach my $td (@$typedefs_lr)
		{
			$td = normalize_namespace ($class, $td);
			$answer .= $td . "\n";
		}
		$answer .= "\n";
	}
	
	$answer .=
"
struct _${class}Iface {
	$parent_iface g_iface;
	
";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = normalize_namespace($class, $class_hr->{$m}->{'rettype'});
		my $args = normalize_namespace($class, $class_hr->{$m}->{'args'});
		$args = convert_args($args);
		if ($args ne '')
		{
			$args = ", ".$args;
		}
		if ($func =~ s/^\:\://)
		{
			$rettype = convert_ret($rettype);
			$answer .= "\t/* Signal */\n";
			$answer .= "\t$rettype (*$func) ($class *obj${args});\n";
		}
	}
	$answer .= "\n";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = normalize_namespace($class, $class_hr->{$m}->{'rettype'});
		my $args = normalize_namespace($class, $class_hr->{$m}->{'args'});
		$args = convert_args($args);
		if ($args ne '')
		{
			$args .= ", ";
		}
		if ($func !~ /^\:\:/)
		{
			$rettype = convert_ret($rettype);
			$answer .= "\t$rettype (*$func) ($class *obj, ${args}GError **err);\n";
		}
	}
	$answer .=
"
};
";
	# Enum GType prototypes.
	if (defined ($enums_hr))
	{
		foreach my $e (sort keys %$enums_hr)
		{
			# add an underscore if necassery
			my $se = $e;
			$se =~ s/[a-z][A-Z]/enum_split($&)/e;
			my $e_lower = lc($se);
			my $e_proto = "${prefix}_${e_lower}_get_type";
			$answer .=
"GType $e_proto (void);
";
		}
	}
	
	$answer .=
"
GQuark ${prefix}_error_quark     (void);
GType  ${prefix}_get_type        (void);

";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = normalize_namespace($class, $class_hr->{$m}->{'rettype'});
		my $args = normalize_namespace($class, $class_hr->{$m}->{'args'});
		$args = convert_args($args);
		if ($args ne '')
		{
			$args .= ", ";
		}
		if ($func !~ /^\:\:/)
		{
			$rettype = convert_ret($rettype);
			$answer .= "${rettype} ${prefix}_$func ($class *obj, ${args}GError **err);\n\n";
		}
	}
	$answer .=
"
G_END_DECLS

#endif
";
	write_file ($filename, $answer);
	push @header_files, $filename;
	
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

";
	# Added class (section) comments
	$answer .= $class_hr->{"__comments"};
	$answer .=
"
#include \"$headerfile\"
#include \"$module_name-iface-marshallers.h\"

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
		my $rettype = normalize_namespace($class, $class_hr->{$m}->{'rettype'});
		my $args = normalize_namespace($class, $class_hr->{$m}->{'args'});
		my $params = "";
		next if ($func =~ /^\:\:/);

		my $asserts = "\t".get_arg_assert($rettype, "$class *obj", 1)."\n";
		## self assert;
		$args = convert_args($args);
		if ($args ne '')
		{
			my @params;
			my @margs = split(",", $args);
			foreach my $one_arg (@margs)
			{
				my $assert_stmt = get_arg_assert($rettype, $one_arg, 0);
				if (defined($assert_stmt) && $assert_stmt ne "")
				{
					$asserts .= "\t$assert_stmt\n";
				}
				if ($one_arg =~ /([\w_][\w\d_]*)$/)
				{
					push @params, $1;
				}
			}
			$params = join (", ", @params);
			$params .= ", ";
		}
		
		$args = convert_args($args);
		if ($args ne '')
		{
			$args .= ", ";
		}
		my $comments_out = $class_hr->{$m}->{'__comments'};
		$rettype = convert_ret($rettype);
		$answer .= "${comments_out}${rettype}\n${prefix}_$func ($class *obj, ${args}GError **err)\n";
		$answer .= "{\n";
		if ($asserts ne "")
		{
			$answer .= $asserts;
		}
		$answer .= "\t";
		if ($rettype ne "void") {
			$answer .= "return ";
		}
		$answer .= "${macro_name}_GET_IFACE (obj)->$func (obj, ${params}err);";
		$answer .="\n}\n\n";

		## Default implementation
		$answer .= "/* Default implementation */\n";
		$answer .= "static ${rettype}\n${prefix}_${func}_default ($class *obj, ${args}GError **err)\n";
		$answer .= "{\n";
		if ($rettype ne "void") {
			$answer .= "\tg_return_val_if_reached (". get_return_type_val($rettype). ");";
		} else {
			$answer .= "\tg_return_if_reached ();";
		}
		$answer .="\n}\n\n";
	}
	$answer .=
"static void
${prefix}_base_init (${class}Iface* klass)
{
	static gboolean initialized = FALSE;

";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		next if ($func =~ /^\:\:/);
		$answer .= "\tklass->$func = ${prefix}_${func}_default;\n";
	}
	$answer .= "	
	if (!initialized) {
";
	foreach my $m (sort keys %$class_hr)
	{
		next if ($m =~ /^__/);
		my $func = $class_hr->{$m}->{'function'};
		my $rettype = normalize_namespace($class, $class_hr->{$m}->{'rettype'});
		my $args = normalize_namespace($class, $class_hr->{$m}->{'args'});
		$args = convert_args($args);
		if ($args ne '')
		{
			$args = ", ".$args;
		}
		if ($func =~ s/^\:\://)
		{
			my $signal = $func;
			$signal =~ s/_/-/g;
			
			my $marshaller = construct_marshaller($rettype, $args);
			$answer .= "\t\t/* Signal */";
			$answer .="\n\t\tg_signal_new (\"$signal\",
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
			(GBaseInitFunc) ${prefix}_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		type = g_type_register_static (G_TYPE_INTERFACE, \"$class\", &info, 0);
		g_type_interface_add_prerequisite (type, $parent_type);
	}
	return type;			
}
";
	# Enum GTypes.
	if (defined ($enums_hr))
	{
		foreach my $e (sort keys %$enums_hr)
		{
			# add an underscore if necassery
			my $se = $e;
			$se =~ s/[a-z][A-Z]/enum_split($&)/e;
			my $e_lower = lc($se);
			$answer .=
"
GType
${prefix}_${e_lower}_get_type (void)
{
	static const GEnumValue values[] =
	{
";
			foreach my $d (@{$enums_hr->{$e}->{"__data"}})
			{
				$d =~ s/^([\w_\d]+).*$/$1/;
				my $e_val = "${macro_name}_$d";
				my $e_val_str = lc($d);
				$e_val_str =~ s/_/-/g;
				$answer .= "\t\t{ $e_val, \"$e_val\", \"$e_val_str\" }, \n";
			}
			$answer .=
"		{ 0, NULL, NULL }
	};

	static GType type = 0;

	if (! type)
	{
		type = g_enum_register_static (\"${class}$e\", values);
	}

	return type;
}
";
		}
	}
	write_file ($filename, $answer);
	push @source_files, $filename;
}

sub write_marshallers
{
	## Write marshallers
	my $filename = "$module_name-iface-marshallers.list";
	push @header_files, "$module_name-iface-marshallers.h";
	push @source_files, "$module_name-iface-marshallers.c";
	my $contents = "";
	foreach my $m (sort keys %marshallers)
	{
		$m =~ s/__/\:/;
		$m =~ s/_/\,/g;
		$contents .= "$m\n";
	}
	if (write_file ($filename, $contents))
	{
		system "echo \"#include \\\"${module_name}-iface-marshallers.h\\\"\" ".
		"> xgen-gmc && glib-genmarshal --prefix=${module_name}_iface_cclosure_marshal ".
		"./${module_name}-iface-marshallers.list --body >> xgen-gmc && cp xgen-gmc ".
		"$module_name-iface-marshallers.c && rm -f xgen-gmc";
		system "glib-genmarshal --prefix=${module_name}_iface_cclosure_marshal ".
		"./${module_name}-iface-marshallers.list --header > xgen-gmc && cp xgen-gmc ".
		"$module_name-iface-marshallers.h && rm -f xgen-gmc";
	}
}

sub write_header
{
	my $iface_headers = "";
	my $answer = 
"
/* Interfaces global include file */

";
;
    foreach my $h (@header_files)
    {
    	$answer .= "#include \"$h\"\n";
	}
    write_file("libanjuta-interfaces.h", $answer);
    push(@header_files, "libanjuta-interfaces.h");
}

sub write_makefile
{
    my $iface_headers = "";
    foreach my $h (@header_files)
    {
	$iface_headers .= "\\\n\t$h";
    }
    my $iface_sources = "";
    foreach my $s (@source_files)
    {
	$iface_sources .= "\\\n\t$s";
    }
    
    my $iface_rules = "noinst_LTLIBRARIES = $module_name-interfaces.la\n";
    $iface_rules .= "${module_name}_interfaces_la_LIBADD = \$(MODULE_LIBS)\n";
##    $iface_rules .= "${module_name}_interfaces_la_LIBADD = \n";
    $iface_rules .= "${module_name}_interfaces_la_SOURCES = $iface_sources\n";
    $iface_rules .= "${module_name}_interfaces_includedir = \$(MODULE_INCLUDEDIR)\n";
    $iface_rules .= "${module_name}_interfaces_include = $iface_headers\n";
    
    my $contents = `cat Makefile.am.iface`;
    $contents =~ s/\@\@IFACE_RULES\@\@/$iface_rules/;
    my $filename = "Makefile.am";
	write_file ($filename, $contents);
}

sub write_file
{
	my ($filename, $contents) = @_;
    open (OUTFILE, ">$filename.tmp")
		or die "Can not open $filename.tmp for writing";
    print OUTFILE $contents;
    close OUTFILE;
	
	my $diff = "not matched";
	if (-e $filename)
	{
		$diff = `diff $filename $filename.tmp`;
	}
	unlink ("$filename.tmp");
	if ($diff !~ /^\s*$/)
	{
		print "\tWriting $filename\n";
		open (OUTFILE, ">$filename")
			or die "Can not open $filename for writing";
		print OUTFILE $contents;
		close OUTFILE;
		return 1;
	}
	return 0;
}
