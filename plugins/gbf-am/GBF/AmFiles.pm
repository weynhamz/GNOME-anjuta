package GBF::AmFiles;

use Exporter;
use strict;
use Fcntl ":seek";

our (@ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

@ISA = qw (Exporter);

@EXPORT = qw (&debug &enable_debug
	      &parse_am_file &output_am_file 
	      &parse_configure_in &parse_configure_in_buffer
	      &output_configure_in
	      &configure_rewrite_variable &configure_remove_variable
	      &configure_rewrite_packages
	      &configure_rewrite_package_info
	      &use_macro &use_rule &expand_one_var
	      &macro_create &macro_remove &macro_rewrite 
	      &macro_append_text &macro_prepend_text &macro_remove_text
	      &rule_remove &am_file_reset
	      &edit_config_files);
@EXPORT_OK = qw (&test_print_am_file &get_line_from_hint &get_config_files);
%EXPORT_TAGS = (DEBUG => [qw (&get_config_files &test_print_am_file)]);


use GBF::General;


my $debug = 0;

sub debug { my $message = $_[0]; print STDERR "DEBUG: $message\n" if $debug; }
sub enable_debug { $debug = 1; }

# +----------------
# | makefile              : information extracted from the parsing of Makefile.am
# +-----------------
# | filename              : full path to Makefile.am file
# | lines                 : list of lines of text of the file
# | macros                : hash of macro hashes (defined below) 
# |                         (macro_name => macro_hash)
# | rules                 : hash of rule hashes (defined below) (rule_name => rule_hash)
# | extra                 : list of other lines of text (i.e. not rules neither macros)
# | dirty                 : whether the makefile needs flushing
# | 
#
# +----------------
# | macro                 : a Makefile.am macro definition
# +-----------------
# | contents              : string with the contents of the macro
# | atline                : line number at the makefile.am where the macro is 
# |                         first defined
# | used                  : flag indicating whether the macro has already been used
# |                         in target extraction
# | 
#
# +----------------
# | rule                  : a Makefile.am rule definition
# +-----------------
# | dependencies          : string containing the dependencies of the rule
# | actions               : string containing the rule actions
# | atline                : line number at the makefile.am where the rule is defined
# | used                  : flag indicating whether the rule has already been used
# |                         in target extraction
# | 
#

######################################################################
# Constants and Variables
######################################################################

# String constants.
# These are stolen from automake

my $IGNORE_PATTERN = '^\s*##([^#\n].*)?\n';
my $WHITE_PATTERN = '^\s*' . "\$";
my $COMMENT_PATTERN = '^#';
my $TARGET_PATTERN='[$a-zA-Z_.@%][-.a-zA-Z0-9_(){}/$+@%]*';
# A rule has three parts: a list of targets, a list of dependencies,
# and optionally actions.
my $RULE_PATTERN =
  "^($TARGET_PATTERN(?:(?:\\\\\n|\\s)+$TARGET_PATTERN)*) *:([^=].*|)\$";

# Only recognize leading spaces, not leading tabs.  If we recognize
# leading tabs here then we need to make the reader smarter, because
# otherwise it will think rules like `foo=bar; \' are errors.
my $ASSIGNMENT_PATTERN = '^ *([^ \t=:+]*)\s*([:+]?)=\s*(.*)' . "\$";
# This pattern recognizes a Gnits version id and sets $1 if the
# release is an alpha release.  We also allow a suffix which can be
# used to extend the version number with a "fork" identifier.
my $GNITS_VERSION_PATTERN = '\d+\.\d+([a-z]|\.\d+)?(-[A-Za-z0-9]+)?';

my $IF_PATTERN = '^if\s+(!?)\s*([A-Za-z][A-Za-z0-9_]*)\s*(?:#.*)?' . "\$";
my $ELSE_PATTERN =
  '^else(?:\s+(!?)\s*([A-Za-z][A-Za-z0-9_]*))?\s*(?:#.*)?' . "\$";
my $ENDIF_PATTERN =
  '^endif(?:\s+(!?)\s*([A-Za-z][A-Za-z0-9_]*))?\s*(?:#.*)?' . "\$";
my $PATH_PATTERN = '(\w|[+/.-])+';
# This will pass through anything not of the prescribed form.
my $INCLUDE_PATTERN = ('^include\s+'
		       . '((\$\(top_srcdir\)/' . $PATH_PATTERN . ')'
		       . '|(\$\(srcdir\)/' . $PATH_PATTERN . ')'
		       . '|([^/\$]' . $PATH_PATTERN . '))\s*(#.*)?' . "\$");

# Match `-d' as a command-line argument in a string.
my $DASH_D_PATTERN = "(^|\\s)-d(\\s|\$)";
# Directories installed during 'install-exec' phase.
my $EXEC_DIR_PATTERN =
  '^(?:bin|sbin|libexec|sysconf|localstate|lib|pkglib|.*exec.*)' . "\$";

my %default_macro_style = ( bs_column          => -1,
			    items_per_line     => 1,
			    first_line_ignored => 1,
			    leading_space      => "\t" );


######################################################################
# Parser Functions
######################################################################

# &check_trailing_slash ( $LINE)
# --------------------------------------
# Return 1 iff $LINE ends with a slash.
# Might modify $LINE.
sub check_trailing_slash (\$)
{
  my ($line) = @_;

  # Ignore `##' lines.
  return 0 if $$line =~ /$IGNORE_PATTERN/o;

  # Catch and fix a common error.
  #msg "syntax", $where, "whitespace following trailing backslash"
  $$line =~ s/\\\s+\n$/\\\n/;

  return $$line =~ /\\$/;
}

# Function parse_file 

# Parses a Makefile.am file extracting the macros and rules in it, while saving
# its contents to an list of lines.

# FIXME: handle includes and conditionals

sub parse_am_file
{
    my $filename = $_[0];

    my (%macros, %rules, @extra_text);

    if (! open (MAKEFILE, "< $filename")) {
	&report_error (100, "Can't open file $filename: $!");
	return {};
    }


    use constant IN_VAR_DEF => 0;
    use constant IN_RULE_DEF => 1;
    use constant IN_COMMENT => 2;
    my $prev_state = IN_RULE_DEF;
    my $saw_bk = 0;
    my $current_rule = "";
    my @lines = ();
    my $rule_indentation = "";

    while (<MAKEFILE>) {
	push @lines, $_;

	if (/$IGNORE_PATTERN/o) {
	    # Merely delete comments beginning with two hashes.

	} elsif (/$WHITE_PATTERN/o) {
	    if ($saw_bk) {
		&report_error (101, "$filename:$.: blank line following " .
			       "trailing backslash");
		return {};
	    };

	} elsif (/$COMMENT_PATTERN/o) {
	    $prev_state = IN_COMMENT;
	} else {
	    last;
	}
	$saw_bk = check_trailing_slash ($_);
    }

    my $last_var_name = '';
    my $last_var_type = '';
    my $last_var_value = '';
    # FIXME: shouldn't use $_ in this loop; it is too big.
    while ($_) {
	# Make sure the line is \n-terminated.
	chomp;
	$_ .= "\n";

	my $new_saw_bk = check_trailing_slash ($_);

	if (/$IGNORE_PATTERN/o) {
	    # Merely delete comments beginning with two hashes.

	    # Keep any backslash from the previous line.
	    $new_saw_bk = $saw_bk;
	} elsif (/$WHITE_PATTERN/o) {
	    if ($saw_bk) {
		&report_error (101, "$filename:$.: blank line following " .
			       "trailing backslash");
		return {};
	    };

	} elsif (/$COMMENT_PATTERN/o) {
	    if ($saw_bk && $prev_state != IN_COMMENT) {
		&report_error (101, "$filename:$.: comment following " .
			       "trailing backslash");
		return {};
	    };
	    $prev_state = IN_COMMENT;
	} elsif ($saw_bk) {	    
	    if ($prev_state == IN_RULE_DEF) {
		if ($rules{$current_rule}{actions} eq "") {
		    # Strip trailing backslash from rule deps
		    s/\\\s*$//;
		    $rules{$current_rule}{dependencies} .= $_;
		} else {
		    $rules{$current_rule}{actions} .= $_;
		};
	    } elsif ($prev_state == IN_COMMENT) {
	    } else # $prev_state == IN_VAR_DEF
	    {
		# Chop newline and backslash if this line is
		# continued.  ensure trailing whitespace exists.
		s/\\\s*$//;		
		my $var_value = '';
		$var_value = ' '
		    unless $last_var_value =~ /\s$/;
		$var_value .= $_;
		$macros{$last_var_name}{contents} .= $var_value;
		$last_var_value .= $var_value;
	    }
	} elsif (/$IF_PATTERN/o) {
	    #$cond = cond_stack_if ($1, $2, $where);
	} elsif (/$ELSE_PATTERN/o) {
	    #$cond = cond_stack_else ($1, $2, $where);
	} elsif (/$ENDIF_PATTERN/o) {
	    #$cond = cond_stack_endif ($1, $2, $where);
	} elsif (/$RULE_PATTERN/o) {
	    # Found a rule.
	    $current_rule = $1;
	    &debug ("$filename:$.: Found rule '$current_rule'");
	    $prev_state = IN_RULE_DEF;
	    $rule_indentation = "";

	    my $value = $2;
	    if (defined ($value)) {
		$value =~ s/\\\s*$//;
	    } else {
		$value = "";
	    };
	    $rules{$current_rule} = { actions      => "",
				      dependencies => $value,
				      atline       => $.,
				      used         => 0 };
	} elsif (/$ASSIGNMENT_PATTERN/o) {
	    # Found a macro definition.
	    $prev_state = IN_VAR_DEF;
	    $last_var_name = $1;
	    $last_var_type = $2;
	    if ($3 ne '' && substr ($3, -1) eq "\\") {
		$last_var_value = substr ($3, 0, length ($3) - 1);
	    } else {
		$last_var_value = $3;
	    }

	    if ($2 ne '+') {
		$macros{$1} = { contents => $last_var_value,
				atline => $.,
				used => 0};
	    } else {
		# Add to the previously defined var
		if (defined ($macros{$1})) {
		    $macros{$1}{contents} .= ' ' . $last_var_value;
		} else {
		    &report_warning (100, "$filename:$.: Macro '$1' not " .
				     "previously defined");
		    $macros{$1} = { contents => $last_var_value,
				    atline => $.,
				    used => 0};
		}
	    };
	} elsif (/$INCLUDE_PATTERN/o) {
	    &debug ("$filename:$.: Found include, FIXME: handle it!");
        } else {
	    # This isn't an error; it is probably a continued rule.
	    # In fact, this is what we assume.

	    if (/^(\s+)/) {
		if ($rule_indentation eq "") {
		    $rule_indentation = $1;
		} elsif ($rule_indentation ne $1) {
		    &report_error (102, "$filename:$.: Incorrect indentation");
		    return {};
		};
	    } else {
		$prev_state = -1;
	    }

	    $last_var_name = "";
	    
	    if ($prev_state == IN_RULE_DEF) {
		$rules{$current_rule}{actions} .= $_;
	    } else {
		push @extra_text, $_;
	    };
	}

	$saw_bk = $new_saw_bk;
        $_ = <MAKEFILE>;
	push @lines, $_;
    };

    &report_error (100, "trailing backslash on last line")
	if $saw_bk;    
    
    close MAKEFILE;

    my $makefile = { filename => $filename,
		     lines    => \@lines,
		     macros   => \%macros,
		     rules    => \%rules,
		     extra    => \@extra_text,
		     dirty    => 0 };

    return $makefile;
}

sub read_entire_file ($)
{
    my $filename = shift;
    my $file_contents = "";
    my $file_offset = 0;
    my $bytes_read = 0;

    if (!open MYFILE, $filename) {
	report_error 100, "Can't open file $filename: $!";
	return "";
    };

    while ($bytes_read = read MYFILE, $file_contents, 4096, $file_offset) {
	$file_offset += $bytes_read;
    };
    close MYFILE;

    return $file_contents;
}

sub extract_syntactic_arguments ($$$$)
{
    my ($string, $starting, $opening, $closing) = @_;

    my $working_string = $string;
    # offset always refers to $working_string
    my $offset = 0;

    my @result = ();

    while ($working_string) {
	# start by searching the starting pattern
	if ($working_string =~ m/$starting/) {
	    # $balance keeps track of the paren count and starts at one because
	    # the opening paren must be included in the starting pattern
	    my $balance = 1;
	    # $index is the offset at which the starting pattern was found
	    my $index;

	    # chop $working_string up to the starting pattern
	    $working_string = substr $working_string, $+[0];
	    $offset += $+[0];
	    $index = $offset;

	    while ($working_string && $balance > 0) {
		# search the first occurrence of $opening, $closing 
		# or an escaping backslash
		my $found = ($working_string =~ m/($opening|$closing|\\)/);

		if ($found) {
		    my ($match, $start, $end) = ($1, $-[1], $+[1]);

		    if ($match eq "\\") {
			# just skip next character
			$end++;
		    }
		    elsif ($match =~ m/$opening/) {
			# increment paren balance
			$balance++;
		    }
		    else {
			# decrement paren balance
			$balance--;

			if ($balance == 0) {
			    my $result_length = $offset - $index + $start;
			    my $result_string = substr $string, $index, $result_length;
			    push @result, [ $index, $result_string ];
			};
		    };

		    # chop already processed portion of $working_string
		    $working_string = substr $working_string, $end;
		    $offset += $end;

		} else {
		    # unbalanced parentheses
		    $working_string = "";
		    last;
		};
	    };

	} else {
	    # no more matches
	    last;
	};
    };
    
    return @result;
}

sub get_config_files ($)
{
    my $contents = shift;

    my @tmp = &extract_syntactic_arguments ($contents, "AC_CONFIG_FILES\\(", "\\(", "\\)");
    @tmp = &extract_syntactic_arguments ($contents, "AC_OUTPUT\\(", "\\(", "\\)") if ! @tmp;

    if (@tmp) {
	# get first match...
        my ($offset, $tmp) = @{$tmp[0]};
	# and remove possibly enclosing square brackets
	($tmp) = split /\s*,\s*/, $tmp;
	if ($tmp =~ m/\s*\[ ( [^\]]+ ) \]/x) {
	    $tmp = $1;
	    $offset += $-[1];
	};
	return ($offset, $tmp);
    } else {
	return ();
    };
}

sub parse_configure_in_buffer
{
    my ($config_file, $contents) = @_;
    
    # get configured files, either from AC_OUTPUT or from AC_CONFIG_FILES
    my (undef, $output) = &get_config_files ($contents);

    # get substed vars
    my (%vars, %tmpvars, @vars_order);
    my @tmp = &extract_syntactic_arguments ($contents, "AC_SUBST\\(", "\\(", "\\)");
    # catch variable definitions
    while ($contents =~ m/(\w+)=(.+)$/mg) {
	$tmpvars{$1} = $2;
    };

    # Only store those AC_SUBSTed vars
    foreach my $tmp (@tmp) {
	my (undef, $varname) = @$tmp;
	if (defined ($tmpvars{$varname})) {
	    $vars{$varname} = $tmpvars{$varname};
	    push (@vars_order, $varname);
	};
    };
    
    # Process AC_OUTPUT
    my %other_files;
 
    if (!defined ($output)) {
	report_warning 101, "Missing AC_OUTPUT or AC_CONFIG_FILES";
	$output = "";
    };

  GENFILE: 
    foreach my $file (split /\s+/, &trim ($output)) {
	my ($gen, $input) = split /:/, $file;
	if (!defined ($input)) {
	    $input = "$gen.in";
	};
	
	# Skip makefiles
	if ($gen =~ /Makefile[^\/]*\z/) {
	    next GENFILE;
	};
	
	$other_files{$gen} = $input;
    };
    
    my $configure_in = { filename          => $config_file,
			 contents          => $contents,
			 other_files       => \%other_files,
			 vars              => \%vars,
			 vars_order        => \@vars_order,
			 dirty             => 0 };

    return $configure_in;
}

sub parse_configure_in ($)
{
    my $project_dir = shift;
    my $config_file;

    if (-e "$project_dir/configure.ac") {
	$config_file = "$project_dir/configure.ac"
    } elsif (-e "$project_dir/configure.in") {
	$config_file = "$project_dir/configure.in";
    } else {
	&report_error (100, "Neither configure.in nor configure.ac exist");
	return {};
    };

    my $contents = &read_entire_file ($config_file);
    if (! $contents) {
	return {};
    };
    return parse_configure_in_buffer ($config_file, $contents);
}

sub configure_rewrite_packages
{
    my ($configure_in, $module, $packages) = @_;

    $packages =~ s/,/ /sg;
    $packages =~ s/ +/ /sg;
    # Module is defined.
    if ($configure_in->{'contents'} =~ s/(PKG_CHECK_MODULES\([\s\[]*$module[\s\]]*,[\s\[]*)[^\,\)\]]+(.*?\))/$1$packages$2/sm) {
    } else {
	## Could not find any proper place to put the new variable.
	## Just putting it on top.
	$configure_in->{'contents'} =~ s/AC_OUTPUT/PKG_CHECK_MODULES($module, $packages)\nAC_OUTPUT/sm;
    }
    $configure_in->{"dirty"} = 1;
}

sub configure_rewrite_variable
{
    my ($configure_in, $var, $value) = @_;
    
    if ($configure_in->{'contents'} =~ /^\Q$var\E=/m) {
	$configure_in->{'contents'} =~
	    s/^\Q$var\E=.*$/$var=$value/m;
    } elsif ($configure_in->{'contents'} =~ /^AC_INIT\(.*?\)\s*$/m) {
	$configure_in->{'contents'} =~
	    s/^(AC_INIT\(.*?\))\s*$/$1\n\n$var=$value\nAC_SUBST($var)\n/m;
    } else {
	## Could not find any proper place to put the new variable.
	## Just putting it on top.
	$configure_in->{'contents'} =
	    "$var=$value\nAC_SUBST($var)\n". $configure_in->{'contents'};
    }
    $configure_in->{"dirty"} = 1;
}

sub configure_remove_variable
{
    my ($configure_in, $var) = @_;
    if ($configure_in->{'contents'} =~ /^\Q$var\E=/m)
    {
	$configure_in->{'contents'} =~
	    s/^\Q$var\E=.*$//m;
	$configure_in->{dirty} = 1;
    }
}

sub configure_rewrite_package_info
{
    my ($configure_in, $package_name, $package_version, $package_url) = @_;
    
    if ($package_url ne "")
    {
	$configure_in->{contents} =~ s/AC_INIT\(.*?\)/AC_INIT($package_name, $package_version, $package_url)/sm;
    }
    elsif ($package_version ne "")
    {
	$configure_in->{contents} =~ s/AC_INIT\(.*?\)/AC_INIT($package_name, $package_version)/sm;
    }
    elsif ($package_name ne "")
    {
	$configure_in->{contents} =~ s/AC_INIT\(.*?\)/AC_INIT($package_name)/sm;
    }    
    
    $configure_in->{dirty} = 1;
}

######################################################################
# Direct file modification functions
######################################################################

sub output_am_file
{
    my ($makefile, $new_file) = @_;

    rename ($new_file, "${new_file}.bak");

    open NEWFILE, "> $new_file" || 
	return &report_error (3, "Can't open $new_file for writing");;

    my $last_line = scalar (@{$makefile->{lines}});
    my $current_line = 1;
    foreach my $line (@{$makefile->{lines}}) {
	chomp $line;
	
	## last line is empty, do not output a newline together with it.
	if (($current_line == $last_line) && $line eq "") {
	    print NEWFILE $line;
	} else {
	    print NEWFILE $line, "\n";
	}
	$current_line++;
    }
    close NEWFILE;
    return 0;
}

sub output_configure_in ($)
{
    my ($configure_in) = @_;
    my $filename = $configure_in->{filename};

    rename ($filename, "${filename}.bak");

    open NEWFILE, "> $filename" || 
	return &report_error (3, "Can't open $filename for writing");;
    print NEWFILE $configure_in->{contents};
    close NEWFILE;

    $configure_in->{dirty} = 0;

    return 0;
}

sub file_insert_lines
{
    # Insert a line of text
    my $makefile = shift;
    my $at = shift;
    my @new_lines = @_;

    if (scalar @new_lines == 0) {
	return;
    }

    if ($at < 1 || $at > $#{$makefile->{lines}} + 2) {
	&report_warning (301, "Out of bounds line number $at.  " .
			 "Will insert at the end\n");
	$at = $#{$makefile->{lines}} + 2;
    };

    my $count = $#new_lines + 1;
    splice @{$makefile->{lines}}, $at - 1, 0, @new_lines;
    $makefile->{dirty} = 1;

    foreach my $macro (values %{$makefile->{macros}}) {
	$macro->{atline} += $count if ($macro->{atline} >= $at);
    };
    foreach my $rule (values %{$makefile->{rules}}) {
	$rule->{atline} += $count if ($rule->{atline} >= $at);
    };
}

sub file_remove_lines
{
    # Remove a line of text
    my ($makefile, $at, $count, $remove_trailing_blank) = @_;

    if ($count <= 0) {
	return;
    }

    if ($at < 1 || $at > $#{$makefile->{lines}} + 2) {
	&report_warning (301, "Out of bounds line number $at.  " .
			 "I will not remove it\n");
	return;
    };

    $_ = @{$makefile->{lines}}[$at + $count - 1];
    if ($at + $count < scalar @{$makefile->{lines}} && 
	/^\s+$/ && $remove_trailing_blank) {
	$count++;
    };
    &debug ("Removing lines from $at to " . ($at + $count));
    splice @{$makefile->{lines}}, $at - 1, $count;
    $makefile->{dirty} = 1;

    foreach my $macro (values %{$makefile->{macros}}) {
	$macro->{atline} -= $count if ($macro->{atline} > $at);
    };
    foreach my $rule (values %{$makefile->{rules}}) {
	$rule->{atline} -= $count if ($rule->{atline} > $at);
    };
}

sub get_line_from_hint
{
    # Return the nearest line to hint into which to insert contents
    my ($makefile, $hint) = @_;

    # Add at the end by default
    my $leading_blank = 0;
    my $trailing_blank = 0;

    my $total = $#{$makefile->{lines}} + 1;
    my $line = $total + 1;

  THINGS:
    foreach my $things ($makefile->{macros}, $makefile->{rules}) {
	foreach my $thing_name (keys %$things) {
	    if ($thing_name eq $hint) {
		$line = $things->{$thing_name}{atline} + 1;
		
		# Now find the next available line
		my $continued = $makefile->{lines}[$line - 2] =~ /\\$/;
		
		while ($line <= $total) {
		    $_ = $makefile->{lines}[$line - 1];
		    if (/\\$/) {
			# Line is continued, skip next too
			$continued = 1;
			$line++;
		    } elsif (/^\t/ || $continued) {
			# Last line was continued or line starts with a tab char, 
			# so it's a rule action
			$line++;
			$continued = 0;
		    } else {
			# An available line here
			# If it's blank, skip it to produce prettier output
			if (/^\s*$/) {
			    $line++;
			    $leading_blank = 1;
			};
			last;
		    };
		};
		last THINGS;
	    };
	};
    };

    if ($line < $total) {
	$trailing_blank = int ($makefile->{lines}[$line - 1] =~ /^\s+$/);
    };
		
    &debug ("Hint for $hint is ($line, $leading_blank, $trailing_blank)");
    return ($line, $leading_blank, $trailing_blank);
}


######################################################################
# Macro editing functions
######################################################################

sub last_macro
{
    # Returns last (in the text file) defined macro
    my $makefile = $_[0];

    my %macros = %{$makefile->{macros}};

    my $line = 0;
    my $macro_name = "";

    foreach my $tmp (keys %macros) {
	if (!$macro_name || $macros{$tmp}{atline} > $line) {
	    $macro_name = $tmp;
	    $line = $macros{$tmp}{atline};
	}
    }
    return $macro_name;
}

sub analyze_macro_style
{
    my ($makefile, $macro_name) = @_;
    
    my $macro = $makefile->{macros}{$macro_name};

    if (!defined $macro) {
	return (0, %default_macro_style);
    }

    # Analyze the current macro text and get the backslash column and the
    # items per line
    my $bs_column = 0;         # backslash column.
    my $items_per_line = 1;    # number of text elements per text line
    my $line = "";
    my $line_num = $macro->{atline} - 1;
    my $first_line = 1;
    my $first_line_ignored = 0;
    my $leading_space;

    # Iterate over the continued lines
    do {
	$line_num++;
	$line = @{$makefile->{lines}}[$line_num - 1];
	
	my $line_copy = &expand_tabs ($line);
	my $bs = $bs_column;
	
	if ($line_copy =~ /\\\s*$/) {
	    # first get the column of the backslash
	    $bs = rindex $line_copy, '\\';
	};
	
	if ($first_line) {
	    $line =~ /$ASSIGNMENT_PATTERN/;
	    $line_copy = &empty_if_undef ($3);
	} elsif (!$leading_space) {
	    $line =~ /^(\s+)\S+/;
	    $leading_space = $1;
	};
	$line_copy =~ s/\\\s*$//;
	
	my @items = split /\s+/, &trim ($line_copy);
	
	if ($first_line && scalar @items == 0) {
	    $first_line_ignored = 1;
	};

	# get the maximum items per line seen
	if (scalar @items + 1 > $items_per_line) {
	    $items_per_line = scalar @items;
	};
	
	if ((!$first_line && $bs_column == 0) || ($first_line && $first_line_ignored)) {
	    # initialize bs_column
	    $bs_column = $bs;
	} elsif ($bs_column != 0 && $bs != $bs_column) {
	    # bs column changed, get amount of separation
	    chomp $line_copy;
	    if ($line_copy =~ s/.+[^ ](\s+)\z/$1/) {
		$bs_column = - length ($line_copy);
	    } else {
		## Line has zero spacing. So use 1 instead.
		$bs_column = -1;
	    };
	};
	
	$first_line = 0;
    } while ($line =~ /\\\s*$/);
    
    &debug ("Macro: $macro_name, " .
	    "bs_column = $bs_column, items_per_line = $items_per_line, ".
	    "last_line = $line_num, ignore_first = $first_line_ignored");

    my %macro_style = ( bs_column          => $bs_column,
			items_per_line     => $items_per_line,
			first_line_ignored => $first_line_ignored,
			leading_space      => $leading_space );

    return ($line_num, \%macro_style);
}

sub format_macro
{
    my ($line, $text_to_add, $macro_style, $add_trailing_bs) = @_;

    my (@result, $one_line, $item_count, @items);

    @items = split /\s+/, &trim ($text_to_add);

    if (!defined ($macro_style)) {
	$macro_style = \%default_macro_style;
    };

    my $leading_space = $macro_style->{leading_space};
    my $bs_column = $macro_style->{bs_column};

    if (!defined ($leading_space)) {
	$leading_space = "\t";
    }

    if ($line =~ /$ASSIGNMENT_PATTERN/) {
	my @current_items = split /\s+/, &trim (&empty_if_undef ($3));
	$item_count = scalar @current_items;

	if (!$bs_column) {
	    if ($item_count == 1) {
		$bs_column = -1;
	    } else {
		$bs_column = $default_macro_style{bs_column};
	    };
	};
	my $line_copy = &expand_tabs ($line);
	if ($bs_column > 0 && length ($line_copy) + 1 > $bs_column) {
	    $bs_column = length ($line_copy) + 1;
	};

    } else {
	# Extract leading space
	$line =~ /^(\s*)\S+/;
	$leading_space = $1;
	
	# count items already in 
	my @current_items = split /\s+/, &trim ($line);
	$item_count = scalar @current_items;
    }

    my $items_per_line = $macro_style->{items_per_line};
    if (!$items_per_line) {
	if ($bs_column < 0) {
	    $items_per_line = 1;
	} else {
	    my $avg_width = 0;
	    foreach (@items) {
		$avg_width += length ($_);
	    };
	    $avg_width /= ($#items + 1);
	    # Wild guess here... could be adjusted
	    $items_per_line = int ($bs_column * .75 / $avg_width);
	};
    };

    $one_line = $line;
    $one_line =~ s/\s+$/ /;
    my $max_width = ($bs_column > 0 ? $bs_column : 70);  # FIXME: don't hardcode
    
    &debug ("Formatting '$one_line' which already has $item_count items, ".
	    "bs_column = $bs_column, items_per_line = $items_per_line");

    my $ignore_first_line = $macro_style->{first_line_ignored};

    foreach my $item (@items) {
	my $line_copy = &expand_tabs ($one_line);

	# if items_per_line is more than 2 we assume the user wants to fill in
	# the space until the bs column (or max_width,
	# but normally, when bs_column < 0, items_per_line will be 1)
	if (($items_per_line <= 2 && $item_count >= $items_per_line) || 
	    length ($line_copy) + length ($item) > $max_width ||
	    $ignore_first_line) {
	    
	    $ignore_first_line = 0;
	    # close this line
	    $one_line =~ s/\s+$//;
	    if ($bs_column > 0) {
		# we need the bs at a fixed column
		$one_line .= " " x ($bs_column - length ($line_copy));
	    } else {
		$one_line .= " " x (- $bs_column);
	    }
	    $one_line .= "\\\n";
	    push @result, $one_line;
	    $one_line = $leading_space;
	    $item_count = 0;
	}
	$one_line .= ($item_count ? ' ' : '' ) . "$item";
	$item_count++;
    };
    # Add last line
    if ($add_trailing_bs) {
	my $line_copy = &expand_tabs ($one_line);

	# close this line
	$one_line =~ s/\s+$//;
	if ($bs_column > 0) {
	    # we need the bs at a fixed column
	    $one_line .= " " x ($bs_column - length ($line_copy));
	} else {
	    $one_line .= " " x (- $bs_column);
	}
	$one_line .= "\\\n";

    } else {
	$one_line .= "\n";
    }

    push @result, $one_line;

    return @result;
}

sub macro_prepend_text
{
    # Prepends some text to the macro
    my ($makefile, $macro_name, $text) = @_;

    my $macro = $makefile->{macros}{$macro_name};
    if (!defined ($macro)) {
	# If the macro doesn't exist create it
	&macro_create ($makefile, $macro_name, $text, &last_macro ($makefile));

    } elsif ($macro->{atline} > 0) {
	my ($last_line_num, $macro_style) = 
	    &analyze_macro_style ($makefile, $macro_name);
	my $first_line_num = $macro->{atline};
	my $first_line = @{$makefile->{lines}}[$first_line_num - 1];

	$first_line =~ /$ASSIGNMENT_PATTERN/;
	my $leading_text = $3;
	$text .= " " . $leading_text;
	$text =~ s/\\\s*$//;
	$first_line = substr $first_line, 0, -length ($leading_text) - 1;

	my @lines = &format_macro ($first_line, $text, $macro_style, 
				   ($last_line_num != $first_line_num));

	$first_line = shift @lines;
	@{$makefile->{lines}}[$first_line_num - 1] = $first_line;
	$makefile->{dirty} = 1;
	&file_insert_lines ($makefile, $first_line_num + 1, @lines);

	# FIXME: this is not right, since maybe $text doesn't have backslashes 
	# and newlines
	$macro->{contents} = $text . " " . $macro->{contents};
    }
}

sub macro_append_text
{
    # Appends some text to the macro
    my ($makefile, $macro_name, $text) = @_;

    my $macro = $makefile->{macros}{$macro_name};
    if (!defined ($macro)) {
	# If the macro doesn't exist create it
	&macro_create ($makefile, $macro_name, $text, &last_macro ($makefile));

    } elsif ($macro->{atline} > 0) {
	my ($last_line_num, $macro_style) =
	    &analyze_macro_style ($makefile, $macro_name);
	my $last_line = @{$makefile->{lines}}[$last_line_num - 1];
	my @lines = &format_macro ($last_line, $text, $macro_style, 0);

	$last_line = shift @lines;
	@{$makefile->{lines}}[$last_line_num - 1] = $last_line;
	$makefile->{dirty} = 1;
	&file_insert_lines ($makefile, $last_line_num + 1, @lines);

	# FIXME: this is not right, since maybe $text doesn't have backslashes 
	# and newlines
	$macro->{contents} .= " $text";
    }
}

sub macro_create
{
    # hint is a macro name.  This macro should be created around it.
    my ($makefile, $macro_name, $initial, $hint_near) = @_;

    my $macros = $makefile->{macros};
    if (!$hint_near) {
	$hint_near = &last_macro ($makefile);
    };

    my ($line, $lead, $trail) = &get_line_from_hint ($makefile, $hint_near);

    if (defined ($macros->{$macro_name})) {
	$line = $macros->{$macro_name}{atline};
	&report_warning (300, "Macro '$macro_name' already exists.  It will be deleted");
	&macro_remove ($makefile, $macro_name);
	$lead = 1;
	$trail = 0;
    };
    
    &debug ("macro_append: Using line $line");

    &file_insert_lines ($makefile, $line, "\n") if (!$trail);
    &file_insert_lines ($makefile, $line, 
			&format_macro ("$macro_name = ", $initial));
    &file_insert_lines ($makefile, $line++, "\n") if !$lead;
    
    # FIXME: this is not right, since maybe $initial doesn't have backslashes 
    # and newlines
    my %new_macro = ( contents => $initial,
		      atline   => $line,
		      used     => 0 );
    $macros->{$macro_name} = \%new_macro;
}

sub macro_remove
{
    my $makefile = shift;

    while (my $macro_name = shift) {
	my $macro = $makefile->{macros}{$macro_name};
	if (!defined ($macro)) {
	    next;
	}

	my $line = $macro->{atline};
	if ($line > 0) {
	    &debug ("Removing macro $macro_name at line $line");
	    
	    my $count = 1;
	    while (@{$makefile->{lines}}[$line + $count - 2] =~ /\\\s*$/) {
		$count++;
	    };
	    
	    &file_remove_lines ($makefile, $line, $count, 1);
	    
	    delete $makefile->{macros}{$macro_name};
	};
    }
}

sub macro_rewrite
{
    # Rewrites the macro with the new text, but trying to accomodate to the
    # previous style
    my ($makefile, $macro_name, $text, $hint_near) = @_;

    my $macro = $makefile->{macros}{$macro_name};
    if (!defined ($macro)) {
    	if ($hint_near eq "") {
	    $hint_near = &last_macro ($makefile);
	}
	# If the macro doesn't exist create it
	&macro_create ($makefile, $macro_name, $text, $hint_near);

    } elsif ($macro->{atline} > 0) {
	my ($last_line_num, $macro_style) =
	    &analyze_macro_style ($makefile, $macro_name);
	my $first_line_num = $macro->{atline};

	my @lines = &format_macro ("$macro_name = ", $text, $macro_style, 0);

	&file_remove_lines ($makefile, $first_line_num, 
			    $last_line_num - $first_line_num + 1);
	&file_insert_lines ($makefile, $first_line_num, @lines);

	# FIXME: this is not right, since maybe $text doesn't have backslashes 
	# and newlines
	$macro->{contents} = $text;
    }
}

sub macro_remove_text
{
    # Removes some text from the macro definition
    my ($makefile, $macro_name, $remove_text, $remove_macro_if_empty) = @_;

    my $macro = $makefile->{macros}{$macro_name};
    if (!defined ($macro) || $macro->{atline} <= 0) {
	return;
    };

    $macro->{contents} =~ s/$remove_text//;
    if (&trim ($macro->{contents}) eq "" && $remove_macro_if_empty) {
	&macro_remove ($makefile, $macro_name);
	return;
    }

    my ($last_line_num, $macro_style) = &analyze_macro_style ($makefile, $macro_name);
    my $line_num = $macro->{atline};
    while ($line_num <= $last_line_num) {
	my $line = @{$makefile->{lines}}[$line_num - 1];
	
	if ($line =~ /(\s|=)$remove_text\s/) {
	    &debug ("found the text to remove at line $line_num");
	    # Found the text
	    # Check if it's the only thing in the line
	    if ($line =~ /^\s+$remove_text\s*\\?$/) {
		&file_remove_lines ($makefile, $line_num, 1);
		if ($line_num == $last_line_num) {
		    # remove the trailing bs of the previous line
		    $_ = @{$makefile->{lines}}[$line_num - 2];
		    chomp; chop;
		    @{$makefile->{lines}}[$line_num - 2] = $_ . "\n";
		    $makefile->{dirty} = 1;
		}
	    } else {
		# else, just remove the text, the trailing bs and reformat the line
		$line =~ s/$remove_text\s//;
		$line =~ s/\\\s*$//;
		if ($line !~ /^\s+/ && $line_num > $macro->{atline}) {
		    $line = $macro_style->{leading_space} . $line;
		};
		my @lines = &format_macro ($line, "", $macro_style,
					   ($line_num != $last_line_num));
		@{$makefile->{lines}}[$line_num - 1] = shift @lines;
		$makefile->{dirty} = 1;
		if (@lines) {
		    &file_insert_lines ($makefile, $line_num + 1, @lines);
		}
	    }
	    last;
	};
	$line_num++;
    }
}

sub test_print_am_file
{
    my ($makefile, $message) = @_;

    print "MAKEFILE CONTENTS $message\n" . "-" x 79 . "\n";
    my $number = 1;
    foreach my $line (@{$makefile->{lines}}) {
	print "$number: $line";
	$number++;
    };
}


######################################################################
# Rule editing functions
######################################################################

sub rule_remove
{
    my $makefile = shift;

    while (my $rule_name = shift) {
	my $rule = $makefile->{rules}{$rule_name};
	if (!defined ($rule)) {
	    next;
	}

	my $line = $rule->{atline};
	if ($line > 0) {
	    my $count = 1;
	    my $maxcount = scalar @{$makefile->{lines}} - $line;
	    # count dependency lines
	    while ($count < $maxcount && 
		   @{$makefile->{lines}}[$line + $count - 2] =~ /\\\s*$/) {
		$count++;
	    };
	    # count action lines
	    while ($count <= $maxcount &&
		   @{$makefile->{lines}}[$line + $count - 1] =~ /^\t/) {
		$count++;
	    };
	    
	    &debug ("Removing rule $rule_name at line $line");
	    
	    &file_remove_lines ($makefile, $line, $count, 1);
	    
	    delete $makefile->{rules}{$rule_name};
	};
    }
}


######################################################################
# Text editing functions
######################################################################

sub beginning_of_line ($$)
{
    my ($string, $offset) = @_;

    # this automagically handles first lines, 
    # since rindex will return -1 in that case
    return rindex ($string, "\n", $offset - 1) + 1;
}

sub next_line_offset ($$)
{
    my ($string, $offset) = @_;

    my $next_line = index ($string, "\n", $offset) + 1;
    # make correction if at end of string and no terminating newline
    $next_line = length $string if $next_line <= $offset;

    return $next_line;
}

sub get_line_at_offset ($$)
{
    my ($string, $offset) = @_;

    my $line_begin = beginning_of_line $string, $offset;
    my $line_end = next_line_offset $string, $offset;

    return substr $string, $line_begin, ($line_end - $line_begin);
}

sub stat_sample (@)
{
    my ($sum, $count, $max, $i) = (0,0,0,0);
    my ($most_freq, $count_most_freq) = (0,0);
    foreach my $data (@_) {
	$sum += $i * $data;
	$count += $data;
	$max = $i if $data > 0;
	if ($data >= $count_most_freq) {
	    ($most_freq, $count_most_freq) = ($i, $data);
	};
	$i++;
    };
    return ($max, int ($sum / $count), $most_freq, $count_most_freq / $count);
}

# return values:
#  0 means unordered
# +1 means descending
# -1 means ascending
sub analyze_order (@)
{
    my $order = undef;
    my $last = shift;
    foreach my $item (@_) {
	my $current = $last cmp $item;
	if ($current != 0) {
	    if (!defined $order) {
		$order = $current;
	    } elsif ($current != $order) {
		$order = 0;
		last;
	    };
	};
	$last = $item;
    };
    return $order;
}

sub find_best_match ($$@)
{
    my $order = shift;
    my $ref = shift;
    my ($winner, $score) = (undef, 0);

    foreach my $item (@_) {
	my $this_score = 0;
	if ($order == 0) {
	    # find the nearest string
	    while (substr ($ref, $this_score, 1) eq substr ($item, $this_score, 1) &&
		   $this_score < length $ref) {
		$this_score++;
	    };
	} elsif ($order > 0) {
	    $this_score = $item cmp $ref;
	} else {
	    $this_score = $ref cmp $item;
	};

	if ($this_score >= $score) {
	    ($winner, $score) = ($item, $this_score);
	};
    };
    return $winner;
}

sub analyze_text ($$$)
{
    my ($string, $offset, $length) = @_;

    # variables to keep information
    my ($ignore_first_line, $ignore_last_line) = (0,0);
    my (@widths, @left_margins, @right_margins, @item_counts);
    my $backslashed = 0;

    # acquire data to analyze
    my $inner_text = substr $string, $offset, $length;
    my $first_line_offset = beginning_of_line $string, $offset;
    my $outer_text = substr ($string, $first_line_offset,
			     next_line_offset ($string, $offset + $length) 
			     - $first_line_offset);

    my @lines = split "\n", $outer_text;
    my @inner_lines = split "\n", $inner_text;

    $ignore_first_line = 1 if trim (chop_backslash ($inner_lines[0])) eq "";
    $ignore_last_line = 1 if trim (chop_backslash ($inner_lines[@inner_lines - 1])) eq "";

    my ($line_count, $line_number) = (0, 0);
    foreach my $line (@lines) {
	my $expanded_line = expand_tabs $line;
	my $line_length = length $expanded_line;

	# skip ignored lines
	next if $line_number == 0 && $ignore_first_line;
	next if $line_number == @lines - 1 && $ignore_last_line;

	# sample data
	$widths[$line_length]++;
	$backslashed = 1 if $expanded_line =~ m/\\\s*$/;
	$expanded_line = chop_backslash $expanded_line;
	if ($line_number != 0 && $line_number != @lines - 1) {
	    $expanded_line =~ m/^(\s*)\S.*\S(\s*)$/;
	    $left_margins[length $1]++;
	    $right_margins[length $2]++;
	};

	my @items = split /\s+/, trim $expanded_line;
	$item_counts[@items]++;

	# finish this line
	$line_count++;

    } continue {	
	$line_number++;
    };

    # get information
    my ($max_width, $avg_width, $width, $width_prop) = stat_sample @widths;
    my (undef, undef, $left_margin) = stat_sample @left_margins;
    my (undef, undef, $right_margin) = stat_sample @right_margins;
    my ($item_count) = stat_sample @item_counts;

    # determine if text is somehow ordered
    my @items = split /\s+/, trim $inner_text;
    my $order = analyze_order @items;

#     print "max_width => $max_width, avg_width => $avg_width, width => ".
# 	($width_prop > .75 ? $width : -1) .", ignore_first_line => $ignore_first_line, ".
# 	"ignore_last_line => $ignore_last_line, left_margin => $left_margin, ".
# 	"right_margin => $right_margin, items_per_line => $item_count, ".
# 	"order => $order, backslashed => $backslashed\n";

    # width = -1 means every line's width is just what's needed for that line
    # and in that case, attention is taken to right_margin
    # otherwise, it indicates the desired width for any new line 
    # and the right_margin adjusts to fit such width
    return { max_width => $max_width,
	     avg_width => $avg_width,
	     width => $width_prop > .75 ? $width : -1,
	     ignore_first_line => $ignore_first_line,
	     ignore_last_line => $ignore_last_line,
	     left_margin => $left_margin,
	     right_margin => $right_margin,
	     items_per_line => $item_count,
	     order => $order,
	     backslashed => $backslashed };
}

# FIXME: probably format_macro can be replaced with this new (possibly
# better thought) function
sub format_line ($$$)
{
    my ($line, $style, $start_column) = @_;

    $start_column = 0 if !defined $start_column;
    my $expanded_line = expand_tabs $line;
    my @line_items = split /\s+/, trim (chop_backslash $line);
    my $item_count = scalar @line_items;
    my $width = $style->{width} > 0 ? $style->{width} : $style->{max_width};

#     print "Formatting $line";

    if ($item_count > 1 && 
	(length ($expanded_line) + $start_column > $width ||
	 $item_count > $style->{items_per_line})) {
	
	my $rest = $line;
	my $new_line = "";
	my @rest;
	my ($at, $chunk);

	# try to split the line as close to $style->{width} as possible
	$item_count = 0;
	$rest =~ m/^(\s*\S+)/;
	$chunk = $1;
	$at = $+[1];
	do {
	    # add the chunk
	    $new_line .= "$chunk ";
	    $item_count++;
	    $rest = substr $rest, $at;
	    
	    # match next chunk
	    $rest =~ m/^\s*(\S+)/;
	    $chunk = $1;
	    $at = $+[1];
	} while (length (expand_tabs ("$new_line $chunk")) + $start_column < $width &&
		 $item_count < $style->{items_per_line});

	# format the reduced line
	$new_line = &format_line ("$new_line\n", $style, $start_column);

	# correct left margin in $rest
	$rest =~ s/^\s*//;
	$rest = (" " x $style->{left_margin}) . $rest;

	# recurse on $rest, which should still be a single line
	$line = $new_line . &format_line ($rest, $style, 0);
	
    } else {
	# line fits and has a valid number of items... just close it
	# remove trailing space
	$line = chop_backslash $line;
	$line =~ s/\s*$//;
	
	if ($style->{width} > 0) {
	    # close line at a fixed length
	    $line .= " " x (length (expand_tabs $line) - $style->{width});
	} else {
	    $line .= " " x $style->{right_margin};
	};
	if ($style->{backslashed}) {
	    $line .= "\\";
	};
	$line .= "\n";

    };

    return $line;
}

sub insert_text ($$$$)
{
    my ($string, $start_column, $new_text, $style) = @_;
    my $result = "";

    # first of all we need to find a place to put the new text on
    my @string_items = split /\s+/, trim $string;
    my $insert_offset;
    my $reference = find_best_match $style->{order}, $new_text, @string_items;

    if ($reference) {
	my $tmp = " $string ";
	while ($tmp =~ m/\s+(\Q$reference\E)\s+/g) {
	    $insert_offset = $+[1];
	};

    } else {
	# append the text to the string if we couldn't find a reference
	$insert_offset = beginning_of_line $string, length ($string);
	$insert_offset = beginning_of_line $string, $insert_offset - 1 
	    if $style->{ignore_last_line};
    };

    my ($before, $after) = (substr ($string, 0, $insert_offset),
			    substr ($string, $insert_offset));

    # chop spaces at both ends
    $before =~ s/\s*$//;
    $after =~ s/^\s*//;

    $result = "$before $new_text $after";

    my $start_of_line_to_format = beginning_of_line $result, length $before;
    my $line_to_format = get_line_at_offset $result, length $before;
    my $formatted_line = format_line ($line_to_format, $style, 
				      $start_of_line_to_format == 0 ? 
				      $start_column : 0);

    # replace the formatted line
    substr $result, $start_of_line_to_format, length $line_to_format, $formatted_line;

    return $result;
}


######################################################################
# configure.in file editing functions
######################################################################

sub get_column ($$)
{
    my ($string, $offset) = @_;

    my $column0 = beginning_of_line $string, $offset;
    return length expand_tabs substr $string, $column0, ($offset - $column0);
}

sub edit_config_files ($$$)
{
    my ($configure_in, $action, $operand) = @_;
    my ($offset, $config_files) = get_config_files $configure_in->{contents};

    return unless defined $config_files;

    # get current style
    my $style = analyze_text $configure_in->{contents}, $offset, length $config_files;
    my $new_config_files = $config_files;
    my $start_column = get_column $configure_in->{contents}, $offset;

    if ($action eq "add") {
	$new_config_files = insert_text ($config_files, $start_column,
					 $operand, $style);

    } elsif ($action eq "remove") {
	$new_config_files =~ s/\Q$operand\E\s*//g;
    };

    # save result string
    if ($new_config_files ne $config_files) {
	substr $configure_in->{contents}, $offset, length $config_files, $new_config_files;
	$configure_in->{dirty} = 1;
    };
}


######################################################################
# General Functions
######################################################################

sub am_file_reset
{
    my $makefile = shift;
    foreach my $macro (values %{$makefile->{macros}}) {
	$macro->{used} = 0;
    }
    foreach my $rule (values %{$makefile->{rules}}) {
	$rule->{used} = 0;
    }
}

sub use_macro
{
    my $macro = $_[0];
    if (defined ($macro)) {
	$macro->{used} = 1;
	return &trim ($macro->{contents});
    } else {
	return undef ();
    };
}

sub use_rule
{
    my $rule = $_[0];
    if (defined ($rule)) {
	$rule->{used} = 1;
	return (&trim ($rule->{dependencies}), $rule->{actions});
    } else {
	return (undef, undef);
    };
}

sub expand_one_var
{
    my ($source, %expansion_set) = @_;
    my ($var_name, $repl);

    # Expand normal macros
    while ($source =~ /\$[({](\w*)[)}]/) {
	$var_name = $1;
	$repl = &empty_if_undef ($expansion_set{$var_name}{contents});
	$source =~ s/\$[({]$var_name[)}]/$repl/;
    }

    # Expand substitution macros $(macro:search=replace)
    while ($source =~ /\$\((\w*):([^=]+)=([^\)]*)\)/) {
	$var_name = $1;
	my ($search, $replace) = ($2, $3);

	$repl = &empty_if_undef ($expansion_set{$var_name}{contents});
	$repl =~ s/$search/$replace/g;

	# Set sources list
	$source =~ s/\$\($var_name:([^=]+)=([^\)]*)\)/$repl/;
    }
    return $source;
}

1;
