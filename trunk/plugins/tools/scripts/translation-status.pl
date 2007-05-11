#!/usr/bin/perl

## status.pl
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

## modified 2001-11-05 Andy Piper <andy.piper@freeuk.com>
## - added further countries to the hash table
## - altered XML file output
## - corrected typos

=head Status.pl
Usage: status.pl

This script shows the status of each po file in the current dir.
The translation status is given as a percentage.

=cut

## Output xml file for translation status
my $translation_file = "translations.xml";

## Country code to country name hash map
## Some contry codes may need to be filled.
my $country_hr = {
		"az" => "Azerbaijani",
		"br" => "Breton",
		"bg" => "Bulgarian", 
		"ca" => "Catalan",
		"cs" => "Czech",
		"da" => "Danish",
		"de" => "German",
		"el" => "Greek",
		"en_GB" => "British English",
		"en_US" => "American English",
		"eo" => "Esperanto",
		"es" => "Spanish",
		"et" => "Estonian",
		"eu" => "Basque",
		"fi" => "Finnish",
		"fr" => "French",
		"he" => "Hebrew",
		"hr" => "Croatian",
		"hu" => "Hungarian",
		"is" => "Icelandic",
		"it" => "Italian",
		"ja" => "Japanese",
		"ko" => "Korean",
		"lt" => "Lithuanian",
		"ms" => "Malay",
		"mk" => "Macedonian",
		"nl" => "Dutch",
		"nn" => "Norwegian Nynorsk",
		"no" => "Norwegian",
		"pl" => "Polish",
		"pt" => "Portuguese",
		"pt_BR" => "Brazilian Portuguese",
		"ro" => "Romanian",
		"ru" => "Russian",
		"sk" => "Slovak",
		"sl" => "Slovenian",
		"sr" => "Serbian",
		"sv" => "Swedish",
		"ta" => "Tamil",
		"tr" => "Turkish",
		"uk" => "Ukrainian",
		"vi" => "Vietnamese",
		"wa" => "Walloon",
		"zh_CN.GB2312" => "Simplified Chinese",
		"zh_CN" => "Simplified Chinese",
		"zh_TW" => "Traditional Chinese"
	};

my ($project_root) = @ARGV;

if (!defined ($project_root) || $project_root eq "") {
	print STDERR "Error: No project\n";
	exit(1);
}
if ($project_root !~ /^\//) {
	print STDERR "Error: Project root is not absolute path\n";
	exit(1);
}
unless (-d "$project_root/po") {
	print STDERR "Error: Project does not have translations\n";
	exit(1);
}

chdir("$project_root/po");

## Generate/update .pot file
system("intltool-update -p");

## Determine missing files.
system("intltool-update --maintain");

## Just take the first pot file found as the reference.
my $pot_file = glob("*.pot");
if ($pot_file eq "") {
	print "There is no reference pot file in this directory.\n";
	print "Make sure you are running status.pl in a po directory.\n";
	print "...... Aborting.\n";
	exit (1);
}

## the appname will be the name of the pot file
$appname = $pot_file;
while ($strip ne ".") {
	$strip = chop ($appname);
}

## Determine the total messages available in this pot file.
my $output = `msgfmt --statistics $pot_file 2>&1`;
my @strs = split (", ", $output);
my ($pot_fuzzy, $pot_translated, $pot_untranslated) = (0,0,0);
foreach my $term (@strs) {
    if ($term =~ /translated/) {
	($pot_translated) = split (" ", $term);
    } elsif ($term =~ /fuzzy/) {
	($pot_fuzzy) = split (" ", $term);
    } elsif ($term =~ /untranslated/) {
	($pot_untranslated) = split (" ", $term);
    }
}
my $total_messages = $pot_translated + $pot_fuzzy + $pot_untranslated;

print   "\n";
print   "\t\tTRANSLATION STATISTICS\n";
print   "\t\tTotal messages: $total_messages\n\n";
print   "\tTranslation status given in percentage.\n";
print   "+----------------------------------------------------------------------------+\n";
printf ("|  Po file  |            Language  | Translated |   Fuzzy   |   Untranslated |\n");
print   "+----------------------------------------------------------------------------+\n";
my @po_files = glob("*.po");

my $date_time = gmtime(time());

foreach my $po_file (@po_files) {
	if (-f $po_file) {
		my $output = `msgfmt --statistics $po_file 2>&1`;
		
		## print $output, "\n";
		
		my ($translated, $fuzzy, $untranslated) = (0,0,0);
		my @strs = split (", ", $output);
		foreach my $coin (@strs) {
			if ($coin =~ /\btranslated\b/) {
				($translated) = split (" ", $coin);
			} elsif ($coin =~ /\bfuzzy\b/) {
				($fuzzy) = split (" ", $coin);
			} elsif ($coin =~ /\buntranslated\b/) {
				($untranslated) = split (" ", $coin);
			}
		}
		$untranslated = $total_messages - ($translated + $fuzzy);

		$untranslated *= 100/$total_messages;
		$untranslated = ($untranslated < 0)? 0:$untranslated;
		$untranslated = ($untranslated > 100)? 100:$untranslated;

		$translated *= 100/$total_messages;
		$translated = ($translated < 0)? 0:$translated;
		$translated = ($translated > 100)? 100:$translated;

		$fuzzy *= 100/$total_messages;
		$fuzzy = ($fuzzy < 0)? 0:$fuzzy;
		$fuzzy = ($fuzzy > 100)? 100:$fuzzy;

		my $country_code = $po_file;
		$country_code =~ s/\.po$//;
		my $country = $country_hr->{$country_code};
		$country = ($country ne "")? $country:"-";
		
		printf ("|%10s |%21s |    %6.2f  ", $po_file, $country, $translated);
		printf ("|    %6.2f |      %6.2f    |\n", $fuzzy, $untranslated);
	}
}
print   "+----------------------------------------------------------------------------+\n";

__END__
