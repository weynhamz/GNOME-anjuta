#!/usr/bin/perl

## prepare-changelog.pl
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
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

my ($project_root) = @ARGV;

if (!defined ($project_root) || $project_root eq "") {
	print STDERR "Error: No project\n";
	exit(1);
}
if ($project_root !~ /^\//) {
	print STDERR "Error: Project root is not absolute path\n";
	exit(1);
}
unless (-f "$project_root/ChangeLog") {
	print STDERR "Error: Project does not have a ChangeLog file\n";
	exit(1);
}
unless (-d "$project_root/CVS") {
	print STDERR "Error: Project is in not a CVS working copy.\n";
	exit(1);
}

my %data_hash;
print "Preparing ChangeLog for $project_root\n";
process_directory ("", \%data_hash);

my @files = keys (%data_hash);

if (@files <= 0) {
	print "No changes made to project\n";
	exit(0);
}

my ($a, $b, $c, $mday, $month, $year) = gmtime(time());
$year += 1900;

my $date_str = sprintf ("%04d-%02d-%02d", $year, $month, $mday);
my $changelog = "$date_str $ENV{USER}  <$ENV{USER}\@$ENV{HOSTNAME}>\n\n";

my $first = 1;
foreach my $file (sort @files) {
	if ($first == 1) {
		$changelog .= "\t* $file";
		$first = 0;
	} else {
		$changelog .= ",\n\t$file";
	}
}
$changelog .= ":\n\n";

my $changelog_file = "$project_root/ChangeLog";
if ((-r $changelog_file) && (-w $changelog_file)) {
	$changelog .= `cat $changelog_file`;
	unless (open (CHLOG, ">$changelog_file")) {
		print STDERR "Failed to open $changelog_file for reading/writing\n";
		exit (1);
	}
	print CHLOG $changelog;
	close (CHLOG);
	print "\032\032$changelog_file:0\n";
} else {
	print STDERR "Failed to open $changelog_file for reading/writing\n";
	exit(1);
}

sub process_directory
{
	my ($dir, $data_hr) = @_;
	
	if ($dir ne "") {
		print "Scanning $dir\n";
	} else {
		print "Scanning .\n";
	}
	open (ENTRIES, "$project_root/$dir/CVS/Entries") or
		die "Can not open $project_root/$dir/CVS/Entries for reading";
	
	my @lines = <ENTRIES>;
	@lines = sort @lines;
	
	foreach my $line (@lines) {
		chomp($line);
		if ($line =~ /^\/([^\/]+)\/([^\/]+)\/([^\/]+)/) {
			my $file = $1;
			my $repo_time = $3;
			if (-f "$project_root/$dir/$file") {
				my $file_time = `date -u -r $project_root/$dir/$file`;
				$file_time =~ s/\s+$//sg;
				$file_time =~ s/\w+\s+(\d+)$/$1/;
				## print "Comparing '$repo_time' and '$file_time'\n";
				if ($file_time ne $repo_time) {
					my $path = "$dir/$file";
					$path =~ s/^\///;
					$data_hr->{$path} = "modified";
				}
			}
		}
	}
	foreach my $line (@lines) {
		chomp($line);
		if ($line =~ /^D\/([^\/]+)/) {
			if (-d "$project_root/$dir/$1") {
				if ($dir ne "") {
					process_directory ("$dir/$1", $data_hr);
				} else {
					process_directory ("$1", $data_hr);
				}
			}
		}
	}
}
