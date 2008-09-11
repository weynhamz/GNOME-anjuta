#!/usr/bin/perl -w

# Copyright 2007 Naba Kumar  <naba@gnome.org>
# License: GPL

my @files = @ARGV;
my %trans_strings;

foreach my $one_file (@files) {

    if ($one_file !~ /\.wiz$/)
    {
	next;
    }
    my $content = `cat $one_file`;

    # Find translatable attribute (name starting with _)
    my @strings = $content =~ /_[\d\w\-\_]+=\"([^\"]+)\"/sgm;
    foreach my $one_string (@strings)
    {
	$one_string =~ s/&gt;/>/gs;
	$one_string =~ s/&lt;/</gs;
	$one_string =~ s/&quot;/\\"/gs;
	$trans_strings{$one_string} = $one_string;
    }
    
    # Find translatable content (element name starting with _)
    @strings = $content =~ /<\s*_[^\>]+>([^\<]+)</sgm;
    foreach my $one_string (@strings)
    {
	$one_string =~ s/&gt;/>/gs;
	$one_string =~ s/&lt;/</gs;
	$one_string =~ s/&quot;/\\"/gs;
	$trans_strings{$one_string} = $one_string;
    }
}
foreach my $one_trans_string (sort keys (%trans_strings))
{
    print "char *s = N_(\"$one_trans_string\");\n";
}
