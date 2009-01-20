#!/usr/bin/perl -w
# 
# Copyright (C) 2008 Sébastien Granjoux  <seb.sfo@free.fr>
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
 
# This program read all *.properties file containing style defintion and copy
# their content in the output file with all style definitions at the end.
# You are supposed to edit all the style definition to use properties defined
# by Anjuta. If the output file already exist, the style definition already
# existing are kept.
#
# The -stat options allow to see when each anjuta style is used.

use strict;
use File::Copy;
use File::Basename;
use Getopt::Long;

my %anjuta_defs;

my %anjuta_stat;

# Get command line argument
my $stat = "";

GetOptions ("stat" => \$stat);

my $output_file = $ARGV[0];
my $prop_directory = $ARGV[1];

if (!defined($prop_directory))
{
	die "Usage: update-properties.pl [-stat] output_properties_file input_directory";
}


# Read old definition, copy all file
%anjuta_defs = read_style_definition ($output_file);
copy($output_file, $output_file.".bak");
if ($stat) {%anjuta_stat = create_stat_hash (\%anjuta_defs);}

# Open output file for writing
open (OUTFILE, ">$output_file") or die "Unable to open file $output_file for writing\n";
print OUTFILE <<EOF;
###############################################################################
#
# Created by update-properties.pl from all SciTE properties files
#
# This file is divided in 2 parts:
#   * The first part concatenate all SciTE *.properties file containing style
#     definition but without these definitions. 
#   * The second part group all style definitions. The style definition should
#     be modified to use anjuta style properties. If the output file already
#     exists the second part is read before overwriting it, so you can keep
#     any change done here.
EOF

# Read properties in new file
opendir (PROPDIR, $prop_directory) or die "Unable to open directory $prop_directory\n";
foreach my $filename (sort readdir(PROPDIR))
{
	if (($filename ne "Embedded.properties") &&
	    ($filename ne "anjuta.properties") &&
	    ($filename ne basename ($output_file)) &&
	    ($filename =~ /\w+\.properties/))
	{
		my %defs;

		%defs = read_style_definition ($prop_directory."/".$filename);

		if (%defs)
		{
			copy_non_style_definition (\*OUTFILE, $prop_directory."/".$filename);

			# Gather some stats
			if ($stat) {gather_stat (\%anjuta_stat, \%anjuta_defs, \%defs);}

			# Merge with anjuta propertes
			merge_style_definition (\%anjuta_defs, \%defs);
		}
	}
}
closedir(PROPDIR);

# Write style values
print OUTFILE "\n\n\n\n";

# Write languages style definition
for my $lang (sort keys %anjuta_defs)
{
	write_style_definition (\*OUTFILE, $anjuta_defs{$lang}, $lang);
}

close(OUTFILE);

if ($stat) {display_stat (\%anjuta_stat);}



# Read a properties file and keep only style definition
sub read_style_definition
{ 
	my ($filename) = @_;

	my %defs = ();
	my $last_comment = "";
	my $line = "";

	if (open (INFILE, "<$filename"))
	{
		while ($line = <INFILE>)
		{
			if ($line =~ /^\s*#\s*style\.\w+\.\d+\s*=.+$/)
			{
				# Style put in comment, just discard
				;
			}
			elsif ($line =~ /^\s*#(.+)$/)
			{
				# Comment keep the last one
				$last_comment = $1;
			}
			elsif ($line =~ /^\s*style\.([\w*]+)\.(\d+)\s*=\s*(.+)\s*$/)
			{
				# Style line add it into the hash
				if (($1 ne '*') && 
				    ($2 ne "32") && 
				    ($2 ne "33") && 
 				    ($2 ne "34") &&
				    ($2 ne "35") &&
 				    ($2 ne "36") &&
				    ($2 ne "37"))
				{
					# Keep only language style
					$defs{$1}{$2}{"value"} = $3;
					$defs{$1}{$2}{"comment"} = $last_comment;
					$defs{$1}{$2}{"filename"} = basename ($filename, "");
					$last_comment = ""
				}
			}
			elsif ($line =~ /^\s*$/)
			{
				# Empty line, just skip
				;
			}
			else
			{
				# Remove last comment
				$last_comment = "";
			}
		}
		close (INFILE);
	}

	return %defs;	
}

# Copy all non properties line of the given file in the output stream
sub copy_non_style_definition
{ 
	my ($out, $filename) = @_;

	my %defs = ();
	my $last_comment = "";
	my $line = "";

	print $out "\n\n\n###############################################################################\n";
	my $basename = basename ($filename, "");
	print $out "# From $basename\n";

	print "Open file $filename...\n";
	if (open (INFILE, "<$filename"))
	{
		while ($line = <INFILE>)
		{
			if ($line =~ /^\s*#\s*style\.\w+\.\d+\s*=.+$/)
			{
				# Style put in comment, just discard
				;
			}
			elsif ($line =~ /^\s*#(.+)$/)
			{
				# Comment, print previous one and keep current one
				if ($last_comment)
				{
					print $out "#$last_comment\n";
					$last_comment = "";
				}
				$last_comment = $1;
			}
			elsif ($line =~ /^\s*style\.([\w*]+)\.(\d+)\s*=\s*(.*)\s*$/)
			{
				# Style line, discard it with the last comment
				$last_comment = "";
			}
			elsif ($line =~ /^\s*$/)
			{
				# Empty line, add it
				if ($last_comment)
				{
					$last_comment = $last_comment . "\n";
				}
				else
				{
					print $out "\n";
				}
			}
			else
			{
				# Other line
				if ($last_comment)
				{
					print $out "#$last_comment\n";
					$last_comment = "";
				}
				print $out $line;
			}
		}
		close (INFILE);
		if ($last_comment) {print $out "#$last_comment\n";}
	}

	return %defs;	
}

# Merge new definitions in anjuta defintions
sub merge_style_definition
{
	my ($anjuta_defs, $defs) = @_;

	for my $lang (keys %$defs)
	{
		for my $style (keys %{$defs->{$lang}})
		{
			if ((exists $anjuta_defs->{$lang}) && (exists $anjuta_defs->{$lang}{$style}))
			{
				if ($anjuta_defs->{$lang}{$style}{"filename"} ne basename ($output_file))
				{
					# Properties defined in two SciTE files
					my $file1 = $anjuta_defs->{$lang}{$style}{"filename"};
					my $file2 = $defs->{$lang}{$style}{"filename"};
					print "Warning: properties style.$lang.$style defined in file $file1 and $file2 \n";
				}
				else
				{
					$anjuta_defs->{$lang}{$style}{"comment"} = $defs->{$lang}{$style}{"comment"};
					$anjuta_defs->{$lang}{$style}{"filename"} = $defs->{$lang}{$style}{"filename"};
				}
			}
			else
			{
				# Add properties
				$anjuta_defs->{$lang}{$style}{"value"} = $defs->{$lang}{$style}{"value"};
				$anjuta_defs->{$lang}{$style}{"comment"} = $defs->{$lang}{$style}{"comment"};
				$anjuta_defs->{$lang}{$style}{"filename"} = $defs->{$lang}{$style}{"filename"};
			}
				
		}
	}
}

# Write style defintions
sub write_style_definition
{
	my ($out, $defs, $lang) = @_;
	my $filename = "";	

	for my $style (sort {$a <=> $b} keys %{$defs})
	{
		my $comment;
		my $value;
		if ($defs->{$style}{"filename"} eq basename ($output_file))
		{
			print "Warning: style.$lang.$style is not defined anymore. It has been commented\n";
			$comment = $defs->{$style}{"comment"};
			$value = $defs->{$style}{"value"};
			print $out "#$comment\n";
			print $out "#style.$lang.$style=$value\n";
		}
		else
		{
			if (!$filename)
			{
				$filename = $defs->{$style}{"filename"};
				print $out "\n###############################################################################\n";
				print $out "# Style for $lang from file $filename\n\n"
			}
			elsif ($filename ne $defs->{$style}{"filename"})
			{
				my $file2 = $defs->{$style}{"filename"};
				print "Warning: style.$lang.$style is defined in file $filename and $file2\n";
			}
			
			$comment = $defs->{$style}{"comment"};
			$value = $defs->{$style}{"value"};
			print $out "#$comment\n";
			print $out "\tstyle.$lang.$style=$value\n";

			if (!($value =~ /\$\(style\.anjuta\.\w+\)\s*$/) &&
			   !($value =~ /\$\(style\.gnome\.\w+\)\s*$/))
			{
				print "Warning: style.$lang.$style does not use anjuta properties\n";
			}
		}
	}
	print $out "\n\n";
}

# Create stats hash
sub create_stat_hash
{
	my ($defs) = @_;
	my %stat = ();

	for my $lang (keys %$defs)
	{
		for my $style (keys %{$defs->{$lang}})
		{
			my $value = $defs->{$lang}{$style}{"value"};
			$stat{$value} = [];
		}
	}

	return %stat;
}

# Get statistic about used anjuta value 
sub gather_stat
{
	my ($stat, $anjuta_defs, $defs) = @_;

	for my $lang (keys %$defs)
	{
		for my $style (keys %{$defs->{$lang}})
		{
			if ((exists $anjuta_defs->{$lang}) && (exists $anjuta_defs->{$lang}{$style}))
			{
				my $anjuta_value = $anjuta_defs->{$lang}{$style}{"value"};
				my $value = $defs->{$lang}{$style}{"value"}." ($lang.$style)";
				
				$stat->{$anjuta_value} = [@{$stat->{$anjuta_value}}, $value];
			}
		}
	}
}

# Display statistic about used anjuta value
sub display_stat
{
	my ($stat) = @_;

	for my $anjuta_value (sort keys %$stat)
	{
		print "$anjuta_value\n";
		foreach my $value (sort @{$stat->{$anjuta_value}})
		{
			print "\t$value\n";
		}
	}
}

