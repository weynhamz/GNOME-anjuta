#!/usr/bin/perl

## aneditor.h
## Copyright (C) 2000  Kh. Naba Kumar Singh
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


=head Status.pl
Usage: status.pl

This script shows the status of each po files in the current dir.
The transtalion status is given in percentage.

=cut

## Just take the first pot file found as the reference.
my $pot_file = glob("*.pot");
if ($pot_file eq "") {
	print "There is no reference pot file in this directory.\n";
	print "...... Aborting.\n";
	exit (1);
}

## Determine the total messages available in this pot file.
my $output = `msgfmt --statistics $pot_file 2>&1`;
my @strs = split (", ", $output);
my ($total_messages) = split (" ", $strs[2]);

print   "\n";
print   "\t\tTRANSLATION STATISTICS\n";
print   "\t\tTotal messages: $total_messages\n\n";
print   "\tTranslations status given in percentage.\n";
print   "+-----------------------------------------------------+\n";
printf ("|  Po file  | Translated |    Fuzzy   |  Untranslated |\n");
print   "+-----------------------------------------------------+\n";
my @po_files = glob("*.po");

foreach my $po_file (@po_files) {
	if (-f $po_file) {
		my $output = `msgfmt --statistics $po_file 2>&1`;
		
		## print $output, "\n";
		
		my ($translated, $fuzzy, $untranslated) = (0,0,0);
		my @strs = split (", ", $output);
		foreach my $coin (@strs) {
			if ($coin =~ /translated/) {
				($translated) = split (" ", $coin);
			} elsif ($coin =~ /fuzzy/) {
				($fuzzy) = split (" ", $coin);
			} elsif ($coin =~ /untranslated/) {
				($untranslated) = split (" ", $coin);
			}
		}
		
		$translated *= 100/$total_messages;
		$fuzzy *= 100/$total_messages;
		$untranslated *= 100/$total_messages;
		
		printf ("|%10s:|    %5.2f   |    %5.2f   |      %5.2f    |\n", $po_file, $translated, $fuzzy, $untranslated);
	}
}
print   "+-----------------------------------------------------+\n";

__END__
