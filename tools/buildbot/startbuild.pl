#!/usr/bin/perl
# vim: set ts=2 sw=2 tw=99 noet: 

use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

chdir('../../../OUTPUT');

if ($^O eq "linux") {
	system("python3.1 build.py");
} else {
	system("C:\\Python31\\python.exe build.py");
}

if ($? == -1)
{
	die "Build failed: $!\n";
}
elsif ($^O eq "linux" and $? & 127)
{
	die "Build died :(\n";
}
elsif ($? >> 8 != 0)
{
	die "Build failed with exit code: " . ($? >> 8) . "\n";
}
else
{
	exit(0);
}

