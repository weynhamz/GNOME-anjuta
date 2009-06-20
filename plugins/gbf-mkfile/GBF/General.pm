package GBF::General;

use Exporter;
use strict;

our (@ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

@ISA = qw (Exporter);

@EXPORT = qw (&report_error &report_warning
	      &trim &canonicalize_name &empty_if_undef &expand_tabs
	      &base_name &dir_name &chop_backslash &max &min);
@EXPORT_OK = qw ($debug);
%EXPORT_TAGS = ();



######################################################################
####  SUBROUTINES AND HELPER FUNCTIONS  ##############################
######################################################################

sub report_error
{
    my ($code, $message) = @_;

    print STDERR "ERROR($code): $message\n";

    return $code;
}

sub report_warning
{
    my ($code, $message) = @_;

    print STDERR "WARNING($code): $message\n";
}

sub canonicalize_name
{
    my ($name) = @_;

    $name =~ s'[^0-9@A-Z_a-z]'_'g;

    return $name;
}

sub trim
{
    $_ = $_[0];

    s/^\s*//;
    s/\s*$//;

    return $_;
}

sub empty_if_undef
{
    return defined ($_[0]) ? $_[0] : "";
}

sub expand_tabs
{
    $_ = $_[0];
    my $i;
    while (/\t/) {
	$i = index ($_, "\t");
	$i = $i % 8;
	if ($i == 0) {
	    s/\t/        /;
	} else {
	    my $pad = " " x (8 - $i);
	    s/\t/$pad/;
	};
    };
    return $_;
}

sub base_name {
    my ($name, $dir);
    
    $name = $_[0];
    $dir = dir_name ($name);
    $name =~ s/$dir//;

    return $name;
}

sub dir_name {
    my ($name);

    $name = $_[0];
    $name =~ s/[^\/]+$//;

    return $name;
}

sub chop_backslash ($)
{
    $_ = shift;
    s/\\\s*$//;
    return $_;
}

sub max
{
    my $max = shift;
    foreach my $x (@_) { $max = $x if $x > $max; };	
    return $max;
}

sub min
{
    my $min = shift;
    foreach my $x (@_) { $min = $x if $x < $min; };	
    return $min;
}

1;
