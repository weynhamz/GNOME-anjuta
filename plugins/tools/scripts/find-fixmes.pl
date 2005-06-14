#!/usr/bin/perl -w

## Prepare ChangeLog

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
unless (-d "$project_root/CVS" || -d "$project_root/.svn") {
	print STDERR "Error: Project is in neither cvs or subversion working copy.\n";
	exit(1);
}

sub process_directory
{
	my ($dir, $data_hr) = @_;
	
	if ($dir ne "") {
		print STDERR "Scanning $dir\n";
	} else {
		print STDERR "Scanning .\n";
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
