#!/usr/bin/perl

use XML::Parser;

%datatypes = (
	"bool" => "b",
	"int" => "i",
	"string" => "s",
	"text" => "s",
	"float" => "f",
	"color" => "s",
	"font" => "s",
	"folder" => "s",
	"file" => "s"
);

%boolean = (
	0 => "false",
	1 => "true"
);

print "<schemalist>\n";
print "\t<schema id=\"$ARGV[1]\" path=\"/apps/anjuta/\">\n";

if ($#ARGV == 2) {
	open FILE, "<", $ARGV[2] or die $!;
	while (<FILE>) { print "\t\t$_"; }
}

my $parser = new XML::Parser(Style => "Stream");
$parser->parsefile($ARGV[0]);

print "\t</schema>\n";
print "</schemalist>";

sub StartTag {
	my %keys = {};
	my $parser = shift;
	my $key = shift;
	if ($key =~ /object/) {
		my $k = $_{"id"};
		if ($k =~ /(preferences_(color|entry|font|spin|text|toggle|menu|folder|file|combo)):(.*):(.*):(\d):(.*)/) {
			my $pref = $2;
			my $type = $3;
			my $default = $4;
			my $flags = $5;
			my $propkey = $6;
			my $realtype = $datatypes{$type};

			if (exists $keys{$propkey})
			{
				return;
			}
			else
			{
				$keys{$propkey} = 1;
			}

			if ($type =~ /bool/) {
				$default = $boolean{$default};
			}


			print "\t\t<key name=\"$propkey\" type=\"$realtype\">\n";
			if ($pref eq "combo") {
				@values = split(',', $default);
				print "\t\t\t<choices>\n";
				foreach (@values) {
					print "\t\t\t\t<choice value=\"$_\" />\n"
				}
				print "\t\t\t</choices>\n";
				print "\t\t\t<default>\"$values[$flags]\"</default>\n";
			}
			elsif ($realtype ne "s") {
				print "\t\t\t<default>$default</default>\n";
			}
			else {
				print "\t\t\t<default>\"$default\"</default>\n";
			}
			print "\t\t</key>\n";
		}
	}
}

sub EndTag {}

sub Text {}