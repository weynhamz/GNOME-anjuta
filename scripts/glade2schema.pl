#!/usr/bin/perl

use XML::Parser;

%datatypes = (
	"bool" => "bool",
	"int" => "int",
	"string" => "string",
	"text" => "string",
	"float" => "float",
	"color" => "string",
	"font" => "string"
);

%boolean = (
	0 => "FALSE",
	1 => "TRUE"
);

$schema_path = "/schemas/apps/anjuta/preferences/";
$key_path ="/apps/anjuta/preferences/";

my $parser = new XML::Parser(Style => "Stream");
print "<gconfschemafile>\n";
print "\t<schemalist>\n";

$parser->parsefile($ARGV[0]);

print "\t</schemalist>\n";
print "</gconfschemafile>\n";

sub StartTag {
	my $parser = shift;
	my $key = shift;
	if ($key =~ /widget/) {
		my $k = $_{"id"};
		if ($k =~ /(preferences_color|entry|font|spin|text|toggle):(.*):(.*):(\d):(.*)/) {
			
			my $type = $2;
			my $default = $3;
			my $flags = $4;
			my $propkey = $5;
			
			
			if ($type =~ /bool/) {
				$default = $boolean{$default};
			}
			
			
			
			print "\t\t<schema>\n";
			print "\t\t\t<key>$schema_path$propkey</key>\n";
			print "\t\t\t<applyto>$key_path$propkey</applyto>\n";
			print "\t\t\t<owner>anjuta</owner>\n";
			print "\t\t\t<type>$datatypes{$type}</type>\n";
			print "\t\t\t<default>$default</default>\n";
			
			# Hack to keep gconftool happy
			print "\t\t\t<locale name=\"C\" />\n";
			
			print "\t\t</schema>\n\n";
		}
	}
}

sub EndTag {}

sub Text {}
