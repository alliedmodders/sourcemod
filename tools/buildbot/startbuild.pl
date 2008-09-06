#!/usr/bin/perl

use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);
chdir('..');
chdir('..');

my ($cmd, $output);

$cmd = 'tools/builder/builder.exe build.cfg 2>&1';

if ($^O eq "linux")
{
    $cmd = 'mono ' . $cmd;
}

print "$cmd\n";
$output = `$cmd`;

print $output;

die "Build failed: $!\n" if $?;

