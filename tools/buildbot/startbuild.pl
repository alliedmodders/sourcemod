#!/usr/bin/perl
# vim: set ts=2 sw=2 tw=99 noet: 

use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

chdir('../../../OUTPUT');

my $argn = $#ARGV + 1;
if ($argn > 0) {
	$ENV{CC} = $ARGV[0];
	$ENV{CXX} = $ARGV[1];
}

if ($^O !~ /MSWin/) {
	system("ambuild --no-color 2>&1");
} else {
	system("C:\\Python38\\scripts\\ambuild --no-color 2>&1");
}

if ($? != 0)
{
	die "Build failed: $!\n";
}
else
{
	exit(0);
}

