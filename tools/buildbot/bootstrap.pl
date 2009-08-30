#!/usr/bin/perl
# vim: set ts=2 sw=2 tw=99 noet: 

use strict;
use Cwd;
use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

#Go back to main source dir.
chdir(Build::PathFormat('../..'));

#Get the source path.
our ($root) = getcwd();

my $reconf = 0;

#Create output folder if it doesn't exist.
if (!(-d 'OUTPUT')) {
	mkdir('OUTPUT') or die("Failed to create output folder: $!\n");
	$reconf = 1;
} else {
	if (-d 'OUTPUT/sentinel') {
		my @s = stat('OUTPUT/sentinel');
		my $mtime = $s[9];
		my @files = ('pushbuild.txt', 'AMBuildScript', 'product.version');
		my ($i);
		for ($i = 0; $i <= $#files; $i++) {
			if (IsNewer($files[$i], $mtime)) {
				$reconf = 1;
				last;
			}
		}
	} else {
		$reconf = 1;
	}
}

if ($reconf) {
	chdir('OUTPUT');
	my ($result);
	print "Attempting to reconfigure...\n";
	if ($^O eq "linux") {
		$result = system('python3.1 ../configure.py --enable-optimize');
	} else {
		$result = system('C:\\Python31\\Python.exe ..\\configure.py --enable-optimize')
	}
	print "$result\n";
}

open(FILE, '>OUTPUT/sentinel');
close(FILE);

sub IsNewer
{
	my ($file, $time) = (@_);

	my @s = stat($file);
	my $mtime = $s[9];
	return $mtime > $time;
}

