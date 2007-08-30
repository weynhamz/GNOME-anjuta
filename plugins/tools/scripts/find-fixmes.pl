#!/usr/bin/perl

## find-fixmes.pl
## Copyright (C) Naba Kumar  <naba@gnome.org>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Library General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

my ($project_root) = @ARGV;

if (!defined ($project_root) || $project_root eq "") {
	print STDERR "Error: No project\n";
	exit(1);
}
if ($project_root !~ /^\//) {
	print STDERR "Error: Project root is not absolute path\n";
	exit(1);
}

my %data_hr;
process_directory ("", \%data_hr);

sub process_directory
{
	my ($dir, $data_hr) = @_;
	
	if ($dir ne "") {
		print STDERR "Scanning: $dir\n";
	} else {
		print STDERR "Scanning: .\n";
	}
	
	my (@files, @directories);
	if (-f "$project_root/$dir/CVS/Entries") {
		open (ENTRIES, "$project_root/$dir/CVS/Entries") or
			die "Can not open $project_root/$dir/CVS/Entries for reading";
	
		my @lines = <ENTRIES>;
		@lines = sort @lines;
	
		foreach my $line (@lines) {
			chomp($line);
			if ($line =~ /^\/([^\/]+)\//) {
				push @files, $1;
			}
		}
		foreach my $line (@lines) {
			chomp($line);
			if ($line =~ /^D\/([^\/]+)/) {
				push @directories, $1;
			}
		}
	} else {
		opendir (ENTRIES, "$project_root/$dir") or
			die "Can not open $project_root/$dir for reading";
		my @lines = readdir(ENTRIES);
		foreach my $line (@lines) {
			next if ($line eq "." || $line eq "..");
			if (-d "$project_root/$dir/$line") {
				push @directories, $line;
			} else {
				push @files, $line;
			}
		}
	}
	foreach my $file (@files) {
		my $file_path = "$project_root/$dir/$file";
		$file_path =~ s/\/\//\//g;
		process_file ($file_path, $data_hr) if (-f $file_path);
	}
	
	foreach my $directory (@directories) {
		if (-d "$project_root/$dir/$directory") {
			if ($dir ne "") {
				process_directory ("$dir/$directory", $data_hr);
			} else {
				process_directory ("$directory", $data_hr);
			}
		}
	}
}

sub process_file
{
	my ($path, $data_hr) = @_;
	my $content = `cat $path`;
	
	## C/C++ specific search
	if ($path =~ /\.(c|cc|cpp|cxx|h|hpp|hxx|hh)$/) {
		my @data = $content =~ /(\n|\/\*.*?\*\/|\/\/.*?\n)/sgi;
		my $continued = 0;
		my $lines = 0;
		foreach my $match (@data) {
			if ($match eq "\n") {
				$continued = 0;
			} elsif ($match =~ /\bfixme\b/is) {
				print "$path:$lines\n";
				$match = "\t$match";
				$match =~ s/\n\s*/\n\t/sg;
				$match =~ s/\s*$//s;
				print "$match\n";
				$continued = 1;
			} else {
				if ($continued == 1 && $match =~ /^(\/\*|\/\/)/) {
					$match = "\t$match";
					$match =~ s/\n\s*/\n\t/sg;
					$match =~ s/\s*$//s;
					print "$match\n";
				}
			}
			$lines += $match =~ /(\n)/sg;
		}
	} else {
		my @data = split(/\n/, $content);
		
		my $lines = 0;
		foreach my $match (@data) {
			if ($match =~ /\bfixme\b/is) {
				print "$path:$lines\n";
				$match =~ s/^\s*/\t/sg;
				print "$match\n";
			}
			$lines++;
		}
	}
}
